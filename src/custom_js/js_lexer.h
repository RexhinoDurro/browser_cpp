// js_lexer.h - Tokenizes JavaScript code
#ifndef CUSTOM_JS_LEXER_H
#define CUSTOM_JS_LEXER_H

#include <string>
#include <vector>
#include <map>
#include <set>

namespace browser {
namespace custom_js {

// Token types
enum class TokenType {
    // Single-character tokens
    LEFT_PAREN, RIGHT_PAREN,      // ( )
    LEFT_BRACE, RIGHT_BRACE,      // { }
    LEFT_BRACKET, RIGHT_BRACKET,  // [ ]
    COMMA, DOT, SEMICOLON, COLON, // , . ; :
    
    // One or two character tokens
    PLUS, PLUS_PLUS,              // + ++
    MINUS, MINUS_MINUS,           // - --
    STAR, STAR_STAR,              // * **
    SLASH, PERCENT,               // / %
    EQUAL, EQUAL_EQUAL,           // = ==
    EQUAL_EQUAL_EQUAL,            // ===
    BANG, BANG_EQUAL,             // ! !=
    BANG_EQUAL_EQUAL,             // !==
    LESS, LESS_EQUAL,             // < <=
    GREATER, GREATER_EQUAL,       // > >=
    AMPERSAND, AMPERSAND_AMPERSAND, // & &&
    PIPE, PIPE_PIPE,              // | ||
    
    // Literals
    IDENTIFIER, STRING, NUMBER,
    
    // Keywords
    VAR, LET, CONST,
    IF, ELSE, FOR, WHILE, DO,
    FUNCTION, RETURN,
    TRUE, FALSE, NULL_TOKEN, UNDEFINED,
    THIS, NEW,
    
    // Special
    EOF_TOKEN,
    ERROR
};

// Represents a token in the source code
struct Token {
    TokenType type;
    std::string lexeme;  // The actual text
    int line;            // Line number
    
    Token(TokenType type, const std::string& lexeme, int line) 
        : type(type), lexeme(lexeme), line(line) {}
    
    std::string toString() const;
};

// JavaScript lexer class
class JSLexer {
public:
    JSLexer();
    
    // Tokenize source code
    std::vector<Token> tokenize(const std::string& source);
    
private:
    std::string m_source;
    std::vector<Token> m_tokens;
    int m_start;        // Start of current lexeme
    int m_current;      // Current position
    int m_line;         // Current line
    
    // Helper methods
    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    
    // Token recognition
    void scanToken();
    void scanString();
    void scanNumber();
    void scanIdentifier();
    
    // Add tokens
    void addToken(TokenType type);
    void addToken(TokenType type, const std::string& lexeme);
    
    // Character checks
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    
    // Keyword mapping
    std::map<std::string, TokenType> m_keywords;
    void initKeywords();
};

} // namespace custom_js
} // namespace browser

#endif // CUSTOM_JS_LEXER_H