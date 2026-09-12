#include "semantic.hpp"
#include "ast.hpp"
#include <iostream>

std::string dataTypeToString(DataType type) {
    switch (type) {
        case DataType::INT: return "int";
        case DataType::STRING: return "string";
        case DataType::VOID: return "void";
        case DataType::BOOL: return "bool";
        case DataType::UNKNOWN: return "unknown";
    }
    return "unknown";
}

DataType stringToDataType(const std::string& str) {
    if (str == "int") return DataType::INT;
    if (str == "string") return DataType::STRING;
    if (str == "void") return DataType::VOID;
    if (str == "bool") return DataType::BOOL;
    return DataType::UNKNOWN;
}

// ==================== SymbolTable ====================

SymbolTable::SymbolTable() {
    enterScope(); // Global scope

    // Register built-in functions with custom validators
    auto printValidator = [](const std::vector<DataType>& args, std::string& err) -> bool {
        if (args.size() != 1) {
            err = "'print' function expects exactly 1 argument, got " + std::to_string(args.size());
            return false;
        }
        if (args[0] == DataType::VOID) {
            err = "Cannot print void expression";
            return false;
        }
        if (args[0] != DataType::INT && args[0] != DataType::STRING && args[0] != DataType::UNKNOWN) {
            err = "Cannot print expression of type " + dataTypeToString(args[0]);
            return false;
        }
        return true;
    };

    declareFunction("print", DataType::VOID, {}, true, printValidator);
}

void SymbolTable::enterScope() {
    variableScopes.emplace_back();
}

void SymbolTable::exitScope() {
    if (variableScopes.size() <= 1) {
        throw std::runtime_error("Cannot exit global scope");
    }
    variableScopes.pop_back();
}

size_t SymbolTable::getScopeDepth() const {
    return variableScopes.size();
}

const std::unordered_map<std::string, VariableSymbol>& SymbolTable::getCurrentScopeVariables() const {
    return variableScopes.back();
}

bool SymbolTable::declareVariable(const std::string& name, DataType type, bool isInitialized, int line, int col) {
    if (variableScopes.back().count(name)) {
        return false; // Redeclaration in same scope
    }
    variableScopes.back()[name] = VariableSymbol{name, type, isInitialized, false, line, col};
    return true;
}

bool SymbolTable::isVariableDeclared(const std::string& name) const {
    for (auto it = variableScopes.rbegin(); it != variableScopes.rend(); ++it) {
        if (it->count(name)) return true;
    }
    return false;
}

VariableSymbol* SymbolTable::lookupVariable(const std::string& name) {
    for (auto it = variableScopes.rbegin(); it != variableScopes.rend(); ++it) {
        if (auto found = it->find(name); found != it->end()) {
            return &found->second;
        }
    }
    return nullptr;
}

const VariableSymbol* SymbolTable::lookupVariable(const std::string& name) const {
    for (auto it = variableScopes.rbegin(); it != variableScopes.rend(); ++it) {
        if (auto found = it->find(name); found != it->end()) {
            return &found->second;
        }
    }
    return nullptr;
}

bool SymbolTable::declareFunction(const std::string& name, DataType returnType, const std::vector<DataType>& paramTypes, bool isBuiltin, BuiltinValidator validator, int line, int col) {
    if (functionTable.count(name)) {
        return false; // Function already declared
    }
    functionTable[name] = FunctionSymbol{name, returnType, paramTypes, isBuiltin, validator, line, col};
    return true;
}

bool SymbolTable::isFunctionDeclared(const std::string& name) const {
    return functionTable.count(name) > 0;
}

FunctionSymbol* SymbolTable::lookupFunction(const std::string& name) {
    if (auto it = functionTable.find(name); it != functionTable.end()) {
        return &it->second;
    }
    return nullptr;
}

const FunctionSymbol* SymbolTable::lookupFunction(const std::string& name) const {
    if (auto it = functionTable.find(name); it != functionTable.end()) {
        return &it->second;
    }
    return nullptr;
}

// Backward compatibility helpers
bool SymbolTable::declare(const std::string& name, SymbolType type) {
    if (type == SymbolType::VARIABLE) {
        return declareVariable(name, DataType::INT, true);
    }
    return declareFunction(name, DataType::INT, {});
}

bool SymbolTable::isDeclared(const std::string& name) const {
    return isVariableDeclared(name) || isFunctionDeclared(name);
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
    static Symbol temp;
    if (isVariableDeclared(name)) {
        temp.type = SymbolType::VARIABLE;
        temp.dataType = DataType::INT;
        return &temp;
    }
    if (isFunctionDeclared(name)) {
        temp.type = SymbolType::FUNCTION;
        temp.dataType = DataType::INT;
        return &temp;
    }
    return nullptr;
}

// ==================== SemanticAnalyzer ====================

SemanticAnalyzer::SemanticAnalyzer()
    : loopDepth(0), currentFunctionReturnType(DataType::INT), insideFunction(false) {}

void SemanticAnalyzer::addError(const ASTNode* node, const std::string& message) {
    int line = node ? node->line : 1;
    int col = node ? node->col : 1;
    errors.push_back("[Line " + std::to_string(line) + ", Col " + std::to_string(col) + "] Semantic Error: " + message);
}

void SemanticAnalyzer::addWarning(const ASTNode* node, const std::string& message) {
    int line = node ? node->line : 1;
    int col = node ? node->col : 1;
    warnings.push_back("[Line " + std::to_string(line) + ", Col " + std::to_string(col) + "] Semantic Warning: " + message);
}

bool SemanticAnalyzer::returnsOnAllPaths(Statement* stmt) {
    if (!stmt) return false;
    if (dynamic_cast<ReturnStatement*>(stmt)) return true;
    if (auto* block = dynamic_cast<Block*>(stmt)) {
        for (const auto& s : block->statements) {
            if (returnsOnAllPaths(s.get())) return true;
        }
        return false;
    }
    if (auto* ifStmt = dynamic_cast<IfStatement*>(stmt)) {
        if (!ifStmt->elseBlock) return false;
        return returnsOnAllPaths(ifStmt->thenBlock.get()) && returnsOnAllPaths(ifStmt->elseBlock.get());
    }
    return false;
}

void SemanticAnalyzer::checkUnusedVariablesInCurrentScope() {
    for (const auto& [name, var] : symbolTable.getCurrentScopeVariables()) {
        if (!var.isUsed) {
            warnings.push_back("[Line " + std::to_string(var.line) + ", Col " + std::to_string(var.col) + "] Semantic Warning: Unused variable '" + name + "'");
        }
    }
}

bool SemanticAnalyzer::analyze(Program* prog) {
    if (!prog) return true;

    // Pass 1: Gather and declare function signatures
    for (const auto& stmt : prog->getStatements()) {
        if (auto* funcDef = dynamic_cast<FunctionDef*>(stmt.get())) {
            DataType retType = stringToDataType(funcDef->returnType);
            std::vector<DataType> paramTypes;
            for (const auto& p : funcDef->params) {
                paramTypes.push_back(stringToDataType(p.type));
            }

            if (!symbolTable.declareFunction(funcDef->name, retType, paramTypes, false, nullptr, funcDef->line, funcDef->col)) {
                addError(funcDef, "Function '" + funcDef->name + "' already declared");
            }
        }
    }

    // Pass 2: Analyze statements and function bodies
    for (const auto& stmt : prog->getStatements()) {
        analyzeStmt(stmt.get());
    }

    // Check for unused global variables
    checkUnusedVariablesInCurrentScope();

    return !hasErrors();
}

void SemanticAnalyzer::analyzeStmt(Statement* stmt) {
    if (!stmt) return;

    if (auto* varDecl = dynamic_cast<VarDeclaration*>(stmt)) {
        analyzeVarDecl(varDecl);
    } else if (auto* assign = dynamic_cast<Assignment*>(stmt)) {
        analyzeAssignment(assign);
    } else if (auto* block = dynamic_cast<Block*>(stmt)) {
        analyzeBlock(block);
    } else if (auto* ifStmt = dynamic_cast<IfStatement*>(stmt)) {
        analyzeIf(ifStmt);
    } else if (auto* whileStmt = dynamic_cast<WhileStatement*>(stmt)) {
        analyzeWhile(whileStmt);
    } else if (auto* forStmt = dynamic_cast<ForStatement*>(stmt)) {
        analyzeFor(forStmt);
    } else if (auto* retStmt = dynamic_cast<ReturnStatement*>(stmt)) {
        analyzeReturn(retStmt);
    } else if (auto* breakStmt = dynamic_cast<BreakStatement*>(stmt)) {
        analyzeBreak(breakStmt);
    } else if (auto* contStmt = dynamic_cast<ContinueStatement*>(stmt)) {
        analyzeContinue(contStmt);
    } else if (auto* exprStmt = dynamic_cast<ExprStatement*>(stmt)) {
        analyzeExprStmt(exprStmt);
    } else if (auto* funcDef = dynamic_cast<FunctionDef*>(stmt)) {
        analyzeFunctionDef(funcDef);
    }
}

void SemanticAnalyzer::analyzeVarDecl(VarDeclaration* stmt) {
    if (!symbolTable.declareVariable(stmt->name, DataType::INT, stmt->init != nullptr, stmt->line, stmt->col)) {
        addError(stmt, "Variable '" + stmt->name + "' already declared in this scope");
        return;
    }

    if (stmt->init) {
        DataType initType = analyzeExpr(stmt->init.get());
        if (initType != DataType::INT && initType != DataType::UNKNOWN) {
            addError(stmt, "Cannot initialize integer variable '" + stmt->name + "' with type " + dataTypeToString(initType));
        }
        definitelyAssigned.insert(stmt->name);
    }
}

void SemanticAnalyzer::analyzeAssignment(Assignment* stmt) {
    VariableSymbol* var = symbolTable.lookupVariable(stmt->name);
    if (!var) {
        addError(stmt, "Assignment to undeclared variable '" + stmt->name + "'");
    }

    DataType exprType = analyzeExpr(stmt->expr.get());
    if (var && exprType != var->type && exprType != DataType::UNKNOWN) {
        addError(stmt, "Type mismatch: cannot assign " + dataTypeToString(exprType) + " to variable '" + stmt->name + "' of type " + dataTypeToString(var->type));
    }

    if (var) {
        definitelyAssigned.insert(stmt->name);
    }
}

void SemanticAnalyzer::analyzeBlock(Block* stmt) {
    symbolTable.enterScope();
    bool terminatorEncountered = false;

    for (const auto& s : stmt->statements) {
        if (terminatorEncountered) {
            addWarning(s.get(), "Unreachable code detected after return, break, or continue");
            break;
        }

        analyzeStmt(s.get());

        if (dynamic_cast<ReturnStatement*>(s.get()) ||
            dynamic_cast<BreakStatement*>(s.get()) ||
            dynamic_cast<ContinueStatement*>(s.get())) {
            terminatorEncountered = true;
        }
    }

    checkUnusedVariablesInCurrentScope();
    symbolTable.exitScope();
}

void SemanticAnalyzer::analyzeIf(IfStatement* stmt) {
    DataType condType = analyzeExpr(stmt->condition.get());
    if (condType != DataType::INT && condType != DataType::BOOL && condType != DataType::UNKNOWN) {
        addError(stmt->condition.get(), "Condition in if-statement must be an integer or boolean, got " + dataTypeToString(condType));
    }

    auto before = definitelyAssigned;

    analyzeStmt(stmt->thenBlock.get());
    auto afterThen = definitelyAssigned;

    if (stmt->elseBlock) {
        definitelyAssigned = before;
        analyzeStmt(stmt->elseBlock.get());
        auto afterElse = definitelyAssigned;

        // Merge join point: intersection of then and else branches
        definitelyAssigned = before;
        for (const auto& var : afterThen) {
            if (afterElse.count(var)) {
                definitelyAssigned.insert(var);
            }
        }
    } else {
        // Without an else branch, then branch is not guaranteed to run
        definitelyAssigned = before;
    }
}

void SemanticAnalyzer::analyzeWhile(WhileStatement* stmt) {
    DataType condType = analyzeExpr(stmt->condition.get());
    if (condType != DataType::INT && condType != DataType::BOOL && condType != DataType::UNKNOWN) {
        addError(stmt->condition.get(), "Condition in while-statement must be an integer or boolean, got " + dataTypeToString(condType));
    }

    auto before = definitelyAssigned;
    loopDepth++;
    analyzeStmt(stmt->body.get());
    loopDepth--;
    // Body might execute 0 times
    definitelyAssigned = before;
}

void SemanticAnalyzer::analyzeFor(ForStatement* stmt) {
    symbolTable.enterScope();

    if (stmt->init) {
        analyzeStmt(stmt->init.get());
    }

    if (stmt->condition) {
        DataType condType = analyzeExpr(stmt->condition.get());
        if (condType != DataType::INT && condType != DataType::BOOL && condType != DataType::UNKNOWN) {
            addError(stmt->condition.get(), "Condition in for-statement must be an integer or boolean, got " + dataTypeToString(condType));
        }
    }

    if (stmt->increment) {
        analyzeStmt(stmt->increment.get());
    }

    auto before = definitelyAssigned;
    loopDepth++;
    if (stmt->body) {
        analyzeStmt(stmt->body.get());
    }
    loopDepth--;
    // Body might execute 0 times
    definitelyAssigned = before;

    checkUnusedVariablesInCurrentScope();
    symbolTable.exitScope();
}

void SemanticAnalyzer::analyzeReturn(ReturnStatement* stmt) {
    if (stmt->expr) {
        DataType retType = analyzeExpr(stmt->expr.get());
        if (insideFunction && currentFunctionReturnType == DataType::VOID) {
            addError(stmt, "Cannot return a value from void function");
        } else if (insideFunction && retType != currentFunctionReturnType && retType != DataType::UNKNOWN) {
            addError(stmt, "Return type mismatch: expected " + dataTypeToString(currentFunctionReturnType) + ", got " + dataTypeToString(retType));
        }
    } else {
        if (insideFunction && currentFunctionReturnType != DataType::VOID) {
            addError(stmt, "Non-void function must return a value of type " + dataTypeToString(currentFunctionReturnType));
        }
    }
}

void SemanticAnalyzer::analyzeBreak(BreakStatement* stmt) {
    if (loopDepth <= 0) {
        addError(stmt, "'break' statement not inside a loop");
    }
}

void SemanticAnalyzer::analyzeContinue(ContinueStatement* stmt) {
    if (loopDepth <= 0) {
        addError(stmt, "'continue' statement not inside a loop");
    }
}

void SemanticAnalyzer::analyzeExprStmt(ExprStatement* stmt) {
    if (stmt->expr) {
        analyzeExpr(stmt->expr.get());
    }
}

void SemanticAnalyzer::analyzeFunctionDef(FunctionDef* stmt) {
    insideFunction = true;
    currentFunctionReturnType = stringToDataType(stmt->returnType);

    symbolTable.enterScope();
    auto savedAssigned = definitelyAssigned;
    definitelyAssigned.clear();

    for (const auto& p : stmt->params) {
        DataType paramType = stringToDataType(p.type);
        if (!symbolTable.declareVariable(p.name, paramType, true, stmt->line, stmt->col)) {
            addError(stmt, "Duplicate parameter name '" + p.name + "' in function '" + stmt->name + "'");
        } else {
            definitelyAssigned.insert(p.name);
        }
    }

    if (stmt->body) {
        analyzeStmt(stmt->body.get());
    }

    // Missing-return check
    if (currentFunctionReturnType != DataType::VOID && stmt->name != "main") {
        if (!returnsOnAllPaths(stmt->body.get())) {
            addError(stmt, "Control reaches end of non-void function '" + stmt->name + "' without returning a value");
        }
    }

    checkUnusedVariablesInCurrentScope();
    symbolTable.exitScope();
    definitelyAssigned = savedAssigned;
    insideFunction = false;
}

DataType SemanticAnalyzer::analyzeExpr(Expression* expr) {
    if (!expr) return DataType::VOID;

    if (auto* intLit = dynamic_cast<IntegerLiteral*>(expr)) {
        return analyzeIntegerLiteral(intLit);
    }
    if (auto* strLit = dynamic_cast<StringLiteral*>(expr)) {
        return analyzeStringLiteral(strLit);
    }
    if (auto* varExpr = dynamic_cast<VariableExpr*>(expr)) {
        return analyzeVariableExpr(varExpr);
    }
    if (auto* unary = dynamic_cast<UnaryExpr*>(expr)) {
        return analyzeUnaryExpr(unary);
    }
    if (auto* bin = dynamic_cast<BinaryExpr*>(expr)) {
        return analyzeBinaryExpr(bin);
    }
    if (auto* cmp = dynamic_cast<ComparisonExpr*>(expr)) {
        return analyzeComparisonExpr(cmp);
    }
    if (auto* log = dynamic_cast<LogicalExpr*>(expr)) {
        return analyzeLogicalExpr(log);
    }
    if (auto* call = dynamic_cast<FunctionCall*>(expr)) {
        return analyzeFunctionCall(call);
    }

    return DataType::UNKNOWN;
}

DataType SemanticAnalyzer::analyzeIntegerLiteral(IntegerLiteral* expr) {
    (void)expr;
    return DataType::INT;
}

DataType SemanticAnalyzer::analyzeStringLiteral(StringLiteral* expr) {
    (void)expr;
    return DataType::STRING;
}

DataType SemanticAnalyzer::analyzeVariableExpr(VariableExpr* expr) {
    VariableSymbol* var = symbolTable.lookupVariable(expr->name);
    if (!var) {
        addError(expr, "Undeclared variable '" + expr->name + "'");
        return DataType::UNKNOWN;
    }

    var->isUsed = true;
    if (definitelyAssigned.find(expr->name) == definitelyAssigned.end()) {
        addWarning(expr, "Variable '" + expr->name + "' used before being initialized");
    }

    return var->type;
}

DataType SemanticAnalyzer::analyzeUnaryExpr(UnaryExpr* expr) {
    DataType t = analyzeExpr(expr->operand.get());
    if (t == DataType::UNKNOWN) return DataType::UNKNOWN;

    if (expr->op == '-') {
        if (t != DataType::INT) {
            addError(expr, "Unary '-' expects integer operand, got " + dataTypeToString(t));
            return DataType::UNKNOWN;
        }
        return DataType::INT;
    }

    if (expr->op == '!') {
        if (t != DataType::INT && t != DataType::BOOL) {
            addError(expr, "Logical '!' expects integer or boolean operand, got " + dataTypeToString(t));
            return DataType::UNKNOWN;
        }
        return DataType::BOOL;
    }

    return DataType::UNKNOWN;
}

DataType SemanticAnalyzer::analyzeBinaryExpr(BinaryExpr* expr) {
    DataType l = analyzeExpr(expr->lhs.get());
    DataType r = analyzeExpr(expr->rhs.get());

    if (l == DataType::UNKNOWN || r == DataType::UNKNOWN) {
        return DataType::UNKNOWN;
    }

    if (l != DataType::INT || r != DataType::INT) {
        addError(expr, std::string("Binary operator '") + expr->op + "' requires integer operands, got " + dataTypeToString(l) + " and " + dataTypeToString(r));
        return DataType::UNKNOWN;
    }

    return DataType::INT;
}

DataType SemanticAnalyzer::analyzeComparisonExpr(ComparisonExpr* expr) {
    DataType l = analyzeExpr(expr->lhs.get());
    DataType r = analyzeExpr(expr->rhs.get());

    if (l == DataType::UNKNOWN || r == DataType::UNKNOWN) {
        return DataType::UNKNOWN;
    }

    if (l != r) {
        addError(expr, "Comparison operator '" + expr->op + "' compares incompatible types (" +
                 dataTypeToString(l) + " and " + dataTypeToString(r) + ")");
        return DataType::UNKNOWN;
    }

    return DataType::BOOL;
}

DataType SemanticAnalyzer::analyzeLogicalExpr(LogicalExpr* expr) {
    DataType l = analyzeExpr(expr->lhs.get());
    DataType r = analyzeExpr(expr->rhs.get());

    if (l == DataType::UNKNOWN || r == DataType::UNKNOWN) {
        return DataType::UNKNOWN;
    }

    if (l == DataType::VOID || l == DataType::STRING ||
        r == DataType::VOID || r == DataType::STRING) {
        addError(expr, "Logical operator '" + expr->op + "' cannot operate on void or string");
        return DataType::UNKNOWN;
    }

    return DataType::BOOL;
}

DataType SemanticAnalyzer::analyzeFunctionCall(FunctionCall* expr) {
    FunctionSymbol* fn = symbolTable.lookupFunction(expr->name);
    if (!fn) {
        addError(expr, "Call to undeclared function '" + expr->name + "'");
        return DataType::UNKNOWN;
    }

    // Collect argument types
    std::vector<DataType> argTypes;
    bool hasUnknownArg = false;
    if (expr->args) {
        for (auto* a : *expr->args) {
            DataType t = analyzeExpr(a);
            if (t == DataType::UNKNOWN) hasUnknownArg = true;
            argTypes.push_back(t);
        }
    }

    // Custom builtin validation path (e.g. for print)
    if (fn->validator) {
        std::string err;
        if (!fn->validator(argTypes, err)) {
            addError(expr, err);
            return DataType::UNKNOWN;
        }
        return fn->returnType;
    }

    if (hasUnknownArg) {
        return fn->returnType;
    }

    size_t expected = fn->paramTypes.size();
    size_t actual = argTypes.size();

    if (expected != actual) {
        addError(expr, "Function '" + expr->name + "' expects " + std::to_string(expected) +
                 " argument(s), but " + std::to_string(actual) + " were provided");
        return fn->returnType;
    }

    for (size_t i = 0; i < actual; ++i) {
        if (argTypes[i] != fn->paramTypes[i]) {
            addError(expr, "Argument " + std::to_string(i + 1) + " of function '" + expr->name +
                     "' expects type " + dataTypeToString(fn->paramTypes[i]) +
                     ", got " + dataTypeToString(argTypes[i]));
        }
    }

    return fn->returnType;
}

void SemanticAnalyzer::printDiagnostics() const {
    for (const auto& w : warnings) {
        std::cerr << w << std::endl;
    }
    for (const auto& e : errors) {
        std::cerr << e << std::endl;
    }
}