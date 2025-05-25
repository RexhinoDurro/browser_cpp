// js_interpreter.h - Interprets and executes JavaScript AST
#ifndef CUSTOM_JS_INTERPRETER_H
#define CUSTOM_JS_INTERPRETER_H

#include "js_parser.h"
#include "js_value.h"
#include <map>
#include <memory>
#include <stack>
#include <stdexcept>

namespace browser {
namespace custom_js {

// Runtime error
class RuntimeError : public std::runtime_error {
public:
    RuntimeError(const std::string& message) : std::runtime_error(message) {}
};

// Environment for variable scope
class Environment {
public:
    Environment();
    Environment(std::shared_ptr<Environment> enclosing);
    
    // Define a new variable
    void define(const std::string& name, const JSValue& value);
    
    // Get a variable
    JSValue get(const std::string& name);
    
    // Assign a value to an existing variable
    void assign(const std::string& name, const JSValue& value);
    
    // Check if a variable exists
    bool exists(const std::string& name) const;
    
    // Get enclosing environment
    std::shared_ptr<Environment> enclosing() const { return m_enclosing; }
    
private:
    std::map<std::string, JSValue> m_values;
    std::shared_ptr<Environment> m_enclosing;
};

// JavaScript interpreter
class JSInterpreter {
public:
    JSInterpreter();
    
    // Execute a program
    JSValue execute(std::shared_ptr<Program> program);
    
    // Execute a source string
    JSValue execute(const std::string& source);
    
    // Create a global function
    void defineGlobalFunction(const std::string& name, JSFunction::NativeFunction func);
    
    // Define a global variable
    void defineGlobalVariable(const std::string& name, const JSValue& value);
    
    // Get the global environment
    std::shared_ptr<Environment> global() const { return m_globals; }
    
private:
    std::shared_ptr<Environment> m_globals;
    std::shared_ptr<Environment> m_environment;
    
    // Return value for return statements
    bool m_hasReturnValue;
    JSValue m_returnValue;
    
    // Node visitors
    JSValue visitExpressionNode(std::shared_ptr<ExpressionNode> node);
    JSValue visitLiteralExpr(std::shared_ptr<LiteralExpr> expr);
    JSValue visitVariableExpr(std::shared_ptr<VariableExpr> expr);
    JSValue visitUnaryExpr(std::shared_ptr<UnaryExpr> expr);
    JSValue visitBinaryExpr(std::shared_ptr<BinaryExpr> expr);
    JSValue visitCallExpr(std::shared_ptr<CallExpr> expr);
    JSValue visitAssignExpr(std::shared_ptr<AssignExpr> expr);
    JSValue visitObjectExpr(std::shared_ptr<ObjectExpr> expr);
    JSValue visitArrayExpr(std::shared_ptr<ArrayExpr> expr);
    JSValue visitMemberExpr(std::shared_ptr<MemberExpr> expr);
    
    void executeStatementNode(std::shared_ptr<StatementNode> node);
    void visitExpressionStmt(std::shared_ptr<ExpressionStmt> stmt);
    void visitVariableStmt(std::shared_ptr<VariableStmt> stmt);
    void visitBlockStmt(std::shared_ptr<BlockStmt> stmt);
    void visitIfStmt(std::shared_ptr<IfStmt> stmt);
    void visitWhileStmt(std::shared_ptr<WhileStmt> stmt);
    void visitForStmt(std::shared_ptr<ForStmt> stmt);
    void visitFunctionStmt(std::shared_ptr<FunctionStmt> stmt);
    void visitReturnStmt(std::shared_ptr<ReturnStmt> stmt);
    
    // Helper for evaluating
    JSValue evaluate(std::shared_ptr<ExpressionNode> expr);
    void execute(std::shared_ptr<StatementNode> stmt);
    
    // Helper for creating environment scope
    void executeBlock(std::shared_ptr<BlockStmt> block, std::shared_ptr<Environment> environment);
    
    // Helper for extracting values
    bool isTruthy(const JSValue& value);
    bool isEqual(const JSValue& a, const JSValue& b);
    
    // Runtime error handling
    RuntimeError error(const std::string& message);
    
    // Helper for resolving variable references
    JSValue lookUpVariable(const std::string& name);
};

} // namespace custom_js
} // namespace browser

#endif // CUSTOM_JS_INTERPRETER_H