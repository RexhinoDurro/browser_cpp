#include <memory>
#include "js_parser.h"
// LogicalAnd -> Equality ("&&" Equality)*
std::shared_ptr<ExpressionNode> JSParser::logicalAnd() {
    std::shared_ptr<ExpressionNode> expr = equality();
    
    while (match(TokenType::AMPERSAND_AMPERSAND)) {
        std::shared_ptr<ExpressionNode> right = equality();
        expr = std::make_shared<BinaryExpr>(BinaryExpr::Operator::AND, expr, right);
    }
    
    return expr;
}

// Equality -> Comparison (("==" | "!=" | "===" | "!==") Comparison)*
std::shared_ptr<ExpressionNode> JSParser::equality() {
    std::shared_ptr<ExpressionNode> expr = comparison();
    
    while (true) {
        BinaryExpr::Operator op;
        
        if (match(TokenType::EQUAL_EQUAL)) {
            op = BinaryExpr::Operator::EQUAL;
        } else if (match(TokenType::BANG_EQUAL)) {
            op = BinaryExpr::Operator::NOT_EQUAL;
        } else if (match(TokenType::EQUAL_EQUAL_EQUAL)) {
            op = BinaryExpr::Operator::STRICT_EQUAL;
        } else if (match(TokenType::BANG_EQUAL_EQUAL)) {
            op = BinaryExpr::Operator::STRICT_NOT_EQUAL;
        } else {
            break;
        }
        
        std::shared_ptr<ExpressionNode> right = comparison();
        expr = std::make_shared<BinaryExpr>(op, expr, right);
    }
    
    return expr;
}

// Comparison -> Term (("<" | "<=" | ">" | ">=") Term)*
std::shared_ptr<ExpressionNode> JSParser::comparison() {
    std::shared_ptr<ExpressionNode> expr = term();
    
    while (true) {
        BinaryExpr::Operator op;
        
        if (match(TokenType::LESS)) {
            op = BinaryExpr::Operator::LESS;
        } else if (match(TokenType::LESS_EQUAL)) {
            op = BinaryExpr::Operator::LESS_EQUAL;
        } else if (match(TokenType::GREATER)) {
            op = BinaryExpr::Operator::GREATER;
        } else if (match(TokenType::GREATER_EQUAL)) {
            op = BinaryExpr::Operator::GREATER_EQUAL;
        } else {
            break;
        }
        
        std::shared_ptr<ExpressionNode> right = term();
        expr = std::make_shared<BinaryExpr>(op, expr, right);
    }
    
    return expr;
}

// Term -> Factor (("+" | "-") Factor)*
std::shared_ptr<ExpressionNode> JSParser::term() {
    std::shared_ptr<ExpressionNode> expr = factor();
    
    while (true) {
        BinaryExpr::Operator op;
        
        if (match(TokenType::PLUS)) {
            op = BinaryExpr::Operator::PLUS;
        } else if (match(TokenType::MINUS)) {
            op = BinaryExpr::Operator::MINUS;
        } else {
            break;
        }
        
        std::shared_ptr<ExpressionNode> right = factor();
        expr = std::make_shared<BinaryExpr>(op, expr, right);
    }
    
    return expr;
}

// Factor -> Unary (("*" | "/" | "%") Unary)*
std::shared_ptr<ExpressionNode> JSParser::factor() {
    std::shared_ptr<ExpressionNode> expr = unary();
    
    while (true) {
        BinaryExpr::Operator op;
        
        if (match(TokenType::STAR)) {
            op = BinaryExpr::Operator::MULTIPLY;
        } else if (match(TokenType::SLASH)) {
            op = BinaryExpr::Operator::DIVIDE;
        } else if (match(TokenType::PERCENT)) {
            op = BinaryExpr::Operator::MODULO;
        } else if (match(TokenType::STAR_STAR)) {
            op = BinaryExpr::Operator::POWER;
        } else {
            break;
        }
        
        std::shared_ptr<ExpressionNode> right = unary();
        expr = std::make_shared<BinaryExpr>(op, expr, right);
    }
    
    return expr;
}

// Unary -> ("!" | "-" | "++" | "--") Unary | Call
std::shared_ptr<ExpressionNode> JSParser::unary() {
    if (match(TokenType::BANG)) {
        std::shared_ptr<ExpressionNode> right = unary();
        return std::make_shared<UnaryExpr>(UnaryExpr::Operator::NOT, right, true);
    }
    
    if (match(TokenType::MINUS)) {
        std::shared_ptr<ExpressionNode> right = unary();
        return std::make_shared<UnaryExpr>(UnaryExpr::Operator::MINUS, right, true);
    }
    
    if (match(TokenType::PLUS_PLUS)) {
        std::shared_ptr<ExpressionNode> right = unary();
        return std::make_shared<UnaryExpr>(UnaryExpr::Operator::INCREMENT, right, true);
    }
    
    if (match(TokenType::MINUS_MINUS)) {
        std::shared_ptr<ExpressionNode> right = unary();
        return std::make_shared<UnaryExpr>(UnaryExpr::Operator::DECREMENT, right, true);
    }
    
    return call();
}

// Call -> Primary ("(" Arguments? ")" | "." IDENTIFIER | "[" Expression "]")*
std::shared_ptr<ExpressionNode> JSParser::call() {
    std::shared_ptr<ExpressionNode> expr = primary();
    
    while (true) {
        if (match(TokenType::LEFT_PAREN)) {
            expr = finishCall(expr);
        } else if (match(TokenType::DOT)) {
            Token name = consume(TokenType::IDENTIFIER, "Expect property name after '.'.");
            auto property = std::make_shared<LiteralExpr>(LiteralExpr::LiteralType::STRING, name.lexeme);
            expr = std::make_shared<MemberExpr>(expr, property, false);
        } else if (match(TokenType::LEFT_BRACKET)) {
            std::shared_ptr<ExpressionNode> index = expression();
            consume(TokenType::RIGHT_BRACKET, "Expect ']' after index.");
            expr = std::make_shared<MemberExpr>(expr, index, true);
        } else {
            break;
        }
    }
    
    return expr;
}

// Primary -> NUMBER | STRING | "true" | "false" | "null" | "undefined" | IDENTIFIER | "(" Expression ")" | "{" ObjectProps "}" | "[" ArrayElements "]"
std::shared_ptr<ExpressionNode> JSParser::primary() {
    if (match(TokenType::NUMBER)) {
        return std::make_shared<LiteralExpr>(LiteralExpr::LiteralType::NUMBER, previous().lexeme);
    }
    
    if (match(TokenType::STRING)) {
        return std::make_shared<LiteralExpr>(LiteralExpr::LiteralType::STRING, previous().lexeme);
    }
    
    if (match(TokenType::TRUE)) {
        return std::make_shared<LiteralExpr>(LiteralExpr::LiteralType::BOOLEAN, "true");
    }
    
    if (match(TokenType::FALSE)) {
        return std::make_shared<LiteralExpr>(LiteralExpr::LiteralType::BOOLEAN, "false");
    }
    
    if (match(TokenType::NULL_TOKEN)) {
        return std::make_shared<LiteralExpr>(LiteralExpr::LiteralType::NULL_TYPE, "null");
    }
    
    if (match(TokenType::UNDEFINED)) {
        return std::make_shared<LiteralExpr>(LiteralExpr::LiteralType::UNDEFINED, "undefined");
    }
    
    if (match(TokenType::IDENTIFIER)) {
        return std::make_shared<VariableExpr>(previous().lexeme);
    }
    
    if (match(TokenType::LEFT_PAREN)) {
        std::shared_ptr<ExpressionNode> expr = expression();
        consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
        return expr;
    }
    
    if (match(TokenType::LEFT_BRACE)) {
        // Object literal
        std::vector<ObjectExpr::Property> properties;
        
        if (!check(TokenType::RIGHT_BRACE)) {
            do {
                // Parse property
                if (check(TokenType::STRING) || check(TokenType::IDENTIFIER)) {
                    Token key = advance();
                    consume(TokenType::COLON, "Expect ':' after property name.");
                    std::shared_ptr<ExpressionNode> value = expression();
                    properties.push_back(ObjectExpr::Property(key.lexeme, value));
                } else {
                    throw error(peek(), "Expect property name.");
                }
            } while (match(TokenType::COMMA));
        }
        
        consume(TokenType::RIGHT_BRACE, "Expect '}' after object literal.");
        return std::make_shared<ObjectExpr>(properties);
    }
    
    if (match(TokenType::LEFT_BRACKET)) {
        // Array literal
        std::vector<std::shared_ptr<ExpressionNode>> elements;
        
        if (!check(TokenType::RIGHT_BRACKET)) {
            do {
                elements.push_back(expression());
            } while (match(TokenType::COMMA));
        }
        
        consume(TokenType::RIGHT_BRACKET, "Expect ']' after array literal.");
        return std::make_shared<ArrayExpr>(elements);
    }
    
    throw error(peek(), "Expect expression.");
}

// Helper for parsing call expressions
std::shared_ptr<ExpressionNode> JSParser::finishCall(std::shared_ptr<ExpressionNode> callee) {
    std::vector<std::shared_ptr<ExpressionNode>> arguments;
    
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            if (arguments.size() >= 255) {
                error(peek(), "Cannot have more than 255 arguments.");
            }
            arguments.push_back(expression());
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
    
    return std::make_shared<CallExpr>(callee, arguments);
}

} // namespace custom_js
} // namespace browser#include "js_parser.h"
#include <iostream>

namespace browser {
namespace custom_js {

//-----------------------------------------------------------------------------
// AST Node Implementations
//-----------------------------------------------------------------------------

// LiteralExpr
LiteralExpr::LiteralExpr(LiteralType type, const std::string& value)
    : literalType(type), value(value) {
}

// VariableExpr
VariableExpr::VariableExpr(const std::string& name)
    : name(name) {
}

// UnaryExpr
UnaryExpr::UnaryExpr(Operator op, std::shared_ptr<ExpressionNode> operand, bool isPrefix)
    : op(op), operand(operand), isPrefix(isPrefix) {
}

// BinaryExpr
BinaryExpr::BinaryExpr(Operator op, std::shared_ptr<ExpressionNode> left, std::shared_ptr<ExpressionNode> right)
    : op(op), left(left), right(right) {
}

// CallExpr
CallExpr::CallExpr(std::shared_ptr<ExpressionNode> callee, std::vector<std::shared_ptr<ExpressionNode>> arguments)
    : callee(callee), arguments(arguments) {
}

// AssignExpr
AssignExpr::AssignExpr(std::shared_ptr<ExpressionNode> target, std::shared_ptr<ExpressionNode> value)
    : target(target), value(value) {
}

// ObjectExpr
ObjectExpr::ObjectExpr(std::vector<Property> properties)
    : properties(properties) {
}

// ArrayExpr
ArrayExpr::ArrayExpr(std::vector<std::shared_ptr<ExpressionNode>> elements)
    : elements(elements) {
}

// MemberExpr
MemberExpr::MemberExpr(std::shared_ptr<ExpressionNode> object, std::shared_ptr<ExpressionNode> property, bool computed)
    : object(object), property(property), computed(computed) {
}

// ExpressionStmt
ExpressionStmt::ExpressionStmt(std::shared_ptr<ExpressionNode> expression)
    : expression(expression) {
}

// VariableStmt
VariableStmt::VariableStmt(DeclarationType declarationType, const std::string& name, std::shared_ptr<ExpressionNode> initializer)
    : declarationType(declarationType), name(name), initializer(initializer) {
}

// BlockStmt
BlockStmt::BlockStmt(std::vector<std::shared_ptr<StatementNode>> statements)
    : statements(statements) {
}

// IfStmt
IfStmt::IfStmt(std::shared_ptr<ExpressionNode> condition, std::shared_ptr<StatementNode> thenBranch, std::shared_ptr<StatementNode> elseBranch)
    : condition(condition), thenBranch(thenBranch), elseBranch(elseBranch) {
}

// WhileStmt
WhileStmt::WhileStmt(std::shared_ptr<ExpressionNode> condition, std::shared_ptr<StatementNode> body)
    : condition(condition), body(body) {
}

// ForStmt
ForStmt::ForStmt(std::shared_ptr<StatementNode> initializer, std::shared_ptr<ExpressionNode> condition, std::shared_ptr<ExpressionNode> increment, std::shared_ptr<StatementNode> body)
    : initializer(initializer), condition(condition), increment(increment), body(body) {
}

// FunctionStmt
FunctionStmt::FunctionStmt(const std::string& name, std::vector<std::string> parameters, std::shared_ptr<BlockStmt> body)
    : name(name), parameters(parameters), body(body) {
}

// ReturnStmt
ReturnStmt::ReturnStmt(std::shared_ptr<ExpressionNode> value)
    : value(value) {
}

// Program
Program::Program(std::vector<std::shared_ptr<StatementNode>> statements)
    : statements(statements) {
}

//-----------------------------------------------------------------------------
// Parser Implementation
//-----------------------------------------------------------------------------

JSParser::JSParser() : m_current(0) {
}

std::shared_ptr<Program> JSParser::parse(const std::string& source) {
    JSLexer lexer;
    m_tokens = lexer.tokenize(source);
    m_current = 0;
    
    try {
        return program();
    } catch (const ParseError& error) {
        std::cerr << "Parse error: " << error.what() << std::endl;
        return std::make_shared<Program>(std::vector<std::shared_ptr<StatementNode>>());
    }
}

bool JSParser::isAtEnd() const {
    return peek().type == TokenType::EOF_TOKEN;
}

Token JSParser::peek() const {
    return m_tokens[m_current];
}

Token JSParser::previous() const {
    return m_tokens[m_current - 1];
}

Token JSParser::advance() {
    if (!isAtEnd()) m_current++;
    return previous();
}

bool JSParser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool JSParser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool JSParser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (match(type)) return true;
    }
    return false;
}

Token JSParser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    
    throw error(peek(), message);
}

ParseError JSParser::error(Token token, const std::string& message) {
    std::string errorMsg = "Error at line " + std::to_string(token.line) + ": " + message;
    if (token.type == TokenType::EOF_TOKEN) {
        errorMsg += " at end";
    } else {
        errorMsg += " at '" + token.lexeme + "'";
    }
    return ParseError(errorMsg);
}

void JSParser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;
        
        switch (peek().type) {
            case TokenType::FUNCTION:
            case TokenType::VAR:
            case TokenType::LET:
            case TokenType::CONST:
            case TokenType::FOR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::RETURN:
                return;
            default:
                break;
        }
        
        advance();
    }
}

// Program -> Declaration* EOF
std::shared_ptr<Program> JSParser::program() {
    std::vector<std::shared_ptr<StatementNode>> statements;
    
    while (!isAtEnd()) {
        try {
            statements.push_back(declaration());
        } catch (const ParseError& error) {
            std::cerr << error.what() << std::endl;
            synchronize();
        }
    }
    
    return std::make_shared<Program>(statements);
}

// Declaration -> FunctionDecl | VariableDecl | Statement
std::shared_ptr<StatementNode> JSParser::declaration() {
    if (match(TokenType::FUNCTION)) {
        return functionDeclaration("function");
    }
    
    if (match({TokenType::VAR, TokenType::LET, TokenType::CONST})) {
        return variableDeclaration();
    }
    
    return statement();
}

// FunctionDecl -> "function" IDENTIFIER "(" Parameters? ")" Block
std::shared_ptr<FunctionStmt> JSParser::functionDeclaration(const std::string& kind) {
    Token name = consume(TokenType::IDENTIFIER, "Expect " + kind + " name.");
    consume(TokenType::LEFT_PAREN, "Expect '(' after " + kind + " name.");
    
    std::vector<std::string> parameters;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            if (parameters.size() >= 255) {
                error(peek(), "Cannot have more than 255 parameters.");
            }
            
            Token param = consume(TokenType::IDENTIFIER, "Expect parameter name.");
            parameters.push_back(param.lexeme);
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");
    
    consume(TokenType::LEFT_BRACE, "Expect '{' before " + kind + " body.");
    std::shared_ptr<BlockStmt> body = block();
    
    return std::make_shared<FunctionStmt>(name.lexeme, parameters, body);
}

// VariableDecl -> ("var" | "let" | "const") IDENTIFIER ("=" Expression)? ";"
std::shared_ptr<VariableStmt> JSParser::variableDeclaration() {
    Token declarationToken = previous();
    VariableStmt::DeclarationType declType;
    
    if (declarationToken.type == TokenType::VAR) {
        declType = VariableStmt::DeclarationType::VAR;
    } else if (declarationToken.type == TokenType::LET) {
        declType = VariableStmt::DeclarationType::LET;
    } else {
        declType = VariableStmt::DeclarationType::CONST;
    }
    
    Token name = consume(TokenType::IDENTIFIER, "Expect variable name.");
    
    std::shared_ptr<ExpressionNode> initializer = nullptr;
    if (match(TokenType::EQUAL)) {
        initializer = expression();
    } else if (declType == VariableStmt::DeclarationType::CONST) {
        throw error(previous(), "Const variables must be initialized.");
    }
    
    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
    return std::make_shared<VariableStmt>(declType, name.lexeme, initializer);
}

// Statement -> ExpressionStmt | BlockStmt | IfStmt | WhileStmt | ForStmt | ReturnStmt
std::shared_ptr<StatementNode> JSParser::statement() {
    if (match(TokenType::LEFT_BRACE)) return block();
    if (match(TokenType::IF)) return ifStatement();
    if (match(TokenType::WHILE)) return whileStatement();
    if (match(TokenType::FOR)) return forStatement();
    if (match(TokenType::RETURN)) return returnStatement();
    
    return expressionStatement();
}

// ExpressionStmt -> Expression ";"
std::shared_ptr<ExpressionStmt> JSParser::expressionStatement() {
    std::shared_ptr<ExpressionNode> expr = expression();
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    return std::make_shared<ExpressionStmt>(expr);
}

// BlockStmt -> "{" Declaration* "}"
std::shared_ptr<BlockStmt> JSParser::block() {
    std::vector<std::shared_ptr<StatementNode>> statements;
    
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
        statements.push_back(declaration());
    }
    
    consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");
    return std::make_shared<BlockStmt>(statements);
}

// IfStmt -> "if" "(" Expression ")" Statement ("else" Statement)?
std::shared_ptr<IfStmt> JSParser::ifStatement() {
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
    std::shared_ptr<ExpressionNode> condition = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after if condition.");
    
    std::shared_ptr<StatementNode> thenBranch = statement();
    std::shared_ptr<StatementNode> elseBranch = nullptr;
    
    if (match(TokenType::ELSE)) {
        elseBranch = statement();
    }
    
    return std::make_shared<IfStmt>(condition, thenBranch, elseBranch);
}

// WhileStmt -> "while" "(" Expression ")" Statement
std::shared_ptr<WhileStmt> JSParser::whileStatement() {
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
    std::shared_ptr<ExpressionNode> condition = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after while condition.");
    
    std::shared_ptr<StatementNode> body = statement();
    
    return std::make_shared<WhileStmt>(condition, body);
}

// ForStmt -> "for" "(" (VariableDecl | ExpressionStmt | ";") Expression? ";" Expression? ")" Statement
std::shared_ptr<ForStmt> JSParser::forStatement() {
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'for'.");
    
    // Initializer
    std::shared_ptr<StatementNode> initializer;
    if (match(TokenType::SEMICOLON)) {
        initializer = nullptr;
    } else if (match({TokenType::VAR, TokenType::LET, TokenType::CONST})) {
        initializer = variableDeclaration();
    } else {
        initializer = expressionStatement();
    }
    
    // Condition
    std::shared_ptr<ExpressionNode> condition = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        condition = expression();
    }
    consume(TokenType::SEMICOLON, "Expect ';' after loop condition.");
    
    // Increment
    std::shared_ptr<ExpressionNode> increment = nullptr;
    if (!check(TokenType::RIGHT_PAREN)) {
        increment = expression();
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after for clauses.");
    
    // Body
    std::shared_ptr<StatementNode> body = statement();
    
    return std::make_shared<ForStmt>(initializer, condition, increment, body);
}

// ReturnStmt -> "return" Expression? ";"
std::shared_ptr<ReturnStmt> JSParser::returnStatement() {
    std::shared_ptr<ExpressionNode> value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = expression();
    }
    
    consume(TokenType::SEMICOLON, "Expect ';' after return value.");
    return std::make_shared<ReturnStmt>(value);
}

// Expression -> Assignment
std::shared_ptr<ExpressionNode> JSParser::expression() {
    return assignment();
}

// Assignment -> LogicalOr ("=" Assignment)?
std::shared_ptr<ExpressionNode> JSParser::assignment() {
    std::shared_ptr<ExpressionNode> expr = logicalOr();
    
    if (match(TokenType::EQUAL)) {
        std::shared_ptr<ExpressionNode> value = assignment();
        
        // Check if left side is a valid assignment target
        if (expr->getType() == ExpressionType::VARIABLE) {
            auto varExpr = std::dynamic_pointer_cast<VariableExpr>(expr);
            return std::make_shared<AssignExpr>(expr, value);
        } else if (expr->getType() == ExpressionType::MEMBER) {
            // Allow object property assignment
            return std::make_shared<AssignExpr>(expr, value);
        }
        
        throw error(previous(), "Invalid assignment target.");
    }
    
    return expr;
}

// LogicalOr -> LogicalAnd ("||" LogicalAnd)*
std::shared_ptr<ExpressionNode> JSParser::logicalOr() {
    std::shared_ptr<ExpressionNode> expr = logicalAnd();
    
    while (match(TokenType::PIPE_PIPE)) {
        std::shared_ptr<ExpressionNode> right = logicalAnd();
        expr = std::make_shared<BinaryExpr>(BinaryExpr::Operator::OR, expr, right);
    }
    
    return expr;
}