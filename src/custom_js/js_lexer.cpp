#include "js_lexer.h"
#include <iostream>
#include <sstream>

namespace browser {
namespace custom_js {

// Token to string conversion
std::string Token::toString() const {
    std::stringstream ss;
    ss << "Token[type=" << static_cast<int>(type) << ", lexeme='" << lexeme << "', line=" << line << "]";
    return ss.str();
}

// JSLexer implementation
JSLexer::JSLexer() {
    initKeywords();
}

void JSLexer::initKeywords() {
    m_keywords["var"] = TokenType::VAR;
    m_keywords["let"] = TokenType::LET;
    m_keywords["const"] = TokenType::CONST;
    m_keywords["if"] = TokenType::IF;
    m_keywords["else"] = TokenType::ELSE;
    m_keywords["for"] = TokenType::FOR;
    m_keywords["while"] = TokenType::WHILE;
    m_keywords["do"] = TokenType::DO;
    m_keywords["function"] = TokenType::FUNCTION;
    m_keywords["return"] = TokenType::RETURN;
    m_keywords["true"] = TokenType::TRUE;
    m_keywords["false"] = TokenType::FALSE;
    m_keywords["null"] = TokenType::NULL_TOKEN;
    m_keywords["undefined"] = TokenType::UNDEFINED;
    m_keywords["this"] = TokenType::THIS;
    m_keywords["new"] = TokenType::NEW;
}

std::vector<Token> JSLexer::tokenize(const std::string& source) {
    m_source = source;
    m_tokens.clear();
    m_start = 0;
    m_current = 0;
    m_line = 1;
    
    while (!isAtEnd()) {
        m_start = m_current;
        scanToken();
    }
    
    // Add EOF token
    m_tokens.push_back(Token(TokenType::EOF_TOKEN, "", m_line));
    return m_tokens;
}

bool JSLexer::isAtEnd() const {
    return m_current >= m_source.length();
}

char JSLexer::advance() {
    return m_source[m_current++];
}

char JSLexer::peek() const {
    if (isAtEnd()) return '\0';
    return m_source[m_current];
}

char JSLexer::peekNext() const {
    if (m_current + 1 >= m_source.length()) return '\0';
    return m_source[m_current + 1];
}

bool JSLexer::match(char expected) {
    if (isAtEnd() || m_source[m_current] != expected) return false;
    
    m_current++;
    return true;
}

void JSLexer::scanToken() {
    char c = advance();
    
    switch (c) {
        // Single-character tokens
        case '(': addToken(TokenType::LEFT_PAREN); break;
        case ')': addToken(TokenType::RIGHT_PAREN); break;
        case '{': addToken(TokenType::LEFT_BRACE); break;
        case '}': addToken(TokenType::RIGHT_BRACE); break;
        case '[': addToken(TokenType::LEFT_BRACKET); break;
        case ']': addToken(TokenType::RIGHT_BRACKET); break;
        case ',': addToken(TokenType::COMMA); break;
        case '.': addToken(TokenType::DOT); break;
        case ';': addToken(TokenType::SEMICOLON); break;
        case ':': addToken(TokenType::COLON); break;
        
        // One or two character tokens
        case '+':
            if (match('+')) {
                addToken(TokenType::PLUS_PLUS);
            } else {
                addToken(TokenType::PLUS);
            }
            break;
        case '-':
            if (match('-')) {
                addToken(TokenType::MINUS_MINUS);
            } else {
                addToken(TokenType::MINUS);
            }
            break;
        case '*':
            if (match('*')) {
                addToken(TokenType::STAR_STAR);
            } else {
                addToken(TokenType::STAR);
            }
            break;
        case '/':
            if (match('/')) {
                // Comment - consume until end of line
                while (peek() != '\n' && !isAtEnd()) advance();
            } else if (match('*')) {
                // Multiline comment
                while (!isAtEnd() && !(peek() == '*' && peekNext() == '/')) {
                    if (peek() == '\n') m_line++;
                    advance();
                }
                
                // Consume the closing */
                if (!isAtEnd()) {
                    advance(); // *
                    advance(); // /
                }
            } else {
                addToken(TokenType::SLASH);
            }
            break;
        case '%': addToken(TokenType::PERCENT); break;
        case '=':
            if (match('=')) {
                if (match('=')) {
                    addToken(TokenType::EQUAL_EQUAL_EQUAL);
                } else {
                    addToken(TokenType::EQUAL_EQUAL);
                }
            } else {
                addToken(TokenType::EQUAL);
            }
            break;
        case '!':
            if (match('=')) {
                if (match('=')) {
                    addToken(TokenType::BANG_EQUAL_EQUAL);
                } else {
                    addToken(TokenType::BANG_EQUAL);
                }
            } else {
                addToken(TokenType::BANG);
            }
            break;
        case '<':
            addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
            break;
        case '>':
            addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
            break;
        case '&':
            addToken(match('&') ? TokenType::AMPERSAND_AMPERSAND : TokenType::AMPERSAND);
            break;
        case '|':
            addToken(match('|') ? TokenType::PIPE_PIPE : TokenType::PIPE);
            break;
            
        // String literals
        case '"': scanString(); break;
        case '\'': scanString(); break;
            
        // Whitespace
        case ' ':
        case '\r':
        case '\t':
            // Ignore whitespace
            break;
        case '\n':
            m_line++;
            break;
            
        // Numbers and identifiers
        default:
            if (isDigit(c)) {
                scanNumber();
            } else if (isAlpha(c)) {
                scanIdentifier();
            } else {
                // Error - unexpected character
                std::string errorMsg = "Unexpected character: ";
                errorMsg += c;
                addToken(TokenType::ERROR, errorMsg);
            }
            break;
    }
}

void JSLexer::scanString() {
    // Remember the quote type (' or ")
    char quoteType = m_source[m_start];
    
    // Capture the string content
    while (peek() != quoteType && !isAtEnd()) {
        if (peek() == '\n') m_line++;
        
        // Handle escaped characters
        if (peek() == '\\' && !isAtEnd()) {
            advance(); // Consume the backslash
        }
        
        advance();
    }
    
    // Unterminated string
    if (isAtEnd()) {
        addToken(TokenType::ERROR, "Unterminated string.");
        return;
    }
    
    // Consume the closing quote
    advance();
    
    // Extract the string value (without the quotes)
    std::string value = m_source.substr(m_start + 1, m_current - m_start - 2);
    
    // Process escape sequences
    std::string processed;
    for (size_t i = 0; i < value.length(); i++) {
        if (value[i] == '\\' && i + 1 < value.length()) {
            switch (value[i + 1]) {
                case 'n': processed += '\n'; break;
                case 't': processed += '\t'; break;
                case 'r': processed += '\r'; break;
                case '\\': processed += '\\'; break;
                case '\'': processed += '\''; break;
                case '"': processed += '"'; break;
                // Add more escape sequences as needed
                default: processed += value[i + 1]; break;
            }
            i++; // Skip the escaped character
        } else {
            processed += value[i];
        }
    }
    
    addToken(TokenType::STRING, processed);
}

void JSLexer::scanNumber() {
    // Consume integer part
    while (isDigit(peek())) advance();
    
    // Look for a decimal point
    if (peek() == '.' && isDigit(peekNext())) {
        // Consume the dot
        advance();
        
        // Consume fractional part
        while (isDigit(peek())) advance();
    }
    
    // Look for exponent
    if (peek() == 'e' || peek() == 'E') {
        advance();
        
        // Optional sign
        if (peek() == '+' || peek() == '-') advance();
        
        // Exponent digits
        if (isDigit(peek())) {
            while (isDigit(peek())) advance();
        } else {
            // Invalid exponent
            addToken(TokenType::ERROR, "Invalid number format.");
            return;
        }
    }
    
    // Extract the number as a string
    std::string numberStr = m_source.substr(m_start, m_current - m_start);
    addToken(TokenType::NUMBER, numberStr);
}

void JSLexer::scanIdentifier() {
    while (isAlphaNumeric(peek())) advance();
    
    // Extract the identifier
    std::string identifier = m_source.substr(m_start, m_current - m_start);
    
    // Check if it's a keyword
    auto it = m_keywords.find(identifier);
    if (it != m_keywords.end()) {
        addToken(it->second);
    } else {
        addToken(TokenType::IDENTIFIER, identifier);
    }
}

void JSLexer::addToken(TokenType type) {
    addToken(type, m_source.substr(m_start, m_current - m_start));
}

void JSLexer::addToken(TokenType type, const std::string& lexeme) {
    m_tokens.push_back(Token(type, lexeme, m_line));
}

bool JSLexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool JSLexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || 
           (c >= 'A' && c <= 'Z') || 
           c == '_' || c == '$';
}

bool JSLexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

} // namespace custom_js
} // namespace browser