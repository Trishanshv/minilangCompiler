#include "codegen.hpp"

using namespace llvm;

CodeGenContext::CodeGenContext()
    : builder(llvmContext)
{
    module = std::make_unique<Module>("main", llvmContext);
}

Function* CodeGenContext::generateCode(Program* prog) {
    // Define int main() function
    FunctionType* funcType = FunctionType::get(builder.getInt32Ty(), false);
    Function* mainFunc = Function::Create(funcType, Function::ExternalLinkage, "main", module.get());

    // Entry basic block
    BasicBlock* entry = BasicBlock::Create(llvmContext, "entry", mainFunc);
    builder.SetInsertPoint(entry);

    for (const auto& stmt : *prog->stmts) {
        codegen(stmt.get());
    }

    if (builder.GetInsertBlock()->getTerminator() == nullptr) {
        builder.CreateRet(ConstantInt::get(Type::getInt32Ty(llvmContext), 0));
    }

    return mainFunc;
}

Value* CodeGenContext::codegen(Expression* expr) {
    if (auto intLit = dynamic_cast<IntegerLiteral*>(expr)) {
        return ConstantInt::get(Type::getInt32Ty(llvmContext), intLit->value);
    }

    if (auto bin = dynamic_cast<BinaryExpr*>(expr)) {
        Value* L = codegen(bin->lhs.get());
        Value* R = codegen(bin->rhs.get());

        switch (bin->op) {
            case '+': return builder.CreateAdd(L, R, "addtmp");
            case '-': return builder.CreateSub(L, R, "subtmp");
            case '*': return builder.CreateMul(L, R, "multmp");
            case '/': return builder.CreateSDiv(L, R, "divtmp");
            case '<': return builder.CreateICmpSLT(L, R, "cmptmp");
            case '>': return builder.CreateICmpSGT(L, R, "cmptmp");
            case '=': return builder.CreateICmpEQ(L, R, "cmptmp");
            default: return nullptr;
        }
    }

    return nullptr;
}

Value* CodeGenContext::codegen(Statement* stmt) {
    if (auto ret = dynamic_cast<ReturnStatement*>(stmt)) {
        Value* val = codegen(ret->expr.get());
        return builder.CreateRet(val);
    }

    return nullptr;
}
