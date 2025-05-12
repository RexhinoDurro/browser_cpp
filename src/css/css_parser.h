// css_parser.h
#ifndef BROWSER_CSS_PARSER_H
#define BROWSER_CSS_PARSER_H

#include <string>
#include <vector>
#include <memory>
#include "../html/dom_tree.h"

namespace browser {
namespace css {

// Forward declarations
class StyleSheet;
class StyleRule;
class Selector;
class Declaration;

// CSS value types
enum class ValueType {
    KEYWORD,
    LENGTH,
    PERCENTAGE,
    COLOR,
    STRING,
    URL,
    NUMBER,
    ANGLE,
    TIME,
    UNKNOWN
};

// CSS units
enum class Unit {
    PX,
    EM,
    REM,
    VW,
    VH,
    PERCENTAGE,
    NONE  // For unitless values
};

// CSS value class
class Value {
public:
    Value();
    Value(const std::string& value);
    ~Value();
    
    ValueType type() const { return m_type; }
    double numericValue() const { return m_numericValue; }
    std::string stringValue() const { return m_stringValue; }
    Unit unit() const { return m_unit; }
    
    // Parse and set value from string
    void parse(const std::string& value);
    
    // Convert to string
    std::string toString() const;
    
private:
    ValueType m_type;
    std::string m_stringValue;
    double m_numericValue;
    Unit m_unit;
    
    // Helper methods
    bool parseLength(const std::string& value);
    bool parseColor(const std::string& value);
    bool parsePercentage(const std::string& value);
};

// CSS declaration (property-value pair)
class Declaration {
public:
    Declaration(const std::string& property, const Value& value);
    ~Declaration();
    
    std::string property() const { return m_property; }
    Value value() const { return m_value; }
    bool important() const { return m_important; }
    
    // Set importance flag
    void setImportant(bool important) { m_important = important; }
    
private:
    std::string m_property;
    Value m_value;
    bool m_important;
};

// CSS selector types
enum class SelectorType {
    TYPE,       // element
    CLASS,      // .class
    ID,         // #id
    UNIVERSAL,  // *
    ATTRIBUTE,  // [attr]
    PSEUDO_CLASS,  // :hover
    PSEUDO_ELEMENT, // ::before
    DESCENDANT,    // space
    CHILD,         // >
    ADJACENT_SIBLING, // +
    GENERAL_SIBLING   // ~
};

// CSS selector
class Selector {
public:
    Selector();
    Selector(const std::string& selectorText);
    ~Selector();
    
    // Parse selector text
    bool parse(const std::string& selectorText);
    
    // Calculate specificity
    int specificity() const;
    
    // Match selector against an element
    bool matches(html::Element* element) const;
    
    // Convert to string
    std::string toString() const;
    
private:
    // Internal representation of selector components
    struct Component {
        SelectorType type;
        std::string value;
        std::string attributeName;  // For attribute selectors
        std::string attributeValue;  // For attribute selectors
        
        // For combinators
        std::vector<Component> subSelectors;
    };
    
    std::vector<Component> m_components;
    
    // Helper methods
    bool parseComponent(const std::string& text, Component& component);
};

// CSS rule (selector + declarations)
class StyleRule {
public:
    StyleRule();
    ~StyleRule();
    
    // Add a selector
    void addSelector(const Selector& selector);
    
    // Add a declaration
    void addDeclaration(const Declaration& declaration);
    
    // Getters
    const std::vector<Selector>& selectors() const { return m_selectors; }
    const std::vector<Declaration>& declarations() const { return m_declarations; }
    
private:
    std::vector<Selector> m_selectors;
    std::vector<Declaration> m_declarations;
};

// CSS stylesheet (collection of rules)
class StyleSheet {
public:
    StyleSheet();
    ~StyleSheet();
    
    // Add a rule
    void addRule(const StyleRule& rule);
    
    // Parse a complete stylesheet
    bool parse(const std::string& cssText);
    
    // Get rules
    const std::vector<StyleRule>& rules() const { return m_rules; }
    
private:
    std::vector<StyleRule> m_rules;
    
    // Helper methods
    bool parseRule(const std::string& ruleText, StyleRule& rule);
    bool parseDeclarationBlock(const std::string& blockText, StyleRule& rule);
};

// CSS parser class
class CSSParser {
public:
    CSSParser();
    ~CSSParser();
    
    // Parse CSS text into a stylesheet
    std::shared_ptr<StyleSheet> parseStylesheet(const std::string& css);
    
    // Parse a CSS declaration block (like inline style)
    std::vector<Declaration> parseDeclarations(const std::string& css);
    
private:
    // CSS tokenization
    enum class TokenType {
        IDENT,
        STRING,
        NUMBER,
        PERCENTAGE,
        DIMENSION,
        HASH,
        DELIM,
        WHITESPACE,
        COLON,
        SEMICOLON,
        COMMA,
        BRACKET_OPEN,
        BRACKET_CLOSE,
        PAREN_OPEN,
        PAREN_CLOSE,
        BRACE_OPEN,
        BRACE_CLOSE,
        COMMENT,
        AT_KEYWORD,
        FUNCTION,
        URL,
        CDO,
        CDC,
        EOF_TOKEN
    };
    
    struct Token {
        TokenType type;
        std::string value;
    };
    
    // Tokenization methods
    std::vector<Token> tokenize(const std::string& css);
    
    // Parsing helpers
    StyleRule parseRuleFromTokens(const std::vector<Token>& tokens, size_t& pos);
    std::vector<Selector> parseSelectorsFromTokens(const std::vector<Token>& tokens, size_t& pos);
    std::vector<Declaration> parseDeclarationsFromTokens(const std::vector<Token>& tokens, size_t& pos);
};

} // namespace css
} // namespace browser

#endif // BROWSER_CSS_PARSER_H