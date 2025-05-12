#include "css_parser.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <cctype>
#include <map>

namespace browser {
namespace css {

//-----------------------------------------------------------------------------
// Value Implementation
//-----------------------------------------------------------------------------

Value::Value()
    : m_type(ValueType::UNKNOWN)
    , m_numericValue(0.0)
    , m_unit(Unit::NONE)
{
}

Value::Value(const std::string& value)
    : m_type(ValueType::UNKNOWN)
    , m_numericValue(0.0)
    , m_unit(Unit::NONE)
{
    parse(value);
}

Value::~Value() {
}

void Value::parse(const std::string& value) {
    // Store original value
    m_stringValue = value;
    
    // Try to parse as different value types
    if (parseLength(value)) {
        m_type = ValueType::LENGTH;
    } else if (parseColor(value)) {
        m_type = ValueType::COLOR;
    } else if (parsePercentage(value)) {
        m_type = ValueType::PERCENTAGE;
    } else {
        // Default to keyword or string
        m_type = ValueType::KEYWORD;
    }
}

bool Value::parseLength(const std::string& value) {
    std::regex lengthRegex("([0-9]*\\.?[0-9]+)(px|em|rem|vh|vw|%)");
    std::smatch match;
    
    if (std::regex_match(value, match, lengthRegex)) {
        m_numericValue = std::stod(match[1].str());
        
        std::string unit = match[2].str();
        if (unit == "px") {
            m_unit = Unit::PX;
        } else if (unit == "em") {
            m_unit = Unit::EM;
        } else if (unit == "rem") {
            m_unit = Unit::REM;
        } else if (unit == "vh") {
            m_unit = Unit::VH;
        } else if (unit == "vw") {
            m_unit = Unit::VW;
        } else if (unit == "%") {
            m_unit = Unit::PERCENTAGE;
            m_type = ValueType::PERCENTAGE;
        }
        
        return true;
    }
    
    // Check for zero without unit
    std::regex zeroRegex("^(0)$");
    if (std::regex_match(value, match, zeroRegex)) {
        m_numericValue = 0.0;
        m_unit = Unit::PX;
        return true;
    }
    
    return false;
}

bool Value::parseColor(const std::string& value) {
    // Check for hex colors
    std::regex hexRegex("#([0-9a-fA-F]{3}|[0-9a-fA-F]{6})");
    
    // Check for named colors
    static const std::map<std::string, std::string> namedColors = {
        {"black", "#000000"},
        {"white", "#ffffff"},
        {"red", "#ff0000"},
        {"green", "#008000"},
        {"blue", "#0000ff"},
        {"yellow", "#ffff00"},
        {"gray", "#808080"},
        {"purple", "#800080"},
        // Add more named colors as needed
    };
    
    // Check for rgba/rgb
    std::regex rgbaRegex("rgba?\\((\\d+),\\s*(\\d+),\\s*(\\d+)(?:,\\s*([0-9]*\\.?[0-9]+))?\\)");
    
    if (std::regex_match(value, hexRegex)) {
        return true;
    }
    
    auto it = namedColors.find(value);
    if (it != namedColors.end()) {
        return true;
    }
    
    std::smatch match;
    if (std::regex_match(value, match, rgbaRegex)) {
        return true;
    }
    
    return false;
}

bool Value::parsePercentage(const std::string& value) {
    std::regex percentRegex("([0-9]*\\.?[0-9]+)%");
    std::smatch match;
    
    if (std::regex_match(value, match, percentRegex)) {
        m_numericValue = std::stod(match[1].str());
        m_unit = Unit::PERCENTAGE;
        return true;
    }
    
    return false;
}

std::string Value::toString() const {
    return m_stringValue;
}

//-----------------------------------------------------------------------------
// Declaration Implementation
//-----------------------------------------------------------------------------

Declaration::Declaration(const std::string& property, const Value& value)
    : m_property(property)
    , m_value(value)
    , m_important(false)
{
}

Declaration::~Declaration() {
}

//-----------------------------------------------------------------------------
// Selector Implementation
//-----------------------------------------------------------------------------

Selector::Selector() {
}

Selector::Selector(const std::string& selectorText) {
    parse(selectorText);
}

Selector::~Selector() {
}

bool Selector::parse(const std::string& selectorText) {
    // Clear any existing components
    m_components.clear();
    
    // Split by combinators (space, >, +, ~)
    std::string text = selectorText;
    std::string::size_type pos = 0;
    
    // Simple implementation for basic selectors
    // In a real browser, use a proper tokenizer and parser for complex selectors
    
    // Replace multiple spaces with a single space
    std::regex multiSpace("\\s+");
    text = std::regex_replace(text, multiSpace, " ");
    
    // Trim leading/trailing spaces
    text = text.substr(text.find_first_not_of(" "), text.find_last_not_of(" ") - text.find_first_not_of(" ") + 1);
    
    // Check for descendant selectors (space)
    if (text.find(" ") != std::string::npos) {
        // Split by space and create descendant components
        std::istringstream iss(text);
        std::string part;
        std::vector<std::string> parts;
        
        while (std::getline(iss, part, ' ')) {
            if (!part.empty()) {
                parts.push_back(part);
            }
        }
        
        // Create components for each part
        for (size_t i = 0; i < parts.size(); ++i) {
            Component component;
            if (!parseComponent(parts[i], component)) {
                return false;
            }
            
            if (i > 0) {
                // Add a descendant combinator
                Component combinator;
                combinator.type = SelectorType::DESCENDANT;
                combinator.subSelectors.push_back(m_components.back());
                combinator.subSelectors.push_back(component);
                
                // Replace the last component with the combinator
                m_components.pop_back();
                m_components.push_back(combinator);
            } else {
                m_components.push_back(component);
            }
        }
        
        return true;
    }
    
    // Check for direct child selector (>)
    if (text.find(">") != std::string::npos) {
        // Split by > and create child components
        // Similar to descendant implementation
        return true;
    }
    
    // Check for adjacent sibling selector (+)
    if (text.find("+") != std::string::npos) {
        // Similar implementation
        return true;
    }
    
    // Check for general sibling selector (~)
    if (text.find("~") != std::string::npos) {
        // Similar implementation
        return true;
    }
    
    // Simple selector (no combinators)
    Component component;
    return parseComponent(text, component) && (m_components.push_back(component), true);
}

bool Selector::parseComponent(const std::string& text, Component& component) {
    if (text.empty()) {
        return false;
    }
    
    // Check for ID selector
    if (text[0] == '#') {
        component.type = SelectorType::ID;
        component.value = text.substr(1);
        return true;
    }
    
    // Check for class selector
    if (text[0] == '.') {
        component.type = SelectorType::CLASS;
        component.value = text.substr(1);
        return true;
    }
    
    // Check for universal selector
    if (text[0] == '*') {
        component.type = SelectorType::UNIVERSAL;
        return true;
    }
    
    // Check for attribute selector
    if (text[0] == '[' && text.back() == ']') {
        component.type = SelectorType::ATTRIBUTE;
        
        // Extract attribute name and value
        std::string attrText = text.substr(1, text.length() - 2);
        
        // Handle different attribute selectors ([attr], [attr=value], etc.)
        size_t eqPos = attrText.find('=');
        if (eqPos != std::string::npos) {
            component.attributeName = attrText.substr(0, eqPos);
            component.attributeValue = attrText.substr(eqPos + 1);
            
            // Remove quotes if present
            if ((component.attributeValue.front() == '"' && component.attributeValue.back() == '"') ||
                (component.attributeValue.front() == '\'' && component.attributeValue.back() == '\'')) {
                component.attributeValue = component.attributeValue.substr(1, component.attributeValue.length() - 2);
            }
        } else {
            component.attributeName = attrText;
        }
        
        return true;
    }
    
    // Check for pseudo-class and pseudo-element
    if (text[0] == ':') {
        // Double colon for pseudo-element (::before)
        if (text.length() > 1 && text[1] == ':') {
            component.type = SelectorType::PSEUDO_ELEMENT;
            component.value = text.substr(2);
        } else {
            component.type = SelectorType::PSEUDO_CLASS;
            component.value = text.substr(1);
        }
        return true;
    }
    
    // Must be a type selector
    component.type = SelectorType::TYPE;
    component.value = text;
    return true;
}

int Selector::specificity() const {
    // Calculate specificity based on the components
    // a = ID selectors count
    // b = class selectors, attribute selectors, and pseudo-class count
    // c = type selectors and pseudo-elements count
    int a = 0, b = 0, c = 0;
    
    for (const auto& component : m_components) {
        switch (component.type) {
            case SelectorType::ID:
                a++;
                break;
            case SelectorType::CLASS:
            case SelectorType::ATTRIBUTE:
            case SelectorType::PSEUDO_CLASS:
                b++;
                break;
            case SelectorType::TYPE:
            case SelectorType::PSEUDO_ELEMENT:
                c++;
                break;
            case SelectorType::DESCENDANT:
            case SelectorType::CHILD:
            case SelectorType::ADJACENT_SIBLING:
            case SelectorType::GENERAL_SIBLING:
                // Combinators don't affect specificity
                break;
            case SelectorType::UNIVERSAL:
                // Universal selector doesn't affect specificity
                break;
        }
    }
    
    return (a * 100) + (b * 10) + c;
}

bool Selector::matches(html::Element* element) const {
    if (!element) {
        return false;
    }
    
    // Match each component against the element
    for (const auto& component : m_components) {
        switch (component.type) {
            case SelectorType::TYPE:
                if (component.value != element->tagName() && component.value != "*") {
                    return false;
                }
                break;
            case SelectorType::CLASS: {
                std::string classes = element->className();
                std::istringstream iss(classes);
                std::string cls;
                bool found = false;
                
                while (std::getline(iss, cls, ' ')) {
                    if (cls == component.value) {
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    return false;
                }
                break;
            }
            case SelectorType::ID:
                if (element->id() != component.value) {
                    return false;
                }
                break;
            case SelectorType::ATTRIBUTE:
                if (!element->hasAttribute(component.attributeName)) {
                    return false;
                }
                
                if (!component.attributeValue.empty() && 
                    element->getAttribute(component.attributeName) != component.attributeValue) {
                    return false;
                }
                break;
            case SelectorType::DESCENDANT:
                // Not implemented in this simple version
                break;
            case SelectorType::CHILD:
                // Not implemented in this simple version
                break;
            case SelectorType::ADJACENT_SIBLING:
                // Not implemented in this simple version
                break;
            case SelectorType::GENERAL_SIBLING:
                // Not implemented in this simple version
                break;
            case SelectorType::PSEUDO_CLASS:
                // Not implemented in this simple version
                // Would need state information (hover, active, etc.)
                break;
            case SelectorType::PSEUDO_ELEMENT:
                // Not relevant for element matching
                break;
            case SelectorType::UNIVERSAL:
                // Always matches
                break;
        }
    }
    
    return true;
}

std::string Selector::toString() const {
    std::string result;
    
    for (size_t i = 0; i < m_components.size(); ++i) {
        const auto& component = m_components[i];
        
        switch (component.type) {
            case SelectorType::TYPE:
                result += component.value;
                break;
            case SelectorType::CLASS:
                result += "." + component.value;
                break;
            case SelectorType::ID:
                result += "#" + component.value;
                break;
            case SelectorType::UNIVERSAL:
                result += "*";
                break;
            case SelectorType::ATTRIBUTE:
                result += "[" + component.attributeName;
                if (!component.attributeValue.empty()) {
                    result += "=\"" + component.attributeValue + "\"";
                }
                result += "]";
                break;
            case SelectorType::PSEUDO_CLASS:
                result += ":" + component.value;
                break;
            case SelectorType::PSEUDO_ELEMENT:
                result += "::" + component.value;
                break;
            case SelectorType::DESCENDANT:
                if (i > 0) result += " ";
                break;
            case SelectorType::CHILD:
                result += " > ";
                break;
            case SelectorType::ADJACENT_SIBLING:
                result += " + ";
                break;
            case SelectorType::GENERAL_SIBLING:
                result += " ~ ";
                break;
        }
    }
    
    return result;
}

//-----------------------------------------------------------------------------
// StyleRule Implementation
//-----------------------------------------------------------------------------

StyleRule::StyleRule() {
}

StyleRule::~StyleRule() {
}

void StyleRule::addSelector(const Selector& selector) {
    m_selectors.push_back(selector);
}

void StyleRule::addDeclaration(const Declaration& declaration) {
    m_declarations.push_back(declaration);
}

//-----------------------------------------------------------------------------
// StyleSheet Implementation
//-----------------------------------------------------------------------------

StyleSheet::StyleSheet() {
}

StyleSheet::~StyleSheet() {
}

void StyleSheet::addRule(const StyleRule& rule) {
    m_rules.push_back(rule);
}

bool StyleSheet::parse(const std::string& cssText) {
    // Create a CSS parser and parse the stylesheet
    CSSParser parser;
    std::shared_ptr<StyleSheet> sheet = parser.parseStylesheet(cssText);
    
    if (sheet) {
        // Copy rules from the parsed stylesheet
        m_rules = sheet->rules();
        return true;
    }
    
    return false;
}

bool StyleSheet::parseRule(const std::string& ruleText, StyleRule& rule) {
    std::string text = ruleText;
    
    // Split into selector and declaration parts
    size_t bracePos = text.find('{');
    if (bracePos == std::string::npos) {
        return false;
    }
    
    std::string selectorText = text.substr(0, bracePos);
    std::string declarationText = text.substr(bracePos + 1);
    
    // Remove closing brace if present
    size_t closingBrace = declarationText.find('}');
    if (closingBrace != std::string::npos) {
        declarationText = declarationText.substr(0, closingBrace);
    }
    
    // Parse selectors
    std::istringstream selectorStream(selectorText);
    std::string selector;
    
    // Split selectors by comma
    while (std::getline(selectorStream, selector, ',')) {
        // Trim whitespace
        selector = selector.substr(selector.find_first_not_of(" \t\n\r\f\v"), 
                                  selector.find_last_not_of(" \t\n\r\f\v") - selector.find_first_not_of(" \t\n\r\f\v") + 1);
        
        Selector sel(selector);
        rule.addSelector(sel);
    }
    
    // Parse declarations
    return parseDeclarationBlock(declarationText, rule);
}

bool StyleSheet::parseDeclarationBlock(const std::string& blockText, StyleRule& rule) {
    std::string text = blockText;
    
    // Split by semicolons
    std::istringstream declarationStream(text);
    std::string declaration;
    
    while (std::getline(declarationStream, declaration, ';')) {
        // Trim whitespace
        declaration = declaration.substr(declaration.find_first_not_of(" \t\n\r\f\v"), 
                                        declaration.find_last_not_of(" \t\n\r\f\v") - declaration.find_first_not_of(" \t\n\r\f\v") + 1);
        
        if (declaration.empty()) {
            continue;
        }
        
        // Split property and value
        size_t colonPos = declaration.find(':');
        if (colonPos == std::string::npos) {
            // Invalid declaration
            continue;
        }
        
        std::string property = declaration.substr(0, colonPos);
        std::string value = declaration.substr(colonPos + 1);
        
        // Trim whitespace
        property = property.substr(property.find_first_not_of(" \t\n\r\f\v"), 
                                  property.find_last_not_of(" \t\n\r\f\v") - property.find_first_not_of(" \t\n\r\f\v") + 1);
        value = value.substr(value.find_first_not_of(" \t\n\r\f\v"), 
                            value.find_last_not_of(" \t\n\r\f\v") - value.find_first_not_of(" \t\n\r\f\v") + 1);
        
        // Check for !important
        bool important = false;
        size_t importantPos = value.find("!important");
        if (importantPos != std::string::npos) {
            value = value.substr(0, importantPos);
            value = value.substr(0, value.find_last_not_of(" \t\n\r\f\v") + 1);
            important = true;
        }
        
        // Create value and declaration
        Value val(value);
        Declaration decl(property, val);
        decl.setImportant(important);
        
        rule.addDeclaration(decl);
    }
    
    return true;
}

//-----------------------------------------------------------------------------
// CSSParser Implementation
//-----------------------------------------------------------------------------

CSSParser::CSSParser() {
}

CSSParser::~CSSParser() {
}

std::shared_ptr<StyleSheet> CSSParser::parseStylesheet(const std::string& css) {
    auto sheet = std::make_shared<StyleSheet>();
    
    // Tokenize the CSS
    std::vector<Token> tokens = tokenize(css);
    
    // Parse the tokens into rules
    size_t pos = 0;
    while (pos < tokens.size()) {
        // Skip whitespace and comments
        while (pos < tokens.size() && 
              (tokens[pos].type == TokenType::WHITESPACE || 
               tokens[pos].type == TokenType::COMMENT)) {
            pos++;
        }
        
        if (pos >= tokens.size()) {
            break;
        }
        
        // Parse a rule
        StyleRule rule = parseRuleFromTokens(tokens, pos);
        
        // Add the rule to the stylesheet
        if (!rule.selectors().empty() && !rule.declarations().empty()) {
            sheet->addRule(rule);
        }
    }
    
    return sheet;
}

std::vector<Declaration> CSSParser::parseDeclarations(const std::string& css) {
    // Tokenize the CSS
    std::vector<Token> tokens = tokenize(css);
    
    // Parse declarations directly
    size_t pos = 0;
    return parseDeclarationsFromTokens(tokens, pos);
}

std::vector<CSSParser::Token> CSSParser::tokenize(const std::string& css) {
    std::vector<Token> tokens;
    
    // Simple tokenizer for demonstration
    // In a real browser, would use a more sophisticated tokenizer
    
    const char* input = css.c_str();
    size_t length = css.length();
    size_t pos = 0;
    
    while (pos < length) {
        // Skip whitespace
        if (std::isspace(input[pos])) {
            size_t start = pos;
            while (pos < length && std::isspace(input[pos])) {
                pos++;
            }
            tokens.push_back({TokenType::WHITESPACE, css.substr(start, pos - start)});
            continue;
        }
        
        // Comments
        if (pos + 1 < length && input[pos] == '/' && input[pos + 1] == '*') {
            size_t start = pos;
            pos += 2;  // Skip /*
            
            while (pos + 1 < length && !(input[pos] == '*' && input[pos + 1] == '/')) {
                pos++;
            }
            
            if (pos + 1 < length) {
                pos += 2;  // Skip */
            }
            
            tokens.push_back({TokenType::COMMENT, css.substr(start, pos - start)});
            continue;
        }
        
        // Identifiers
        if (std::isalpha(input[pos]) || input[pos] == '_' || input[pos] == '-') {
            size_t start = pos;
            while (pos < length && (std::isalnum(input[pos]) || input[pos] == '_' || input[pos] == '-')) {
                pos++;
            }
            tokens.push_back({TokenType::IDENT, css.substr(start, pos - start)});
            continue;
        }
        
        // Numbers
        if (std::isdigit(input[pos]) || (input[pos] == '.' && pos + 1 < length && std::isdigit(input[pos + 1]))) {
            size_t start = pos;
            if (input[pos] == '.') {
                pos++;
            } else {
                while (pos < length && std::isdigit(input[pos])) {
                    pos++;
                }
                
                if (pos < length && input[pos] == '.') {
                    pos++;
                    while (pos < length && std::isdigit(input[pos])) {
                        pos++;
                    }
                }
            }
            
            // Check for percentage
            if (pos < length && input[pos] == '%') {
                pos++;
                tokens.push_back({TokenType::PERCENTAGE, css.substr(start, pos - start)});
            }
            // Check for dimension
            else if (pos < length && (std::isalpha(input[pos]) || input[pos] == '_' || input[pos] == '-')) {
                size_t unitStart = pos;
                while (pos < length && (std::isalnum(input[pos]) || input[pos] == '_' || input[pos] == '-')) {
                    pos++;
                }
                tokens.push_back({TokenType::DIMENSION, css.substr(start, pos - start)});
            } else {
                tokens.push_back({TokenType::NUMBER, css.substr(start, pos - start)});
            }
            continue;
        }
        
        // Strings
        if (input[pos] == '"' || input[pos] == '\'') {
            char quote = input[pos];
            size_t start = pos;
            pos++;  // Skip the opening quote
            
            while (pos < length && input[pos] != quote) {
                if (input[pos] == '\\' && pos + 1 < length) {
                    pos += 2;  // Skip escape sequence
                } else {
                    pos++;
                }
            }
            
            if (pos < length) {
                pos++;  // Skip the closing quote
            }
            
            tokens.push_back({TokenType::STRING, css.substr(start, pos - start)});
            continue;
        }
        
        // Hash
        if (input[pos] == '#') {
            size_t start = pos;
            pos++;  // Skip #
            
            while (pos < length && (std::isalnum(input[pos]) || input[pos] == '_' || input[pos] == '-')) {
                pos++;
            }
            
            tokens.push_back({TokenType::HASH, css.substr(start, pos - start)});
            continue;
        }
        
        // Delimiters
        switch (input[pos]) {
            case '{':
                tokens.push_back({TokenType::BRACE_OPEN, "{"});
                pos++;
                break;
            case '}':
                tokens.push_back({TokenType::BRACE_CLOSE, "}"});
                pos++;
                break;
            case '(':
                tokens.push_back({TokenType::PAREN_OPEN, "("});
                pos++;
                break;
            case ')':
                tokens.push_back({TokenType::PAREN_CLOSE, ")"});
                pos++;
                break;
            case '[':
                tokens.push_back({TokenType::BRACKET_OPEN, "["});
                pos++;
                break;
            case ']':
                tokens.push_back({TokenType::BRACKET_CLOSE, "]"});
                pos++;
                break;
            case ':':
                tokens.push_back({TokenType::COLON, ":"});
                pos++;
                break;
            case ';':
                tokens.push_back({TokenType::SEMICOLON, ";"});
                pos++;
                break;
            case ',':
                tokens.push_back({TokenType::COMMA, ","});
                pos++;
                break;
            default:
                // Other delimiters
                tokens.push_back({TokenType::DELIM, std::string(1, input[pos])});
                pos++;
                break;
        }
    }
    
    // Add EOF token
    tokens.push_back({TokenType::EOF_TOKEN, ""});
    
    return tokens;
}

StyleRule CSSParser::parseRuleFromTokens(const std::vector<Token>& tokens, size_t& pos) {
    StyleRule rule;
    
    // Parse selectors
    std::vector<Selector> selectors = parseSelectorsFromTokens(tokens, pos);
    for (const auto& selector : selectors) {
        rule.addSelector(selector);
    }
    
    // Skip to opening brace
    while (pos < tokens.size() && tokens[pos].type != TokenType::BRACE_OPEN) {
        pos++;
    }
    
    if (pos >= tokens.size()) {
        return rule;
    }
    
    // Skip opening brace
    pos++;
    
    // Parse declarations
    std::vector<Declaration> declarations = parseDeclarationsFromTokens(tokens, pos);
    for (const auto& declaration : declarations) {
        rule.addDeclaration(declaration);
    }
    
    return rule;
}

std::vector<Selector> CSSParser::parseSelectorsFromTokens(const std::vector<Token>& tokens, size_t& pos) {
    std::vector<Selector> selectors;
    std::string currentSelector;
    
    while (pos < tokens.size() && tokens[pos].type != TokenType::BRACE_OPEN) {
        if (tokens[pos].type == TokenType::COMMA) {
            // End of selector, start a new one
            if (!currentSelector.empty()) {
                Selector selector(currentSelector);
                selectors.push_back(selector);
                currentSelector.clear();
            }
        } else if (tokens[pos].type != TokenType::WHITESPACE) {
            currentSelector += tokens[pos].value;
        } else if (!currentSelector.empty()) {
            // Add a space for descendant selectors, but only if we have content already
            currentSelector += " ";
        }
        
        pos++;
    }
    
    // Add the last selector
    if (!currentSelector.empty()) {
        Selector selector(currentSelector);
        selectors.push_back(selector);
    }
    
    return selectors;
}

std::vector<Declaration> CSSParser::parseDeclarationsFromTokens(const std::vector<Token>& tokens, size_t& pos) {
    std::vector<Declaration> declarations;
    std::string currentProperty;
    std::string currentValue;
    bool inProperty = true;
    bool important = false;
    
    while (pos < tokens.size() && tokens[pos].type != TokenType::BRACE_CLOSE) {
        if (tokens[pos].type == TokenType::WHITESPACE) {
            // Skip whitespace
            pos++;
            continue;
        }
        
        if (tokens[pos].type == TokenType::SEMICOLON) {
            // End of declaration
            if (!currentProperty.empty() && !currentValue.empty()) {
                // Create value and declaration
                Value value(currentValue);
                Declaration declaration(currentProperty, value);
                declaration.setImportant(important);
                
                declarations.push_back(declaration);
            }
            
            // Reset
            currentProperty.clear();
            currentValue.clear();
            inProperty = true;
            important = false;
            
            pos++;
            continue;
        }
        
        if (tokens[pos].type == TokenType::COLON && inProperty) {
            inProperty = false;
            pos++;
            continue;
        }
        
        if (inProperty) {
            currentProperty += tokens[pos].value;
        } else {
            // Check for !important
            if (tokens[pos].type == TokenType::DELIM && tokens[pos].value == "!" &&
                pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::IDENT && 
                tokens[pos + 1].value == "important") {
                important = true;
                pos += 2;
                continue;
            }
            
            currentValue += tokens[pos].value;
        }
        
        pos++;
    }
    
    // Add the last declaration
    if (!currentProperty.empty() && !currentValue.empty()) {
        Value value(currentValue);
        Declaration declaration(currentProperty, value);
        declaration.setImportant(important);
        
        declarations.push_back(declaration);
    }
    
    // Skip closing brace
    if (pos < tokens.size() && tokens[pos].type == TokenType::BRACE_CLOSE) {
        pos++;
    }
    
    return declarations;
}

} // namespace css
} // namespace browser