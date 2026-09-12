#include "codegen.hpp"
#include "ast.hpp"
#include "semantic.hpp"
#include <cassert>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Support/CodeGen.h>

#if __has_include(<llvm/MC/TargetRegistry.h>)
#include <llvm/MC/TargetRegistry.h>
#else
#include <llvm/Support/TargetRegistry.h>
#endif

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/CGSCCAnalysisManager.h>

#if __has_include(<llvm/Passes/OptimizationLevel.h>)
#include <llvm/Passes/OptimizationLevel.h>
using OptLevelType = llvm::OptimizationLevel;
#else
using OptLevelType = llvm::PassBuilder::OptimizationLevel;
#endif

#include <iostream>

using namespace llvm;

CodeGenContext::CodeGenContext()
    : context(std::make_unique<LLVMContext>()),
      builder(*context) {
    module = std::make_unique<Module>("main", *context);
    pushScope(); // Create global scope
}

Function* CodeGenContext::getPrintfFunction() {
    if (printfFunction) return printfFunction;
    FunctionType* printfType = FunctionType::get(
        Type::getInt32Ty(*context),
        PointerType::get(Type::getInt8Ty(*context), 0),
        true // variadic
    );
    printfFunction = Function::Create(
        printfType,
        Function::ExternalLinkage,
        "printf",
        module.get()
    );
    return printfFunction;
}

Function* CodeGenContext::generateCode(Program* prog) {
    // Separate function definitions and other top-level statements
    std::vector<FunctionDef*> functionDefs;
    std::vector<Statement*> topLevelStmts;
    bool hasExplicitMain = false;

    for (const auto& stmt : prog->getStatements()) {
        if (auto funcDef = dynamic_cast<FunctionDef*>(stmt.get())) {
            functionDefs.push_back(funcDef);
            if (funcDef->name == "main") {
                hasExplicitMain = true;
            }
        } else {
            topLevelStmts.push_back(stmt.get());
        }
    }

    // Codegen all user-defined functions
    for (auto* funcDef : functionDefs) {
        if (!codegen(funcDef)) {
            std::cerr << "Error generating code for function: " << funcDef->name << std::endl;
            return nullptr;
        }
    }

    // If an explicit main was defined and there are top-level statements,
    // prepend them to main; otherwise create synthetic main
    if (hasExplicitMain) {
        Function* mainFunc = module->getFunction("main");
        if (!topLevelStmts.empty() && mainFunc) {
            BasicBlock& entryBB = mainFunc->getEntryBlock();
            builder.SetInsertPoint(&entryBB, entryBB.begin());
            setCurrentFunction(mainFunc);
            for (auto* stmt : topLevelStmts) {
                codegen(stmt);
            }
        }
        return mainFunc;
    }

    // Create synthetic main function for script-style top-level statements
    FunctionType* funcType = FunctionType::get(
        Type::getInt32Ty(*context), 
        false
    );
    
    Function* mainFunc = Function::Create(
        funcType, 
        Function::ExternalLinkage, 
        "main", 
        module.get()
    );

    BasicBlock* entry = BasicBlock::Create(*context, "entry", mainFunc);
    builder.SetInsertPoint(entry);
    setCurrentFunction(mainFunc);

    for (auto* stmt : topLevelStmts) {
        codegen(stmt);
        if (builder.GetInsertBlock()->getTerminator()) {
            break;
        }
    }

    if (builder.GetInsertBlock()->getTerminator() == nullptr) {
        builder.CreateRet(ConstantInt::get(Type::getInt32Ty(*context), 0));
    }

    if (verifyFunction(*mainFunc, &llvm::errs())) {
        std::cerr << "Error: Function verification failed!" << std::endl;
        return nullptr;
    }

    return mainFunc;
}

// Scope management
void CodeGenContext::pushScope() {
    scopes.emplace_back();
    symbolTable.enterScope();
}

void CodeGenContext::popScope() {
    if (!scopes.empty()) {
        scopes.pop_back();
    }
    if (symbolTable.getScopeDepth() > 1) {
        symbolTable.exitScope();
    }
}

// Loop management
void CodeGenContext::pushLoop(BasicBlock* breakBB, BasicBlock* continueBB) {
    loopStack.push_back({breakBB, continueBB});
}

void CodeGenContext::popLoop() {
    if (!loopStack.empty()) {
        loopStack.pop_back();
    }
}

BasicBlock* CodeGenContext::getCurrentLoopEnd() const {
    return loopStack.empty() ? nullptr : loopStack.back().breakBB;
}

BasicBlock* CodeGenContext::getCurrentLoopContinue() const {
    return loopStack.empty() ? nullptr : loopStack.back().continueBB;
}

// Variable management
AllocaInst* CodeGenContext::findVariable(const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->count(name)) return it->at(name);
    }
    return nullptr;
}

void CodeGenContext::registerVariable(const std::string& name, AllocaInst* alloca) {
    if (!scopes.empty()) {
        scopes.back()[name] = alloca;
    }
}

// Expression codegen dispatch
Value* CodeGenContext::codegen(Expression* expr) {
    if (!expr) return nullptr;
    if (auto intLit = dynamic_cast<IntegerLiteral*>(expr)) {
        return codegen(intLit);
    }
    if (auto strLit = dynamic_cast<StringLiteral*>(expr)) {
        return codegen(strLit);
    }
    if (auto varExpr = dynamic_cast<VariableExpr*>(expr)) {
        return codegen(varExpr);
    }
    if (auto unaryExpr = dynamic_cast<UnaryExpr*>(expr)) {
        return codegen(unaryExpr);
    }
    if (auto binExpr = dynamic_cast<BinaryExpr*>(expr)) {
        return codegen(binExpr);
    }
    if (auto cmpExpr = dynamic_cast<ComparisonExpr*>(expr)) {
        return codegen(cmpExpr);
    }
    if (auto logExpr = dynamic_cast<LogicalExpr*>(expr)) {
        return codegen(logExpr);
    }
    if (auto callExpr = dynamic_cast<FunctionCall*>(expr)) {
        return codegen(callExpr);
    }
    return nullptr;
}

// Statement codegen dispatch
Value* CodeGenContext::codegen(Statement* stmt) {
    if (!stmt) return nullptr;
    if (auto retStmt = dynamic_cast<ReturnStatement*>(stmt)) {
        return codegen(retStmt);
    }
    if (auto varDecl = dynamic_cast<VarDeclaration*>(stmt)) {
        return codegen(varDecl);
    }
    if (auto assign = dynamic_cast<Assignment*>(stmt)) {
        return codegen(assign);
    }
    if (auto ifStmt = dynamic_cast<IfStatement*>(stmt)) {
        return codegen(ifStmt);
    }
    if (auto whileStmt = dynamic_cast<WhileStatement*>(stmt)) {
        return codegen(whileStmt);
    }
    if (auto forStmt = dynamic_cast<ForStatement*>(stmt)) {
        return codegen(forStmt);
    }
    if (auto block = dynamic_cast<Block*>(stmt)) {
        return codegen(block);
    }
    if (auto exprStmt = dynamic_cast<ExprStatement*>(stmt)) {
        return codegen(exprStmt);
    }
    if (auto breakStmt = dynamic_cast<BreakStatement*>(stmt)) {
        return codegen(breakStmt);
    }
    if (auto contStmt = dynamic_cast<ContinueStatement*>(stmt)) {
        return codegen(contStmt);
    }
    if (auto funcDef = dynamic_cast<FunctionDef*>(stmt)) {
        return codegen(funcDef);
    }
    return nullptr;
}

// Specific expression implementations
Value* CodeGenContext::codegen(IntegerLiteral* expr) {
    return ConstantInt::get(
        Type::getInt32Ty(*context),
        expr->value,
        true
    );
}

Value* CodeGenContext::codegen(StringLiteral* expr) {
    return builder.CreateGlobalStringPtr(expr->value, "strtmp");
}

Value* CodeGenContext::codegen(VariableExpr* expr) {
    assert(symbolTable.isDeclared(expr->name) && "Semantic error leaked into codegen: variable not declared");
    AllocaInst* alloca = findVariable(expr->name);
    assert(alloca && "Internal error: variable declared but not allocated");
    return builder.CreateLoad(
        alloca->getAllocatedType(),
        alloca,
        expr->name.c_str()
    );
}

Value* CodeGenContext::codegen(UnaryExpr* expr) {
    Value* val = expr->operand->codegen(*this);
    if (!val) return nullptr;

    if (expr->op == '-') {
        return builder.CreateNeg(val, "negtmp");
    }
    if (expr->op == '!') {
        Value* zero = ConstantInt::get(val->getType(), 0);
        Value* cmp = builder.CreateICmpEQ(val, zero, "nottmp");
        return builder.CreateZExt(cmp, Type::getInt32Ty(*context), "notcast");
    }
    std::cerr << "Invalid unary operator: " << expr->op << std::endl;
    return nullptr;
}

Value* CodeGenContext::codegen(BinaryExpr* expr) {
    Value* L = expr->lhs->codegen(*this);
    Value* R = expr->rhs->codegen(*this);
    if (!L || !R) return nullptr;

    switch (expr->op) {
        case '+': return builder.CreateAdd(L, R, "addtmp");
        case '-': return builder.CreateSub(L, R, "subtmp");
        case '*': return builder.CreateMul(L, R, "multmp");
        case '/': return builder.CreateSDiv(L, R, "divtmp");
        case '%': return builder.CreateSRem(L, R, "remtmp");
        default:
            std::cerr << "Invalid binary operator: " << expr->op << std::endl;
            return nullptr;
    }
}

Value* CodeGenContext::codegen(ComparisonExpr* expr) {
    Value* L = expr->lhs->codegen(*this);
    Value* R = expr->rhs->codegen(*this);
    if (!L || !R) return nullptr;

    if (expr->op == "<") return builder.CreateICmpSLT(L, R, "cmptmp");
    if (expr->op == "<=") return builder.CreateICmpSLE(L, R, "cmptmp");
    if (expr->op == ">") return builder.CreateICmpSGT(L, R, "cmptmp");
    if (expr->op == ">=") return builder.CreateICmpSGE(L, R, "cmptmp");
    if (expr->op == "==") return builder.CreateICmpEQ(L, R, "cmptmp");
    if (expr->op == "!=") return builder.CreateICmpNE(L, R, "cmptmp");

    std::cerr << "Invalid comparison operator: " << expr->op << std::endl;
    return nullptr;
}

Value* CodeGenContext::codegen(LogicalExpr* expr) {
    Function* func = builder.GetInsertBlock()->getParent();

    Value* lVal = expr->lhs->codegen(*this);
    if (!lVal) return nullptr;

    Value* lCond = builder.CreateICmpNE(
        lVal,
        ConstantInt::get(lVal->getType(), 0),
        "lcond"
    );

    BasicBlock* startBB = builder.GetInsertBlock();
    BasicBlock* rhsBB = BasicBlock::Create(*context, "logical.rhs", func);
    BasicBlock* mergeBB = BasicBlock::Create(*context, "logical.merge", func);

    if (expr->op == "&&") {
        builder.CreateCondBr(lCond, rhsBB, mergeBB);
    } else if (expr->op == "||") {
        builder.CreateCondBr(lCond, mergeBB, rhsBB);
    } else {
        std::cerr << "Unknown logical operator: " << expr->op << std::endl;
        return nullptr;
    }

    // RHS Block
    builder.SetInsertPoint(rhsBB);
    Value* rVal = expr->rhs->codegen(*this);
    if (!rVal) return nullptr;
    Value* rCond = builder.CreateICmpNE(
        rVal,
        ConstantInt::get(rVal->getType(), 0),
        "rcond"
    );
    BasicBlock* rhsEndBB = builder.GetInsertBlock();
    builder.CreateBr(mergeBB);

    // Merge Block with PHI node
    builder.SetInsertPoint(mergeBB);
    PHINode* phi = builder.CreatePHI(Type::getInt1Ty(*context), 2, "logical.res");
    if (expr->op == "&&") {
        phi->addIncoming(ConstantInt::get(Type::getInt1Ty(*context), 0), startBB);
        phi->addIncoming(rCond, rhsEndBB);
    } else {
        phi->addIncoming(ConstantInt::get(Type::getInt1Ty(*context), 1), startBB);
        phi->addIncoming(rCond, rhsEndBB);
    }

    return builder.CreateZExt(phi, Type::getInt32Ty(*context), "logical.ext");
}

Value* CodeGenContext::codegen(FunctionCall* expr) {
    // Built-in print helper
    if (expr->name == "print") {
        if (!expr->args || expr->args->size() != 1) {
            std::cerr << "Error: 'print' expects exactly 1 argument" << std::endl;
            return nullptr;
        }

        Value* argVal = (*expr->args)[0]->codegen(*this);
        if (!argVal) return nullptr;

        Function* printfFunc = getPrintfFunction();
        Value* fmtStr = nullptr;

        if (argVal->getType()->isPointerTy()) {
            fmtStr = builder.CreateGlobalStringPtr("%s\n", "fmt_str");
        } else {
            fmtStr = builder.CreateGlobalStringPtr("%d\n", "fmt_int");
        }

        return builder.CreateCall(printfFunc, {fmtStr, argVal}, "print_call");
    }

    // Generic function call
    Function* callee = module->getFunction(expr->name);
    if (!callee) {
        std::cerr << "Unknown function: " << expr->name << std::endl;
        return nullptr;
    }

    std::vector<Value*> args;
    if (expr->args) {
        for (auto& arg : *expr->args) {
            args.push_back(arg->codegen(*this));
            if (!args.back()) return nullptr;
        }
    }

    if (callee->getReturnType()->isVoidTy()) {
        return builder.CreateCall(callee, args);
    }
    return builder.CreateCall(callee, args, "calltmp");
}

// Specific statement implementations
Value* CodeGenContext::codegen(FunctionDef* stmt) {
    Type* retType = (stmt->returnType == "void") 
        ? Type::getVoidTy(*context)
        : Type::getInt32Ty(*context);

    std::vector<Type*> paramTypes;
    for (const auto& p : stmt->params) {
        paramTypes.push_back(Type::getInt32Ty(*context));
    }

    FunctionType* funcType = FunctionType::get(retType, paramTypes, false);
    Function* func = Function::Create(
        funcType,
        Function::ExternalLinkage,
        stmt->name,
        module.get()
    );

    BasicBlock* entry = BasicBlock::Create(*context, "entry", func);
    BasicBlock* savedBlock = builder.GetInsertBlock();
    Function* savedFunc = currentFunction;

    setCurrentFunction(func);
    builder.SetInsertPoint(entry);
    pushScope();

    // Allocate parameters and store arguments
    size_t idx = 0;
    for (auto& arg : func->args()) {
        const std::string& paramName = stmt->params[idx].name;
        arg.setName(paramName);

        AllocaInst* alloca = builder.CreateAlloca(arg.getType(), nullptr, paramName);
        builder.CreateStore(&arg, alloca);
        registerVariable(paramName, alloca);
        symbolTable.declare(paramName, SymbolType::VARIABLE);
        idx++;
    }

    // Generate body
    if (stmt->body) {
        codegen(stmt->body.get());
    }

    // Ensure terminator
    if (builder.GetInsertBlock()->getTerminator() == nullptr) {
        if (retType->isVoidTy()) {
            builder.CreateRetVoid();
        } else {
            builder.CreateRet(ConstantInt::get(retType, 0));
        }
    }

    popScope();
    if (savedBlock) {
        builder.SetInsertPoint(savedBlock);
    }
    setCurrentFunction(savedFunc);

    if (verifyFunction(*func, &llvm::errs())) {
        std::cerr << "Error: Function verification failed for '" << stmt->name << "'" << std::endl;
        return nullptr;
    }

    return func;
}

Value* CodeGenContext::codegen(VarDeclaration* stmt) {
    if (!symbolTable.declare(stmt->name, SymbolType::VARIABLE)) {
        std::cerr << "Error: Variable '" << stmt->name << "' already declared in this scope" << std::endl;
        return nullptr;
    }
    
    Type* type = Type::getInt32Ty(*context);
    AllocaInst* alloca = builder.CreateAlloca(
        type, 
        nullptr, 
        stmt->name
    );
    
    registerVariable(stmt->name, alloca);
    
    if (stmt->init) {
        Value* initVal = stmt->init->codegen(*this);
        if (!initVal) return nullptr;
        builder.CreateStore(initVal, alloca);
    } else {
        builder.CreateStore(ConstantInt::get(type, 0), alloca);
    }
    
    return alloca;
}

Value* CodeGenContext::codegen(Assignment* stmt) {
    AllocaInst* alloca = findVariable(stmt->name);
    assert(alloca && "Semantic error leaked into codegen: assignment to unallocated variable");

    Value* val = stmt->expr->codegen(*this);
    assert(val && "Internal error: assignment expression codegen failed");

    builder.CreateStore(val, alloca);
    return val;
}

Value* CodeGenContext::codegen(ReturnStatement* stmt) {
    if (stmt->expr) {
        Value* retVal = stmt->expr->codegen(*this);
        if (!retVal) return nullptr;
        return builder.CreateRet(retVal);
    }
    return builder.CreateRetVoid();
}

Value* CodeGenContext::codegen(IfStatement* stmt) {
    Value* condVal = stmt->condition->codegen(*this);
    if (!condVal) return nullptr;
    
    condVal = builder.CreateICmpNE(
        condVal, 
        ConstantInt::get(condVal->getType(), 0), 
        "ifcond");

    Function* func = builder.GetInsertBlock()->getParent();

    BasicBlock* thenBB = BasicBlock::Create(*context, "then", func);
    BasicBlock* elseBB = BasicBlock::Create(*context, "else");
    BasicBlock* mergeBB = BasicBlock::Create(*context, "ifcont");

    builder.CreateCondBr(condVal, thenBB, stmt->elseBlock ? elseBB : mergeBB);

    // Then branch
    builder.SetInsertPoint(thenBB);
    codegen(stmt->thenBlock.get());
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(mergeBB);
    }

    // Else branch
    if (stmt->elseBlock) {
        func->insert(func->end(), elseBB);
        builder.SetInsertPoint(elseBB);
        codegen(stmt->elseBlock.get());
        if (!builder.GetInsertBlock()->getTerminator()) {
            builder.CreateBr(mergeBB);
        }
    }

    func->insert(func->end(), mergeBB);
    builder.SetInsertPoint(mergeBB);

    return nullptr;
}

Value* CodeGenContext::codegen(WhileStatement* stmt) {
    Function* func = builder.GetInsertBlock()->getParent();

    BasicBlock* loopCondBB = BasicBlock::Create(*context, "loopcond", func);
    BasicBlock* loopBodyBB = BasicBlock::Create(*context, "loopbody");
    BasicBlock* afterLoopBB = BasicBlock::Create(*context, "afterloop");

    builder.CreateBr(loopCondBB);

    builder.SetInsertPoint(loopCondBB);
    Value* condVal = stmt->condition->codegen(*this);
    if (!condVal) return nullptr;

    condVal = builder.CreateICmpNE(
        condVal,
        ConstantInt::get(condVal->getType(), 0),
        "loopcond");

    builder.CreateCondBr(condVal, loopBodyBB, afterLoopBB);

    func->insert(func->end(), loopBodyBB);
    builder.SetInsertPoint(loopBodyBB);

    pushLoop(afterLoopBB, loopCondBB);
    codegen(stmt->body.get());
    popLoop();

    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(loopCondBB);
    }

    func->insert(func->end(), afterLoopBB);
    builder.SetInsertPoint(afterLoopBB);

    return nullptr;
}

Value* CodeGenContext::codegen(ForStatement* stmt) {
    pushScope(); // Loop variable scope

    if (stmt->init) {
        codegen(stmt->init.get());
    }

    Function* func = builder.GetInsertBlock()->getParent();

    BasicBlock* condBB = BasicBlock::Create(*context, "for.cond", func);
    BasicBlock* bodyBB = BasicBlock::Create(*context, "for.body");
    BasicBlock* incBB = BasicBlock::Create(*context, "for.inc");
    BasicBlock* afterBB = BasicBlock::Create(*context, "for.after");

    builder.CreateBr(condBB);

    // Condition
    builder.SetInsertPoint(condBB);
    Value* condVal = nullptr;
    if (stmt->condition) {
        condVal = stmt->condition->codegen(*this);
        if (!condVal) {
            popScope();
            return nullptr;
        }
        condVal = builder.CreateICmpNE(
            condVal,
            ConstantInt::get(condVal->getType(), 0),
            "for.condval");
    } else {
        condVal = ConstantInt::get(Type::getInt1Ty(*context), 1);
    }
    builder.CreateCondBr(condVal, bodyBB, afterBB);

    // Body
    func->insert(func->end(), bodyBB);
    builder.SetInsertPoint(bodyBB);

    pushLoop(afterBB, incBB);
    if (stmt->body) {
        codegen(stmt->body.get());
    }
    popLoop();

    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(incBB);
    }

    // Increment (continue target)
    func->insert(func->end(), incBB);
    builder.SetInsertPoint(incBB);
    if (stmt->increment) {
        codegen(stmt->increment.get());
    }
    if (!builder.GetInsertBlock()->getTerminator()) {
        builder.CreateBr(condBB);
    }

    // After loop (break target)
    func->insert(func->end(), afterBB);
    builder.SetInsertPoint(afterBB);

    popScope();
    return nullptr;
}

Value* CodeGenContext::codegen(Block* stmt) {
    pushScope();
    Value* last = nullptr;
    for (const auto& s : stmt->statements) {
        last = codegen(s.get());
        if (builder.GetInsertBlock()->getTerminator()) {
            break;
        }
    }
    popScope();
    return last ? last : ConstantInt::get(Type::getInt32Ty(*context), 0);
}

Value* CodeGenContext::codegen(ExprStatement* stmt) {
    return stmt->expr ? stmt->expr->codegen(*this) : nullptr;
}

Value* CodeGenContext::codegen(BreakStatement* stmt) {
    (void)stmt;
    BasicBlock* breakBB = getCurrentLoopEnd();
    assert(breakBB && "Semantic error leaked into codegen: 'break' statement not inside loop");
    return builder.CreateBr(breakBB);
}

Value* CodeGenContext::codegen(ContinueStatement* stmt) {
    (void)stmt;
    BasicBlock* contBB = getCurrentLoopContinue();
    assert(contBB && "Semantic error leaked into codegen: 'continue' statement not inside loop");
    return builder.CreateBr(contBB);
}

void CodeGenContext::initLLVMTargets() {
    static bool initialized = false;
    if (!initialized) {
        InitializeNativeTarget();
        InitializeNativeTargetAsmPrinter();
        InitializeNativeTargetAsmParser();
        initialized = true;
    }
}

bool CodeGenContext::optimizeModule(int optLevel, bool verbose) {
    if (optLevel <= 0) {
        if (verbose) {
            std::cout << "[MiniLang] Optimization level -O0: Skipping optimization passes.\n";
        }
        return true;
    }

    if (verbose) {
        std::cout << "[MiniLang] Running optimization pipeline at -O" << optLevel << "...\n";
    }

    initLLVMTargets();

    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;

    PassBuilder PB;

    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    OptLevelType level;
    switch (optLevel) {
        case 1: level = OptLevelType::O1; break;
        case 2: level = OptLevelType::O2; break;
        case 3: default: level = OptLevelType::O3; break;
    }

    ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(level);
    MPM.run(*module, MAM);

    if (verbose) {
        std::cout << "[MiniLang] Optimization pipeline completed successfully.\n";
    }
    return true;
}

bool CodeGenContext::emitLLVMIR(const std::string& outputPath) {
    if (outputPath.empty() || outputPath == "-") {
        module->print(outs(), nullptr);
        return true;
    }

    std::error_code EC;
    raw_fd_ostream dest(outputPath, EC, sys::fs::OF_None);
    if (EC) {
        std::cerr << "Error opening output file '" << outputPath << "': " << EC.message() << "\n";
        return false;
    }
    module->print(dest, nullptr);
    dest.flush();
    return true;
}

bool CodeGenContext::emitBitcode(const std::string& outputPath) {
    if (outputPath.empty() || outputPath == "-") {
        WriteBitcodeToFile(*module, outs());
        return true;
    }

    std::error_code EC;
    raw_fd_ostream dest(outputPath, EC, sys::fs::OF_None);
    if (EC) {
        std::cerr << "Error opening output file '" << outputPath << "': " << EC.message() << "\n";
        return false;
    }
    WriteBitcodeToFile(*module, dest);
    dest.flush();
    return true;
}

static bool emitTargetOutput(Module* module, const std::string& outputPath, CodeGenFileType fileType) {
    CodeGenContext::initLLVMTargets();

    auto targetTriple = sys::getDefaultTargetTriple();
    module->setTargetTriple(targetTriple);

    std::string error;
    const Target* target = TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        std::cerr << "Error looking up target for triple " << targetTriple << ": " << error << "\n";
        return false;
    }

    std::string cpu = sys::getHostCPUName().str();
    if (cpu.empty()) cpu = "generic";
    std::string features = "";

    TargetOptions opt;
    auto rm = Reloc::Model::PIC_;
    std::unique_ptr<TargetMachine> targetMachine(
        target->createTargetMachine(targetTriple, cpu, features, opt, rm)
    );
    if (!targetMachine) {
        std::cerr << "Could not create TargetMachine for " << targetTriple << "\n";
        return false;
    }

    module->setDataLayout(targetMachine->createDataLayout());

    std::error_code EC;
    raw_fd_ostream dest(outputPath, EC, sys::fs::OF_None);
    if (EC) {
        std::cerr << "Error opening output file '" << outputPath << "': " << EC.message() << "\n";
        return false;
    }

    legacy::PassManager pass;
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
        std::cerr << "TargetMachine cannot emit a file of this type.\n";
        return false;
    }

    pass.run(*module);
    dest.flush();
    return true;
}

bool CodeGenContext::emitAssembly(const std::string& outputPath) {
    return emitTargetOutput(module.get(), outputPath, CodeGenFileType::AssemblyFile);
}

bool CodeGenContext::emitObjectFile(const std::string& outputPath) {
    return emitTargetOutput(module.get(), outputPath, CodeGenFileType::ObjectFile);
}