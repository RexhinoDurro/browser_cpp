#include "html_parser.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace browser {
namespace html {

// Special tag sets for parsing rules
static const std::unordered_set<std::string> kSelfClosingTags = {
    "area", "base", "br", "col", "embed", "hr", "img", "input", 
    "link", "meta", "param", "source", "track", "wbr"
};

static const std::unordered_set<std::string> kSpecialElements = {
    "address", "applet", "area", "article", "aside", "base", "basefont", "bgsound",
    "blockquote", "body", "br", "button", "caption", "center", "col", "colgroup", 
    "dd", "details", "dir", "div", "dl", "dt", "embed", "fieldset", "figcaption", 
    "figure", "footer", "form", "frame", "frameset", "h1", "h2", "h3", "h4", "h5", 
    "h6", "head", "header", "hgroup", "hr", "html", "iframe", "img", "input", "li", 
    "link", "listing", "main", "marquee", "menu", "meta", "nav", "noembed", "noframes", 
    "noscript", "object", "ol", "p", "param", "plaintext", "pre", "script", "section", 
    "select", "source", "style", "summary", "table", "tbody", "td", "template", "textarea", 
    "tfoot", "th", "thead", "title", "tr", "track", "ul", "wbr", "xmp"
};

HTMLParser::HTMLParser() 
    : m_position(0)
    , m_state(ParserState::DATA)
    , m_reconsumeCurrentChar(false)
    , m_isInitialized(false)
{
}

HTMLParser::~HTMLParser() {
}

bool HTMLParser::initialize() {
    m_isInitialized = true;
    return true;
}

DOMTree HTMLParser::parse(const std::string& html) {
    if (!m_isInitialized) {
        initialize();
    }
    
    // Reset parser state
    m_html = html;
    m_position = 0;
    m_state = ParserState::DATA;
    m_reconsumeCurrentChar = false;
    m_tempBuffer.clear();
    m_errors.clear();
    
    // Initialize DOM tree
    m_document.initialize();
    m_currentNode = m_document.document();
    
    // Clear open elements stack and push the initial HTML element
    while (!m_openElements.empty()) {
        m_openElements.pop();
    }
    
    // Create default HTML structure if none exists
    auto docElement = m_document.document()->createElement("html");
    m_document.document()->appendChild(docElement);
    m_openElements.push(docElement);
    
    // Head element
    auto headElement = m_document.document()->createElement("head");
    docElement->appendChild(headElement);
    m_openElements.push(headElement);
    
    // Process all tokens until EOF
    Token token;
    do {
        token = nextToken();
        processToken(token);
    } while (token.type != TokenType::EOF_TOKEN);
    
    return m_document;
}

std::shared_ptr<Element> HTMLParser::parseElement(const std::string& html) {
    // Create a temporary document to hold the element
    DOMTree tempDoc;
    tempDoc.initialize();
    
    // Parse the HTML fragment
    std::string wrappedHTML = "<div>" + html + "</div>";
    DOMTree result = parse(wrappedHTML);
    
    // Extract the first element from the wrapped div
    Element* div = result.document()->querySelector("div");
    if (!div || !div->firstElementChild()) {
        return nullptr;
    }
    
    // Clone the element
    return std::dynamic_pointer_cast<Element>(div->firstElementChild()->cloneNode(true));
}

Token HTMLParser::nextToken() {
    // Reset the current token
    m_currentToken = Token();
    
    // Process characters until we have a complete token
    while (hasMoreChars() || m_reconsumeCurrentChar) {
        // Handle the current state
        switch (m_state) {
            case ParserState::DATA:
                handleDataState();
                break;
            case ParserState::TAG_OPEN:
                handleTagOpenState();
                break;
            case ParserState::END_TAG_OPEN:
                handleEndTagOpenState();
                break;
            case ParserState::TAG_NAME:
                handleTagNameState();
                break;
            case ParserState::BEFORE_ATTRIBUTE_NAME:
                handleBeforeAttributeNameState();
                break;
            case ParserState::ATTRIBUTE_NAME:
                handleAttributeNameState();
                break;
            case ParserState::AFTER_ATTRIBUTE_NAME:
                handleAfterAttributeNameState();
                break;
            case ParserState::BEFORE_ATTRIBUTE_VALUE:
                handleBeforeAttributeValueState();
                break;
            case ParserState::ATTRIBUTE_VALUE_DOUBLE_QUOTED:
                handleAttributeValueDoubleQuotedState();
                break;
            case ParserState::ATTRIBUTE_VALUE_SINGLE_QUOTED:
                handleAttributeValueSingleQuotedState();
                break;
            case ParserState::ATTRIBUTE_VALUE_UNQUOTED:
                handleAttributeValueUnquotedState();
                break;
            case ParserState::AFTER_ATTRIBUTE_VALUE_QUOTED:
                handleAfterAttributeValueQuotedState();
                break;
            case ParserState::SELF_CLOSING_START_TAG:
                handleSelfClosingStartTagState();
                break;
            case ParserState::BOGUS_COMMENT:
                handleBogusCommentState();
                break;
            case ParserState::MARKUP_DECLARATION_OPEN:
                handleMarkupDeclarationOpenState();
                break;
            case ParserState::COMMENT_START:
                handleCommentStartState();
                break;
            case ParserState::COMMENT:
                handleCommentState();
                break;
            case ParserState::COMMENT_END:
                handleCommentEndState();
                break;
            case ParserState::COMMENT_END_DASH:
                handleCommentEndDashState();
                break;
            case ParserState::DOCTYPE:
                handleDOCTYPEState();
                break;
            case ParserState::BEFORE_DOCTYPE_NAME:
                handleBeforeDOCTYPENameState();
                break;
            case ParserState::DOCTYPE_NAME:
                handleDOCTYPENameState();
                break;
            case ParserState::AFTER_DOCTYPE_NAME:
                handleAfterDOCTYPENameState();
                break;
            default:
                // Unhandled state, consume the character and stay in DATA state
                parseError("Unhandled parser state: " + std::to_string(static_cast<int>(m_state)));
                consumeCharacter();
                m_state = ParserState::DATA;
                break;
        }
        
        // If we've constructed a complete token, return it
        if (m_currentToken.type != TokenType::EOF_TOKEN) {
            return m_currentToken;
        }
    }
    
    // If we've reached the end of the input, return an EOF token
    return Token{TokenType::EOF_TOKEN, "", "", {}, false};
}

void HTMLParser::handleDataState() {
    if (!hasMoreChars()) {
        m_currentToken = Token{TokenType::EOF_TOKEN, "", "", {}, false};
        return;
    }
    
    char c = currentChar();
    if (c == '<') {
        // Switch to tag open state
        m_state = ParserState::TAG_OPEN;
        consumeCharacter();
    } else {
        // Build a text token
        std::string text;
        while (hasMoreChars() && currentChar() != '<') {
            text += currentChar();
            consumeCharacter();
        }
        m_currentToken = createTextToken(text);
    }
}

void HTMLParser::handleTagOpenState() {
    if (!hasMoreChars()) {
        parseError("EOF in tag open state");
        m_currentToken = createTextToken("<");
        return;
    }
    
    char c = currentChar();
    if (c == '!') {
        m_state = ParserState::MARKUP_DECLARATION_OPEN;
        consumeCharacter();
    } else if (c == '/') {
        m_state = ParserState::END_TAG_OPEN;
        consumeCharacter();
    } else if (std::isalpha(c)) {
        m_currentToken = createStartToken("");
        m_state = ParserState::TAG_NAME;
        reconsume();  // Don't consume the character, reprocess it in TAG_NAME state
    } else if (c == '?') {
        parseError("Unexpected '?' in tag open state");
        m_state = ParserState::BOGUS_COMMENT;
        consumeCharacter();
    } else {
        parseError("Invalid character in tag open state");
        m_currentToken = createTextToken("<");
        reconsume();  // Don't consume the character, reprocess it in DATA state
        m_state = ParserState::DATA;
    }
}

void HTMLParser::handleEndTagOpenState() {
    if (!hasMoreChars()) {
        parseError("EOF in end tag open state");
        m_currentToken = createTextToken("</");
        return;
    }
    
    char c = currentChar();
    if (std::isalpha(c)) {
        m_currentToken = createEndToken("");
        m_state = ParserState::TAG_NAME;
        reconsume();  // Don't consume the character, reprocess it in TAG_NAME state
    } else if (c == '>') {
        parseError("Empty end tag");
        m_state = ParserState::DATA;
        consumeCharacter();
    } else {
        parseError("Invalid character in end tag open state");
        m_state = ParserState::BOGUS_COMMENT;
        reconsume();  // Don't consume the character, reprocess it in BOGUS_COMMENT state
    }
}

void HTMLParser::handleTagNameState() {
    if (!hasMoreChars()) {
        parseError("EOF in tag name state");
        return;
    }
    
    char c = currentChar();
    if (std::isspace(c)) {
        m_state = ParserState::BEFORE_ATTRIBUTE_NAME;
        consumeCharacter();
    } else if (c == '/') {
        m_state = ParserState::SELF_CLOSING_START_TAG;
        consumeCharacter();
    } else if (c == '>') {
        m_state = ParserState::DATA;
        consumeCharacter();
        return;  // Return the token
    } else {
        // Append to tag name
        m_currentToken.name += std::tolower(c);
        consumeCharacter();
    }
}

void HTMLParser::handleBeforeAttributeNameState() {
    if (!hasMoreChars()) {
        parseError("EOF in before attribute name state");
        return;
    }
    
    char c = currentChar();
    if (std::isspace(c)) {
        // Skip whitespace
        consumeCharacter();
    } else if (c == '/' || c == '>') {
        m_state = c == '/' ? ParserState::SELF_CLOSING_START_TAG : ParserState::DATA;
        reconsume();  // Reprocess in the new state
    } else if (c == '=') {
        parseError("Unexpected '=' in before attribute name state");
        m_tempBuffer = "=";  // Start attribute name with =
        m_state = ParserState::ATTRIBUTE_NAME;
        consumeCharacter();
    } else {
        // Start a new attribute
        m_tempBuffer.clear();  // This will hold the attribute name
        m_state = ParserState::ATTRIBUTE_NAME;
        reconsume();  // Reprocess in ATTRIBUTE_NAME state
    }
}

void HTMLParser::handleAttributeNameState() {
    if (!hasMoreChars()) {
        parseError("EOF in attribute name state");
        m_currentToken.attributes[m_tempBuffer] = "";  // Add the attribute
        return;
    }
    
    char c = currentChar();
    if (std::isspace(c) || c == '/' || c == '>') {
        m_currentToken.attributes[m_tempBuffer] = "";  // Add the attribute with empty value
        m_state = ParserState::AFTER_ATTRIBUTE_NAME;
        reconsume();  // Reprocess in the new state
    } else if (c == '=') {
        m_state = ParserState::BEFORE_ATTRIBUTE_VALUE;
        consumeCharacter();
    } else {
        // Append to attribute name, converting to lowercase
        m_tempBuffer += std::tolower(c);
        consumeCharacter();
    }
}

void HTMLParser::handleAfterAttributeNameState() {
    if (!hasMoreChars()) {
        parseError("EOF in after attribute name state");
        return;
    }
    
    char c = currentChar();
    if (std::isspace(c)) {
        // Skip whitespace
        consumeCharacter();
    } else if (c == '/') {
        m_state = ParserState::SELF_CLOSING_START_TAG;
        consumeCharacter();
    } else if (c == '=') {
        m_state = ParserState::BEFORE_ATTRIBUTE_VALUE;
        consumeCharacter();
    } else if (c == '>') {
        m_state = ParserState::DATA;
        consumeCharacter();
        return;  // Return the token
    } else {
        // Start a new attribute
        m_tempBuffer.clear();
        m_state = ParserState::ATTRIBUTE_NAME;
        reconsume();  // Reprocess in ATTRIBUTE_NAME state
    }
}

void HTMLParser::handleBeforeAttributeValueState() {
    if (!hasMoreChars()) {
        parseError("EOF in before attribute value state");
        m_currentToken.attributes[m_tempBuffer] = "";  // Add empty value
        return;
    }
    
    char c = currentChar();
    if (std::isspace(c)) {
        // Skip whitespace
        consumeCharacter();
    } else if (c == '"') {
        m_state = ParserState::ATTRIBUTE_VALUE_DOUBLE_QUOTED;
        m_currentToken.attributes[m_tempBuffer] = "";  // Initialize with empty value
        consumeCharacter();
    } else if (c == '\'') {
        m_state = ParserState::ATTRIBUTE_VALUE_SINGLE_QUOTED;
        m_currentToken.attributes[m_tempBuffer] = "";  // Initialize with empty value
        consumeCharacter();
    } else if (c == '>') {
        parseError("Missing attribute value");
        m_currentToken.attributes[m_tempBuffer] = "";  // Add empty value
        m_state = ParserState::DATA;
        consumeCharacter();
        return;  // Return the token
    } else {
        m_state = ParserState::ATTRIBUTE_VALUE_UNQUOTED;
        m_currentToken.attributes[m_tempBuffer] = "";  // Initialize with empty value
        reconsume();  // Reprocess in ATTRIBUTE_VALUE_UNQUOTED state
    }
}

void HTMLParser::handleAttributeValueDoubleQuotedState() {
    if (!hasMoreChars()) {
        parseError("EOF in attribute value double quoted state");
        return;
    }
    
    char c = currentChar();
    if (c == '"') {
        m_state = ParserState::AFTER_ATTRIBUTE_VALUE_QUOTED;
        consumeCharacter();
    } else {
        // Append to attribute value
        m_currentToken.attributes[m_tempBuffer] += c;
        consumeCharacter();
    }
}

void HTMLParser::handleAttributeValueSingleQuotedState() {
    if (!hasMoreChars()) {
        parseError("EOF in attribute value single quoted state");
        return;
    }
    
    char c = currentChar();
    if (c == '\'') {
        m_state = ParserState::AFTER_ATTRIBUTE_VALUE_QUOTED;
        consumeCharacter();
    } else {
        // Append to attribute value
        m_currentToken.attributes[m_tempBuffer] += c;
        consumeCharacter();
    }
}

void HTMLParser::handleAttributeValueUnquotedState() {
    if (!hasMoreChars()) {
        parseError("EOF in attribute value unquoted state");
        return;
    }
    
    char c = currentChar();
    if (std::isspace(c)) {
        m_state = ParserState::BEFORE_ATTRIBUTE_NAME;
        consumeCharacter();
    } else if (c == '>') {
        m_state = ParserState::DATA;
        consumeCharacter();
        return;  // Return the token
    } else if (c == '"' || c == '\'' || c == '<' || c == '=' || c == '`') {
        parseError("Unexpected character in unquoted attribute value");
        // Append to attribute value anyway
        m_currentToken.attributes[m_tempBuffer] += c;
        consumeCharacter();
    } else {
        // Append to attribute value
        m_currentToken.attributes[m_tempBuffer] += c;
        consumeCharacter();
    }
}

void HTMLParser::handleAfterAttributeValueQuotedState() {
    if (!hasMoreChars()) {
        parseError("EOF in after attribute value quoted state");
        return;
    }
    
    char c = currentChar();
    if (std::isspace(c)) {
        m_state = ParserState::BEFORE_ATTRIBUTE_NAME;
        consumeCharacter();
    } else if (c == '/') {
        m_state = ParserState::SELF_CLOSING_START_TAG;
        consumeCharacter();
    } else if (c == '>') {
        m_state = ParserState::DATA;
        consumeCharacter();
        return;  // Return the token
    } else {
        parseError("Unexpected character after attribute value");
        m_state = ParserState::BEFORE_ATTRIBUTE_NAME;
        reconsume();  // Reprocess in BEFORE_ATTRIBUTE_NAME state
    }
}

void HTMLParser::handleSelfClosingStartTagState() {
    if (!hasMoreChars()) {
        parseError("EOF in self-closing start tag state");
        return;
    }
    
    char c = currentChar();
    if (c == '>') {
        m_currentToken.selfClosing = true;
        m_state = ParserState::DATA;
        consumeCharacter();
        return;  // Return the token
    } else {
        parseError("Unexpected character in self-closing start tag");
        m_state = ParserState::BEFORE_ATTRIBUTE_NAME;
        reconsume();  // Reprocess in BEFORE_ATTRIBUTE_NAME state
    }
}

void HTMLParser::handleBogusCommentState() {
    // Create a comment token
    m_currentToken = createCommentToken("");
    
    // Consume characters until > or EOF
    while (hasMoreChars() && currentChar() != '>') {
        m_currentToken.data += currentChar();
        consumeCharacter();
    }
    
    if (hasMoreChars()) {
        consumeCharacter();  // Consume '>'
    }
    
    m_state = ParserState::DATA;
    return;  // Return the comment token
}

void HTMLParser::handleMarkupDeclarationOpenState() {
    if (!hasMoreChars()) {
        parseError("EOF in markup declaration open state");
        m_currentToken = createCommentToken("");
        return;
    }
    
    // Check for DOCTYPE
    if (m_position + 6 <= m_html.length() && 
        m_html.substr(m_position, 7) == "DOCTYPE") {
        m_position += 7;  // Skip "DOCTYPE"
        m_state = ParserState::DOCTYPE;
        return;
    }
    
    // Check for comment
    if (m_position + 2 <= m_html.length() && 
        m_html.substr(m_position, 2) == "--") {
        m_position += 2;  // Skip "--"
        m_state = ParserState::COMMENT_START;
        m_currentToken = createCommentToken("");
        return;
    }
    
    // Anything else is a bogus comment
    parseError("Invalid markup declaration");
    m_state = ParserState::BOGUS_COMMENT;
    reconsume();  // Reprocess in BOGUS_COMMENT state
}

void HTMLParser::handleCommentStartState() {
    if (!hasMoreChars()) {
        parseError("EOF in comment start state");
        return;
    }
    
    char c = currentChar();
    if (c == '-') {
        m_state = ParserState::COMMENT_END_DASH;
        consumeCharacter();
    } else if (c == '>') {
        parseError("Empty comment");
        m_state = ParserState::DATA;
        consumeCharacter();
        return;  // Return the empty comment token
    } else {
        m_state = ParserState::COMMENT;
        reconsume();  // Reprocess in COMMENT state
    }
}

void HTMLParser::handleCommentState() {
    if (!hasMoreChars()) {
        parseError("EOF in comment state");
        return;
    }
    
    char c = currentChar();
    if (c == '-') {
        m_state = ParserState::COMMENT_END_DASH;
        consumeCharacter();
    } else {
        // Append to comment data
        m_currentToken.data += c;
        consumeCharacter();
    }
}

void HTMLParser::handleCommentEndDashState() {
    if (!hasMoreChars()) {
        parseError("EOF in comment end dash state");
        return;
    }
    
    char c = currentChar();
    if (c == '-') {
        m_state = ParserState::COMMENT_END;
        consumeCharacter();
    } else {
        // Append dash and current character to comment data
        m_currentToken.data += '-';
        m_currentToken.data += c;
        m_state = ParserState::COMMENT;
        consumeCharacter();
    }
}

void HTMLParser::handleCommentEndState() {
    if (!hasMoreChars()) {
        parseError("EOF in comment end state");
        return;
    }
    
    char c = currentChar();
    if (c == '>') {
        m_state = ParserState::DATA;
        consumeCharacter();
        return;  // Return the comment token
    } else if (c == '-') {
        // Another dash, append to comment data
        m_currentToken.data += '-';
        consumeCharacter();
    } else {
        // Append dashes and current character to comment data
        m_currentToken.data += "--";
        m_currentToken.data += c;
        m_state = ParserState::COMMENT;
        consumeCharacter();
    }
}

void HTMLParser::handleDOCTYPEState() {
    if (!hasMoreChars()) {
        parseError("EOF in DOCTYPE state");
        m_currentToken = createDOCTYPEToken("", "", "", true);  // Force quirks
        return;
    }
    
    char c = currentChar();
    if (std::isspace(c)) {
        m_state = ParserState::BEFORE_DOCTYPE_NAME;
        consumeCharacter();
    } else {
        parseError("Missing whitespace before DOCTYPE name");
        reconsume();  // Reprocess in BEFORE_DOCTYPE_NAME state
        m_state = ParserState::BEFORE_DOCTYPE_NAME;
    }
}

void HTMLParser::handleBeforeDOCTYPENameState() {
    if (!hasMoreChars()) {
        parseError("EOF in before DOCTYPE name state");
        m_currentToken = createDOCTYPEToken("", "", "", true);  // Force quirks
        return;
    }
    
    char c = currentChar();
    if (std::isspace(c)) {
        // Skip whitespace
        consumeCharacter();
    } else if (c == '>') {
        parseError("Missing DOCTYPE name");
        m_currentToken = createDOCTYPEToken("", "", "", true);  // Force quirks
        m_state = ParserState::DATA;
        consumeCharacter();
        return;  // Return the DOCTYPE token
    } else {
        // Start DOCTYPE name
        m_tempBuffer.clear();
        m_state = ParserState::DOCTYPE_NAME;
        reconsume();  // Reprocess in DOCTYPE_NAME state
    }
}

void HTMLParser::handleDOCTYPENameState() {
    if (!hasMoreChars()) {
        parseError("EOF in DOCTYPE name state");
        m_currentToken = createDOCTYPEToken(m_tempBuffer, "", "", true);  // Force quirks
        return;
    }
    
    char c = currentChar();
    if (std::isspace(c)) {
        m_state = ParserState::AFTER_DOCTYPE_NAME;
        m_currentToken = createDOCTYPEToken(m_tempBuffer);
        consumeCharacter();
    } else if (c == '>') {
        m_state = ParserState::DATA;
        m_currentToken = createDOCTYPEToken(m_tempBuffer);
        consumeCharacter();
        return;  // Return the DOCTYPE token
    } else {
        // Append to DOCTYPE name, converting to lowercase
        m_tempBuffer += std::tolower(c);
        consumeCharacter();
    }
}

void HTMLParser::handleAfterDOCTYPENameState() {
    if (!hasMoreChars()) {
        parseError("EOF in after DOCTYPE name state");
        m_currentToken.type = TokenType::DOCTYPE;
        m_currentToken.name = m_tempBuffer;
        return;
    }
    
    char c = currentChar();
    if (std::isspace(c)) {
        // Skip whitespace
        consumeCharacter();
    } else if (c == '>') {
        m_state = ParserState::DATA;
        consumeCharacter();
        return;  // Return the DOCTYPE token
    } else {
        // We're not handling PUBLIC or SYSTEM identifiers fully
        // Skip until '>'
        while (hasMoreChars() && currentChar() != '>') {
            consumeCharacter();
        }
        
        if (hasMoreChars()) {
            consumeCharacter();  // Consume '>'
        }
        
        m_state = ParserState::DATA;
        return;  // Return the DOCTYPE token
    }
}

void HTMLParser::processToken(const Token& token) {
    switch (token.type) {
        case TokenType::DOCTYPE:
            insertDoctype(token);
            break;
        case TokenType::START_TAG:
            insertElement(token);
            break;
        case TokenType::END_TAG:
            endTagToken(token);
            break;
        case TokenType::COMMENT:
            insertComment(token);
            break;
        case TokenType::TEXT:
            insertText(token);
            break;
        case TokenType::EOF_TOKEN:
            // Nothing to do for EOF
            break;
    }
}

void HTMLParser::insertElement(const Token& token) {
    // Create new element
    auto element = m_document.document()->createElement(token.name);
    
    // Add attributes
    for (const auto& attr : token.attributes) {
        element->setAttribute(attr.first, attr.second);
    }
    
    // Append to current node
    if (m_currentNode) {
        m_currentNode->appendChild(element);
    }
    
    // Handle self-closing tags
    if (token.selfClosing || isSelfClosingTag(token.name)) {
        // Nothing more to do for self-closing tags
        return;
    }
    
    // Update current node and open elements
    m_currentNode = element.get();
    m_openElements.push(element);
}

void HTMLParser::insertComment(const Token& token) {
    auto comment = m_document.document()->createComment(token.data);
    
    if (m_currentNode) {
        m_currentNode->appendChild(comment);
    }
}

void HTMLParser::insertText(const Token& token) {
    // Skip empty text nodes
    if (token.data.empty()) {
        return;
    }
    
    auto text = m_document.document()->createTextNode(token.data);
    
    if (m_currentNode) {
        m_currentNode->appendChild(text);
    }
}

void HTMLParser::insertDoctype(const Token& token) {
    auto doctype = m_document.document()->createDocumentType(token.name, 
                                                           token.attributes.count("publicid") ? token.attributes.at("publicid") : "",
                                                           token.attributes.count("systemid") ? token.attributes.at("systemid") : "");
    
    m_document.document()->appendChild(doctype);
}

void HTMLParser::endTagToken(const Token& token) {
    // Find matching start tag in open elements stack
    std::string tagName = token.name;
    
    if (m_openElements.empty()) {
        parseError("End tag without matching start tag: " + tagName);
        return;
    }
    
    // Special handling for certain tags
    if (tagName == "body" || tagName == "html") {
        // Find the element
        bool found = false;
        std::stack<std::shared_ptr<Element>> tempStack;
        
        while (!m_openElements.empty()) {
            auto element = m_openElements.top();
            m_openElements.pop();
            
            if (element->tagName() == tagName) {
                found = true;
                break;
            }
            
            tempStack.push(element);
        }
        
        // Restore the stack except for the matched element
        while (!tempStack.empty()) {
            m_openElements.push(tempStack.top());
            tempStack.pop();
        }
        
        if (!found) {
            parseError("End tag without matching start tag: " + tagName);
        }
        
        // Update current node to parent
        if (!m_openElements.empty()) {
            m_currentNode = m_openElements.top().get();
        } else {
            m_currentNode = m_document.document();
        }
        
        return;
    }
    
    // Regular end tag handling
    if (m_openElements.top()->tagName() == tagName) {
        m_openElements.pop();
        
        // Update current node to parent
        if (!m_openElements.empty()) {
            m_currentNode = m_openElements.top().get();
        } else {
            m_currentNode = m_document.document();
        }
    } else {
        // Try to find the matching tag deeper in the stack (mismatched tags)
        std::stack<std::shared_ptr<Element>> tempStack;
        bool found = false;
        
        while (!m_openElements.empty()) {
            auto element = m_openElements.top();
            m_openElements.pop();
            
            if (element->tagName() == tagName) {
                found = true;
                break;
            }
            
            tempStack.push(element);
        }
        
        if (found) {
            // We found a matching tag, but had to close some elements in between
            parseError("Mismatched tags, expected: " + m_openElements.top()->tagName() + ", got: " + tagName);
            
            // Update current node to parent
            if (!m_openElements.empty()) {
                m_currentNode = m_openElements.top().get();
            } else {
                m_currentNode = m_document.document();
            }
        } else {
            // No matching tag found at all, ignore this end tag
            parseError("End tag without matching start tag: " + tagName);
            
            // Restore the stack
            while (!tempStack.empty()) {
                m_openElements.push(tempStack.top());
                tempStack.pop();
            }
        }
    }
}

Token HTMLParser::createStartToken(const std::string& name) {
    Token token;
    token.type = TokenType::START_TAG;
    token.name = name;
    return token;
}

Token HTMLParser::createEndToken(const std::string& name) {
    Token token;
    token.type = TokenType::END_TAG;
    token.name = name;
    return token;
}

Token HTMLParser::createTextToken(const std::string& text) {
    Token token;
    token.type = TokenType::TEXT;
    token.data = text;
    return token;
}

Token HTMLParser::createCommentToken(const std::string& data) {
    Token token;
    token.type = TokenType::COMMENT;
    token.data = data;
    return token;
}

Token HTMLParser::createDOCTYPEToken(const std::string& name, 
                                     const std::string& publicId, 
                                     const std::string& systemId, 
                                     bool forceQuirks) {
    Token token;
    token.type = TokenType::DOCTYPE;
    token.name = name;
    
    if (!publicId.empty()) {
        token.attributes["publicid"] = publicId;
    }
    
    if (!systemId.empty()) {
        token.attributes["systemid"] = systemId;
    }
    
    if (forceQuirks) {
        token.attributes["forceQuirks"] = "true";
    }
    
    return token;
}

bool HTMLParser::isSpecialElement(const std::string& tagName) {
    return kSpecialElements.find(tagName) != kSpecialElements.end();
}

bool HTMLParser::isSelfClosingTag(const std::string& tagName) {
    return kSelfClosingTags.find(tagName) != kSelfClosingTags.end();
}

void HTMLParser::consumeCharacter() {
    if (m_reconsumeCurrentChar) {
        m_reconsumeCurrentChar = false;
    } else if (hasMoreChars()) {
        m_position++;
    }
}

char HTMLParser::currentChar() const {
    if (m_position < m_html.length()) {
        return m_html[m_position];
    }
    return '\0';
}

char HTMLParser::peekNext() const {
    if (m_position + 1 < m_html.length()) {
        return m_html[m_position + 1];
    }
    return '\0';
}

bool HTMLParser::hasMoreChars() const {
    return m_position < m_html.length();
}

void HTMLParser::reconsume() {
    m_reconsumeCurrentChar = true;
}

void HTMLParser::parseError(const std::string& message) {
    m_errors.push_back("Error at position " + std::to_string(m_position) + ": " + message);
    std::cerr << "HTML Parser Error: " << message << " at position " << m_position << std::endl;
}

} // namespace html
} // namespace browser