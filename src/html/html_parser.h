#ifndef BROWSER_HTML_PARSER_H
#define BROWSER_HTML_PARSER_H

#include "dom_tree.h"
#include <string>
#include <vector>
#include <memory>
#include <stack>
#include <map>

namespace browser {
namespace html {

// Forward declarations
class Element;
class Node;

// HTML token types
enum class TokenType {
    DOCTYPE,
    START_TAG,
    END_TAG,
    COMMENT,
    TEXT,
    EOF_TOKEN
};

// HTML token structure
struct Token {
    TokenType type;
    std::string name;
    std::string data;
    std::map<std::string, std::string> attributes;
    bool selfClosing = false;
};

// HTML parser states
enum class ParserState {
    DATA,
    TAG_OPEN,
    END_TAG_OPEN,
    TAG_NAME,
    BEFORE_ATTRIBUTE_NAME,
    ATTRIBUTE_NAME,
    AFTER_ATTRIBUTE_NAME,
    BEFORE_ATTRIBUTE_VALUE,
    ATTRIBUTE_VALUE_DOUBLE_QUOTED,
    ATTRIBUTE_VALUE_SINGLE_QUOTED,
    ATTRIBUTE_VALUE_UNQUOTED,
    AFTER_ATTRIBUTE_VALUE_QUOTED,
    SELF_CLOSING_START_TAG,
    BOGUS_COMMENT,
    MARKUP_DECLARATION_OPEN,
    COMMENT_START,
    COMMENT,
    COMMENT_END,
    COMMENT_END_DASH,
    DOCTYPE,
    BEFORE_DOCTYPE_NAME,
    DOCTYPE_NAME,
    AFTER_DOCTYPE_NAME,
    AFTER_DOCTYPE_PUBLIC_KEYWORD,
    DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED,
    DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED,
    AFTER_DOCTYPE_PUBLIC_IDENTIFIER,
    BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS,
    AFTER_DOCTYPE_SYSTEM_KEYWORD,
    DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED,
    DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED,
    AFTER_DOCTYPE_SYSTEM_IDENTIFIER,
    BOGUS_DOCTYPE
};

// HTML Parser class responsible for parsing HTML documents
class HTMLParser {
public:
    HTMLParser();
    ~HTMLParser();

    // Initialize the HTML parser
    bool initialize();

    // Parse HTML content and return a DOM tree
    DOMTree parse(const std::string& html);

    // Parse a single HTML element (for testing/partial parsing)
    std::shared_ptr<Element> parseElement(const std::string& html);

    // Resolve URL helper
    std::string resolveUrl(const std::string& baseUrl, const std::string& relativeUrl);

private:
    // Tokenization methods
    Token nextToken();
    void consumeCharacter();
    char currentChar() const;
    char peekNext() const;
    bool hasMoreChars() const;
    void reconsume();
    
    // Token creation helpers
    Token createStartToken(const std::string& name);
    Token createEndToken(const std::string& name);
    Token createTextToken(const std::string& text);
    Token createCommentToken(const std::string& data);
    Token createDOCTYPEToken(const std::string& name, 
                             const std::string& publicId = "", 
                             const std::string& systemId = "", 
                             bool forceQuirks = false);
    
    // Tree construction
    void processToken(const Token& token);
    void insertElement(const Token& token);
    void insertComment(const Token& token);
    void insertText(const Token& token);
    void insertDoctype(const Token& token);
    void endTagToken(const Token& token);
    
    // Special element handling
    bool isSpecialElement(const std::string& tagName);
    bool isSelfClosingTag(const std::string& tagName);
    
    // State handlers for tokenization
    void handleDataState();
    void handleTagOpenState();
    void handleEndTagOpenState();
    void handleTagNameState();
    void handleBeforeAttributeNameState();
    void handleAttributeNameState();
    void handleAfterAttributeNameState();
    void handleBeforeAttributeValueState();
    void handleAttributeValueDoubleQuotedState();
    void handleAttributeValueSingleQuotedState();
    void handleAttributeValueUnquotedState();
    void handleAfterAttributeValueQuotedState();
    void handleSelfClosingStartTagState();
    void handleBogusCommentState();
    void handleMarkupDeclarationOpenState();
    void handleCommentStartState();
    void handleCommentState();
    void handleCommentEndState();
    void handleCommentEndDashState();
    void handleDOCTYPEState();
    void handleBeforeDOCTYPENameState();
    void handleDOCTYPENameState();
    void handleAfterDOCTYPENameState();
    
    // Error handling
    void parseError(const std::string& message);
    
    // Members
    std::string m_html;
    size_t m_position;
    ParserState m_state;
    Token m_currentToken;
    std::string m_tempBuffer;
    bool m_reconsumeCurrentChar;
    
    // DOM construction members
    DOMTree m_document;
    std::stack<std::shared_ptr<Element>> m_openElements;
    Node* m_currentNode;
    bool m_isInitialized;
    
    // Error tracking
    std::vector<std::string> m_errors;
};

} // namespace html
} // namespace browser

#endif // BROWSER_HTML_PARSER_H