// js_parser.h - Parses JavaScript tokens into an AST
#ifndef CUSTOM_JS_PARSER_H
#define CUSTOM_JS_PARSER_H

#include "js_lexer.h"
#include <memory>
#include <vector>
#include <string>
#include <stdexcept>

namespace browser {
namespace custom_js {

// Forward declarations for AST nodes
class ExpressionNode;
class StatementNode;

// Parser exception
class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& message) : std::runtime_error(message) {}
};

// AST base node
class ASTNode {
public:
    virtual ~ASTNode() = default;
};

// Expression node types
enum class ExpressionType {
    LITERAL,
    VARIABLE,
    UNARY,
    BINARY,
    CALL,
    ASSIGN,
    OBJECT,
    ARRAY,
    MEMBER
};

// Expression node
class ExpressionNode : public ASTNode {
public:
    virtual ExpressionType getType() const = 0;
};

// Literal expression
class LiteralExpr : public ExpressionNode {
public:
    enum class LiteralType {
        NUMBER,
        STRING,
        BOOLEAN,
        NULL_TYPE,
        UNDEFINED
    };
    
    LiteralExpr(LiteralType type, const std::string& value);
    ExpressionType getType() const override { return ExpressionType::LITERAL; }
    
    LiteralType literalType;
    std::string value;
};

// Variable reference expression
class VariableExpr : public ExpressionNode {
public:
    VariableExpr(const std::string& name);
    ExpressionType getType() const override { return ExpressionType::VARIABLE; }
    
    std::string name;
};

// Unary expression
class UnaryExpr : public ExpressionNode {
public:
    enum class Operator {
        MINUS,      // -
        NOT,        // !
        INCREMENT,  // ++
        DECREMENT   // --
    };
    
    UnaryExpr(Operator op, std::shared_ptr<ExpressionNode> operand, bool isPrefix);
    ExpressionType getType() const override { return ExpressionType::UNARY; }
    
    Operator op;
    std::shared_ptr<ExpressionNode> operand;
    bool isPrefix; // true for ++x, false for x++
};

// Binary expression
class BinaryExpr : public ExpressionNode {
public:
    enum class Operator {
        PLUS,           // +
        MINUS,          // -
        MULTIPLY,       // *
        DIVIDE,         // /
        MODULO,         // %
        POWER,          // **
        EQUAL,          // ==
        NOT_EQUAL,      // !=
        STRICT_EQUAL,   // ===
        STRICT_NOT_EQUAL, // !==
        LESS,           // <
        LESS_EQUAL,     // <=
        GREATER,        // >
        GREATER_EQUAL,  // >=
        AND,            // &&
        OR              // ||
    };
    
    BinaryExpr(Operator op, std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right);
    ExpressionType getType() const override { return ExpressionType::BINARY; }
    
    Operator op;
    std::shared_ptr<ExpressionNode> left;
    std::shared_ptr<ExpressionNode> right;
};

// Call expression
class CallExpr : public ExpressionNode {
public:
    CallExpr(std::shared_ptr<ExpressionNode> callee, std::vector<std::shared_ptr<ExpressionNode>> arguments);
    ExpressionType getType() const override { return ExpressionType::CALL; }
    
    std::shared_ptr<ExpressionNode> callee;
    std::vector<std::shared_ptr<ExpressionNode>> arguments;
};

// Assignment expression
class AssignExpr : public ExpressionNode {
public:
    AssignExpr(std::shared_ptr<ExpressionNode> target, std::shared_ptr<ExpressionNode> value);
    ExpressionType getType() const override { return ExpressionType::ASSIGN; }
    
    std::shared_ptr<ExpressionNode> target;
    std::shared_ptr<ExpressionNode> value;
};

// Object literal expression
class ObjectExpr : public ExpressionNode {
public:
    struct Property {
        std::string key;
        std::shared_ptr<ExpressionNode> value;
        
        Property(const std::string& k, std::shared_ptr<ExpressionNode> v)
            : key(k), value(v) {}
    };
    
    ObjectExpr(std::vector<Property> properties);
    ExpressionType getType() const override { return ExpressionType::OBJECT; }
    
    std::vector<Property> properties;
};

// Array literal expression
class ArrayExpr : public ExpressionNode {
public:
    ArrayExpr(std::vector<std::shared_ptr<ExpressionNode>> elements);
    ExpressionType getType() const override { return ExpressionType::ARRAY; }
    
    std::vector<std::shared_ptr<ExpressionNode>> elements;
};

// Member access expression (obj.prop or obj["prop"])
class MemberExpr : public ExpressionNode {
public:
    MemberExpr(std::shared_ptr<ExpressionNode> object, std::shared_ptr<ExpressionNode> property, bool computed);
    ExpressionType getType() const override { return ExpressionType::MEMBER; }
    
    std::shared_ptr<ExpressionNode> object;
    std::shared_ptr<ExpressionNode> property;
    bool computed; // true for obj["prop"], false for obj.prop
};

// Statement node types
enum class StatementType {
    EXPRESSION,
    VARIABLE,
    BLOCK,
    IF,
    WHILE,
    FOR,
    FUNCTION,
    RETURN
};

// Statement node
class StatementNode : public ASTNode {
public:
    virtual StatementType getType() const = 0;
};

// Expression statement
class ExpressionStmt : public StatementNode {
public:
    ExpressionStmt(std::shared_ptr<ExpressionNode> expression);
    StatementType getType() const override { return StatementType::EXPRESSION; }
    
    std::shared_ptr<ExpressionNode> expression;
};

// Variable declaration statement
class VariableStmt : public StatementNode {
public:
    enum class DeclarationType {
        VAR,
        LET,
        CONST
    };
    
    VariableStmt(DeclarationType declarationType, const std::string& name, std::shared_ptr<ExpressionNode> initializer);
    StatementType getType() const override { return StatementType::VARIABLE; }
    
    DeclarationType declarationType;
    std::string name;
    std::shared_ptr<ExpressionNode> initializer; // Can be nullptr
};

// Block statement
class BlockStmt : public StatementNode {
public:
    BlockStmt(std::vector<std::shared_ptr<StatementNode>> statements);
    StatementType getType() const override { return StatementType::BLOCK; }
    
    std::vector<std::shared_ptr<StatementNode>> statements;
};

// If statement
class IfStmt : public StatementNode {
public:
    IfStmt(std::shared_ptr<ExpressionNode> condition,
          std::shared_ptr<StatementNode> thenBranch,
          std::shared_ptr<StatementNode> elseBranch);
    StatementType getType() const override { return StatementType::IF; }
    
    std::shared_ptr<ExpressionNode> condition;
    std::shared_ptr<StatementNode> thenBranch;
    std::shared_ptr<StatementNode> elseBranch; // Can be nullptr
};

// While statement
class WhileStmt : public StatementNode {
public:
    WhileStmt(std::shared_ptr<ExpressionNode> condition, std::shared_ptr<StatementNode> body);
    StatementType getType() const override { return StatementType::WHILE; }
    
    std::shared_ptr<ExpressionNode> condition;
    std::shared_ptr<StatementNode> body;
};

// For statement
class ForStmt : public StatementNode {
public:
    ForStmt(std::shared_ptr<StatementNode> initializer,
           std::shared_ptr<ExpressionNode> condition,
           std::shared_ptr<ExpressionNode> increment,
           std::shared_ptr<StatementNode> body);
    StatementType getType() const override { return StatementType::FOR; }
    
    std::shared_ptr<StatementNode> initializer; // Can be nullptr
    std::shared_ptr<ExpressionNode> condition;  // Can be nullptr (infinite loop)
    std::shared_ptr<ExpressionNode> increment;  // Can be nullptr
    std::shared_ptr<StatementNode> body;
};

// Function declaration statement
class FunctionStmt : public StatementNode {
public:
    FunctionStmt(const std::string& name,
                std::vector<std::string> parameters,
                std::shared_ptr<BlockStmt> body);
    StatementType getType() const override { return StatementType::FUNCTION; }
    
    std::string name;
    std::vector<std::string> parameters;
    std::shared_ptr<BlockStmt> body;
};

// Return statement
class ReturnStmt : public StatementNode {
public:
    ReturnStmt(std::shared_ptr<ExpressionNode> value);
    StatementType getType() const override { return StatementType::RETURN; }
    
    std::shared_ptr<ExpressionNode> value; // Can be nullptr
};

// Program node (root of the AST)
class Program : public ASTNode {
public:
    Program(std::vector<std::shared_ptr<StatementNode>> statements);
    
    std::vector<std::shared_ptr<StatementNode>> statements;
};

// JavaScript parser
class JSParser {
public:
    JSParser();
    
    // Parse source code into an AST
    std::shared_ptr<Program> parse(const std::string& source);
    
private:
    std::vector<Token> m_tokens;
    int m_current;
    
    // Helper methods
    bool isAtEnd() const;
    Token peek() const;
    Token previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(std::initializer_list<TokenType> types);
    Token consume(TokenType type, const std::string& message);
    ParseError error(Token token, const std::string& message);
    void synchronize();
    
    // Grammar rules
    std::shared_ptr<Program> program();
    std::shared_ptr<StatementNode> declaration();
    std::shared_ptr<FunctionStmt> functionDeclaration(const std::string& kind);
    std::shared_ptr<VariableStmt> variableDeclaration();
    std::shared_ptr<StatementNode> statement();
    std::shared_ptr<ExpressionStmt> expressionStatement();
    std::shared_ptr<BlockStmt> block();
    std::shared_ptr<IfStmt> ifStatement();
    std::shared_ptr<WhileStmt> whileStatement();
    std::shared_ptr<ForStmt> forStatement();
    std::shared_ptr<ReturnStmt> returnStatement();
    
    std::shared_ptr<ExpressionNode> expression();
    std::shared_ptr<ExpressionNode> assignment();
    std::shared_ptr<ExpressionNode> logicalOr();
    std::shared_ptr<ExpressionNode> logicalAnd();
    std::shared_ptr<ExpressionNode> equality();
    std::shared_ptr<ExpressionNode> comparison();
    std::shared_ptr<ExpressionNode> term();
    std::shared_ptr<ExpressionNode> factor();
    std::shared_ptr<ExpressionNode> unary();
    std::shared_ptr<ExpressionNode> call();
    std::shared_ptr<ExpressionNode> primary();
    
    // Helper for call expressions
    std::shared_ptr<ExpressionNode> finishCall(std::shared_ptr<ExpressionNode> callee);
};

} // namespace custom_js
} // namespace browser

#endif // CUSTOM_JS_PARSER_H