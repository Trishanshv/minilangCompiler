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
    // Register built-in functions
    declareFunction("print", DataType::VOID, {}, true);
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

bool SymbolTable::declareVariable(const std::string& name, DataType type, bool isInitialized) {
    if (variableScopes.back().count(name)) {
        return false; // Redeclaration in same scope
    }
    variableScopes.back()[name] = VariableSymbol{name, type, isInitialized, false};
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

bool SymbolTable::declareFunction(const std::string& name, DataType returnType, const std::vector<DataType>& paramTypes, bool isBuiltin) {
    if (functionTable.count(name)) {
        return false; // Function already declared
    }
    functionTable[name] = FunctionSymbol{name, returnType, paramTypes, isBuiltin};
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
    : loopDepth(0), currentFunctionReturnType(DataType::INT),
      currentFunctionHasReturn(false), insideFunction(false) {}

void SemanticAnalyzer::addError(const std::string& message) {
    errors.push_back("Semantic Error: " + message);
}

void SemanticAnalyzer::addWarning(const std::string& message) {
    warnings.push_back("Semantic Warning: " + message);
}

bool SemanticAnalyzer::analyze(Program* prog) {
    if (!prog) return true;

    // Pass 1: Register all function signatures
    for (const auto& stmt : prog->getStatements()) {
        if (auto* funcDef = dynamic_cast<FunctionDef*>(stmt.get())) {
            DataType retType = stringToDataType(funcDef->returnType);
            std::vector<DataType> paramTypes;
            for (const auto& p : funcDef->params) {
                paramTypes.push_back(stringToDataType(p.type));
            }

            if (!symbolTable.declareFunction(funcDef->name, retType, paramTypes)) {
                addError("Function '" + funcDef->name + "' already declared");
            }
        }
    }

    // Pass 2: Analyze all statement bodies and expressions
    for (const auto& stmt : prog->getStatements()) {
        analyzeStmt(stmt.get());
    }

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
    if (!symbolTable.declareVariable(stmt->name, DataType::INT, stmt->init != nullptr)) {
        addError("Variable '" + stmt->name + "' already declared in this scope");
        return;
    }

    if (stmt->init) {
        DataType initType = analyzeExpr(stmt->init.get());
        if (initType != DataType::INT && initType != DataType::UNKNOWN) {
            addError("Cannot initialize integer variable '" + stmt->name + "' with type " + dataTypeToString(initType));
        }
    }
}

void SemanticAnalyzer::analyzeAssignment(Assignment* stmt) {
    VariableSymbol* var = symbolTable.lookupVariable(stmt->name);
    if (!var) {
        addError("Assignment to undeclared variable '" + stmt->name + "'");
    }

    DataType exprType = analyzeExpr(stmt->expr.get());
    if (var && exprType != var->type && exprType != DataType::UNKNOWN) {
        addError("Type mismatch: cannot assign " + dataTypeToString(exprType) + " to variable '" + stmt->name + "' of type " + dataTypeToString(var->type));
    }

    if (var) {
        var->isInitialized = true;
    }
}

void SemanticAnalyzer::analyzeBlock(Block* stmt) {
    symbolTable.enterScope();
    bool terminatorEncountered = false;

    for (const auto& s : stmt->statements) {
        if (terminatorEncountered) {
            addWarning("Unreachable code detected after return, break, or continue");
            break;
        }

        analyzeStmt(s.get());

        if (dynamic_cast<ReturnStatement*>(s.get()) ||
            dynamic_cast<BreakStatement*>(s.get()) ||
            dynamic_cast<ContinueStatement*>(s.get())) {
            terminatorEncountered = true;
        }
    }

    symbolTable.exitScope();
}

void SemanticAnalyzer::analyzeIf(IfStatement* stmt) {
    DataType condType = analyzeExpr(stmt->condition.get());
    if (condType == DataType::VOID || condType == DataType::STRING) {
        addError("Condition in if-statement must be an integer or boolean, got " + dataTypeToString(condType));
    }

    analyzeStmt(stmt->thenBlock.get());
    if (stmt->elseBlock) {
        analyzeStmt(stmt->elseBlock.get());
    }
}

void SemanticAnalyzer::analyzeWhile(WhileStatement* stmt) {
    DataType condType = analyzeExpr(stmt->condition.get());
    if (condType == DataType::VOID || condType == DataType::STRING) {
        addError("Condition in while-statement must be an integer or boolean, got " + dataTypeToString(condType));
    }

    loopDepth++;
    analyzeStmt(stmt->body.get());
    loopDepth--;
}

void SemanticAnalyzer::analyzeFor(ForStatement* stmt) {
    symbolTable.enterScope();

    if (stmt->init) {
        analyzeStmt(stmt->init.get());
    }

    if (stmt->condition) {
        DataType condType = analyzeExpr(stmt->condition.get());
        if (condType == DataType::VOID || condType == DataType::STRING) {
            addError("Condition in for-statement must be an integer or boolean, got " + dataTypeToString(condType));
        }
    }

    if (stmt->increment) {
        analyzeStmt(stmt->increment.get());
    }

    loopDepth++;
    if (stmt->body) {
        analyzeStmt(stmt->body.get());
    }
    loopDepth--;

    symbolTable.exitScope();
}

void SemanticAnalyzer::analyzeReturn(ReturnStatement* stmt) {
    if (insideFunction) {
        currentFunctionHasReturn = true;
    }

    if (stmt->expr) {
        DataType retType = analyzeExpr(stmt->expr.get());
        if (insideFunction && currentFunctionReturnType == DataType::VOID) {
            addError("Cannot return a value from void function");
        } else if (insideFunction && retType != currentFunctionReturnType && retType != DataType::UNKNOWN) {
            addError("Return type mismatch: expected " + dataTypeToString(currentFunctionReturnType) + ", got " + dataTypeToString(retType));
        }
    } else {
        if (insideFunction && currentFunctionReturnType != DataType::VOID) {
            addError("Non-void function must return a value");
        }
    }
}

void SemanticAnalyzer::analyzeBreak(BreakStatement* stmt) {
    (void)stmt;
    if (loopDepth <= 0) {
        addError("'break' statement not inside a loop");
    }
}

void SemanticAnalyzer::analyzeContinue(ContinueStatement* stmt) {
    (void)stmt;
    if (loopDepth <= 0) {
        addError("'continue' statement not inside a loop");
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
    currentFunctionHasReturn = false;

    symbolTable.enterScope();

    for (const auto& p : stmt->params) {
        DataType paramType = stringToDataType(p.type);
        if (!symbolTable.declareVariable(p.name, paramType, true)) {
            addError("Duplicate parameter name '" + p.name + "' in function '" + stmt->name + "'");
        }
    }

    if (stmt->body) {
        analyzeStmt(stmt->body.get());
    }

    if (currentFunctionReturnType != DataType::VOID && !currentFunctionHasReturn && stmt->name != "main") {
        addWarning("Function '" + stmt->name + "' does not explicitly return a value");
    }

    symbolTable.exitScope();
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
        addError("Undeclared variable '" + expr->name + "'");
        return DataType::UNKNOWN;
    }

    var->isUsed = true;
    if (!var->isInitialized) {
        addWarning("Variable '" + expr->name + "' used without being initialized");
    }

    return var->type;
}

DataType SemanticAnalyzer::analyzeUnaryExpr(UnaryExpr* expr) {
    DataType t = analyzeExpr(expr->operand.get());

    if (expr->op == '-') {
        if (t != DataType::INT && t != DataType::UNKNOWN) {
            addError("Unary '-' expects integer operand, got " + dataTypeToString(t));
        }
        return DataType::INT;
    }

    if (expr->op == '!') {
        if (t != DataType::INT && t != DataType::BOOL && t != DataType::UNKNOWN) {
            addError("Logical '!' expects integer or boolean operand, got " + dataTypeToString(t));
        }
        return DataType::BOOL;
    }

    return DataType::UNKNOWN;
}

DataType SemanticAnalyzer::analyzeBinaryExpr(BinaryExpr* expr) {
    DataType l = analyzeExpr(expr->lhs.get());
    DataType r = analyzeExpr(expr->rhs.get());

    if ((l != DataType::INT && l != DataType::UNKNOWN) ||
        (r != DataType::INT && r != DataType::UNKNOWN)) {
        addError(std::string("Binary operator '") + expr->op + "' requires integer operands");
    }

    return DataType::INT;
}

DataType SemanticAnalyzer::analyzeComparisonExpr(ComparisonExpr* expr) {
    DataType l = analyzeExpr(expr->lhs.get());
    DataType r = analyzeExpr(expr->rhs.get());

    if (l != r && l != DataType::UNKNOWN && r != DataType::UNKNOWN) {
        addError("Comparison operator '" + expr->op + "' compares incompatible types (" +
                 dataTypeToString(l) + " and " + dataTypeToString(r) + ")");
    }

    return DataType::BOOL;
}

DataType SemanticAnalyzer::analyzeLogicalExpr(LogicalExpr* expr) {
    DataType l = analyzeExpr(expr->lhs.get());
    DataType r = analyzeExpr(expr->rhs.get());

    if (l == DataType::VOID || l == DataType::STRING ||
        r == DataType::VOID || r == DataType::STRING) {
        addError("Logical operator '" + expr->op + "' cannot operate on void or string");
    }

    return DataType::BOOL;
}

DataType SemanticAnalyzer::analyzeFunctionCall(FunctionCall* expr) {
    if (expr->name == "print") {
        if (!expr->args || expr->args->size() != 1) {
            addError("'print' function expects exactly 1 argument");
        } else {
            DataType argType = analyzeExpr((*expr->args)[0]);
            if (argType == DataType::VOID) {
                addError("Cannot print void expression");
            }
        }
        return DataType::VOID;
    }

    FunctionSymbol* fn = symbolTable.lookupFunction(expr->name);
    if (!fn) {
        addError("Call to undeclared function '" + expr->name + "'");
        return DataType::UNKNOWN;
    }

    size_t expected = fn->paramTypes.size();
    size_t actual = expr->args ? expr->args->size() : 0;

    if (expected != actual) {
        addError("Function '" + expr->name + "' expects " + std::to_string(expected) +
                 " argument(s), but " + std::to_string(actual) + " were provided");
    } else if (expr->args) {
        for (size_t i = 0; i < actual; ++i) {
            DataType argType = analyzeExpr((*expr->args)[i]);
            if (argType != fn->paramTypes[i] && argType != DataType::UNKNOWN) {
                addError("Argument " + std::to_string(i + 1) + " of function '" + expr->name +
                         "' expects type " + dataTypeToString(fn->paramTypes[i]) +
                         ", got " + dataTypeToString(argType));
            }
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