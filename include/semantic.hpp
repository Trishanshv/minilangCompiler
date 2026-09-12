#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <iostream>

// Forward declarations of AST nodes
struct ASTNode;
class Program;
class Statement;
class Expression;
class IntegerLiteral;
class StringLiteral;
class VariableExpr;
class UnaryExpr;
class BinaryExpr;
class ComparisonExpr;
class LogicalExpr;
class FunctionCall;
class ReturnStatement;
class VarDeclaration;
class Assignment;
class Block;
class IfStatement;
class WhileStatement;
class ForStatement;
class BreakStatement;
class ContinueStatement;
class ExprStatement;
class FunctionDef;

enum class DataType {
    INT,
    STRING,
    VOID,
    BOOL,
    UNKNOWN
};

std::string dataTypeToString(DataType type);
DataType stringToDataType(const std::string& str);

enum class SymbolType {
    VARIABLE,
    FUNCTION
};

struct Symbol {
    SymbolType type;
    DataType dataType = DataType::INT;
};

struct VariableSymbol {
    std::string name;
    DataType type = DataType::INT;
    bool isInitialized = false;
    bool isUsed = false;
    int line = 1;
    int col = 1;
};

using BuiltinValidator = std::function<bool(const std::vector<DataType>&, std::string&)>;

struct FunctionSymbol {
    std::string name;
    DataType returnType = DataType::INT;
    std::vector<DataType> paramTypes;
    bool isBuiltin = false;
    BuiltinValidator validator = nullptr;
    int line = 1;
    int col = 1;
};

class SymbolTable {
private:
    std::vector<std::unordered_map<std::string, VariableSymbol>> variableScopes;
    std::unordered_map<std::string, FunctionSymbol> functionTable;

public:
    SymbolTable();

    // Scope management
    void enterScope();
    void exitScope();
    size_t getScopeDepth() const;
    const std::unordered_map<std::string, VariableSymbol>& getCurrentScopeVariables() const;

    // Variable management
    bool declareVariable(const std::string& name, DataType type, bool isInitialized = false, int line = 1, int col = 1);
    bool isVariableDeclared(const std::string& name) const;
    VariableSymbol* lookupVariable(const std::string& name);
    const VariableSymbol* lookupVariable(const std::string& name) const;

    // Function management
    bool declareFunction(const std::string& name, DataType returnType, const std::vector<DataType>& paramTypes, bool isBuiltin = false, BuiltinValidator validator = nullptr, int line = 1, int col = 1);
    bool isFunctionDeclared(const std::string& name) const;
    FunctionSymbol* lookupFunction(const std::string& name);
    const FunctionSymbol* lookupFunction(const std::string& name) const;

    // Backward compatibility helpers
    bool declare(const std::string& name, SymbolType type);
    bool isDeclared(const std::string& name) const;
    const Symbol* lookup(const std::string& name) const;
};

class SemanticAnalyzer {
private:
    SymbolTable symbolTable;
    int loopDepth = 0;
    DataType currentFunctionReturnType = DataType::INT;
    bool insideFunction = false;

    // Flow-sensitive definite-assignment analysis
    std::unordered_set<std::string> definitelyAssigned;

    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    void addError(const ASTNode* node, const std::string& message);
    void addWarning(const ASTNode* node, const std::string& message);
    bool returnsOnAllPaths(Statement* stmt);
    void checkUnusedVariablesInCurrentScope();

public:
    SemanticAnalyzer();

    bool analyze(Program* prog);

    // Statement analysis
    void analyzeStmt(Statement* stmt);
    void analyzeVarDecl(VarDeclaration* stmt);
    void analyzeAssignment(Assignment* stmt);
    void analyzeBlock(Block* stmt);
    void analyzeIf(IfStatement* stmt);
    void analyzeWhile(WhileStatement* stmt);
    void analyzeFor(ForStatement* stmt);
    void analyzeReturn(ReturnStatement* stmt);
    void analyzeBreak(BreakStatement* stmt);
    void analyzeContinue(ContinueStatement* stmt);
    void analyzeExprStmt(ExprStatement* stmt);
    void analyzeFunctionDef(FunctionDef* stmt);

    // Expression analysis & type resolution
    DataType analyzeExpr(Expression* expr);
    DataType analyzeIntegerLiteral(IntegerLiteral* expr);
    DataType analyzeStringLiteral(StringLiteral* expr);
    DataType analyzeVariableExpr(VariableExpr* expr);
    DataType analyzeUnaryExpr(UnaryExpr* expr);
    DataType analyzeBinaryExpr(BinaryExpr* expr);
    DataType analyzeComparisonExpr(ComparisonExpr* expr);
    DataType analyzeLogicalExpr(LogicalExpr* expr);
    DataType analyzeFunctionCall(FunctionCall* expr);

    // Diagnostic reporting
    bool hasErrors() const { return !errors.empty(); }
    size_t getErrorCount() const { return errors.size(); }
    size_t getWarningCount() const { return warnings.size(); }
    const std::vector<std::string>& getErrors() const { return errors; }
    const std::vector<std::string>& getWarnings() const { return warnings; }
    void printDiagnostics() const;
};
