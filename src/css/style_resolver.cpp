#include "style_resolver.h"
#include <algorithm>
#include <iostream>

namespace browser {
namespace css {

//-----------------------------------------------------------------------------
// ComputedStyle Implementation
//-----------------------------------------------------------------------------

ComputedStyle::ComputedStyle() {
}

ComputedStyle::~ComputedStyle() {
}

Value ComputedStyle::getProperty(const std::string& property) const {
    auto it = m_properties.find(property);
    if (it != m_properties.end()) {
        return it->second;
    }
    
    // Return a default value
    return Value();
}

void ComputedStyle::setProperty(const std::string& property, const Value& value) {
    m_properties[property] = value;
}

bool ComputedStyle::hasProperty(const std::string& property) const {
    return m_properties.find(property) != m_properties.end();
}

void ComputedStyle::inheritFrom(const ComputedStyle& parentStyle) {
    // Inherit properties from parent style if they are inheritable
    for (const auto& prop : parentStyle.properties()) {
        if (isInherited(prop.first) && !hasProperty(prop.first)) {
            setProperty(prop.first, prop.second);
        }
    }
}

void ComputedStyle::applyInitialValues() {
    // Set initial values for common CSS properties
    // This is a simplified list, a real browser would have many more properties
    
    if (!hasProperty("display")) {
        setProperty("display", Value("inline"));
    }
    
    if (!hasProperty("color")) {
        setProperty("color", Value("black"));
    }
    
    if (!hasProperty("background-color")) {
        setProperty("background-color", Value("transparent"));
    }
    
    if (!hasProperty("font-family")) {
        setProperty("font-family", Value("Times New Roman"));
    }
    
    if (!hasProperty("font-size")) {
        setProperty("font-size", Value("16px"));
    }
    
    if (!hasProperty("font-weight")) {
        setProperty("font-weight", Value("normal"));
    }
    
    if (!hasProperty("text-align")) {
        setProperty("text-align", Value("left"));
    }
    
    if (!hasProperty("margin-top")) {
        setProperty("margin-top", Value("0"));
    }
    
    if (!hasProperty("margin-right")) {
        setProperty("margin-right", Value("0"));
    }
    
    if (!hasProperty("margin-bottom")) {
        setProperty("margin-bottom", Value("0"));
    }
    
    if (!hasProperty("margin-left")) {
        setProperty("margin-left", Value("0"));
    }
    
    if (!hasProperty("padding-top")) {
        setProperty("padding-top", Value("0"));
    }
    
    if (!hasProperty("padding-right")) {
        setProperty("padding-right", Value("0"));
    }
    
    if (!hasProperty("padding-bottom")) {
        setProperty("padding-bottom", Value("0"));
    }
    
    if (!hasProperty("padding-left")) {
        setProperty("padding-left", Value("0"));
    }
    
    if (!hasProperty("border-top-width")) {
        setProperty("border-top-width", Value("0"));
    }
    
    if (!hasProperty("border-right-width")) {
        setProperty("border-right-width", Value("0"));
    }
    
    if (!hasProperty("border-bottom-width")) {
        setProperty("border-bottom-width", Value("0"));
    }
    
    if (!hasProperty("border-left-width")) {
        setProperty("border-left-width", Value("0"));
    }
    
    if (!hasProperty("width")) {
        setProperty("width", Value("auto"));
    }
    
    if (!hasProperty("height")) {
        setProperty("height", Value("auto"));
    }
    
    if (!hasProperty("position")) {
        setProperty("position", Value("static"));
    }
}

bool ComputedStyle::isInherited(const std::string& property) const {
    // List of properties that are inherited
    // This is a simplified list, a real browser would have many more properties
    static const std::vector<std::string> inheritedProperties = {
        "color",
        "font-family",
        "font-size",
        "font-weight",
        "line-height",
        "list-style",
        "text-align",
        "text-indent",
        "text-transform",
        "visibility",
        "white-space",
        "word-spacing"
    };
    
    return std::find(inheritedProperties.begin(), inheritedProperties.end(), property) != inheritedProperties.end();
}

//-----------------------------------------------------------------------------
// StyleResolver Implementation
//-----------------------------------------------------------------------------

StyleResolver::StyleResolver()
    : m_document(nullptr)
{
}

StyleResolver::~StyleResolver() {
}

void StyleResolver::addStyleSheet(const StyleSheet& styleSheet) {
    m_styleSheets.push_back(styleSheet);
}

void StyleResolver::resolveStyles() {
    if (!m_document) {
        return;
    }
    
    // Clear previous styles
    m_elementStyles.clear();
    
    // Start with the root element and apply styles recursively
    html::Element* root = m_document->documentElement();
    if (root) {
        // Create a default style for the root
        ComputedStyle rootStyle;
        rootStyle.applyInitialValues();
        
        resolveStyleForElement(root, rootStyle);
    }
}

ComputedStyle StyleResolver::getComputedStyle(html::Element* element) const {
    auto it = m_elementStyles.find(element);
    if (it != m_elementStyles.end()) {
        return it->second;
    }
    
    // Return a default style if not found
    ComputedStyle defaultStyle;
    defaultStyle.applyInitialValues();
    return defaultStyle;
}

void StyleResolver::resolveStyleForElement(html::Element* element, const ComputedStyle& parentStyle) {
    if (!element) {
        return;
    }
    
    // Create a new style object for this element
    ComputedStyle style;
    
    // 1. Apply initial values
    style.applyInitialValues();

    std::string tagName = element->tagName();
    if (tagName == "div" || tagName == "p" || tagName == "h1" || tagName == "h2" || 
        tagName == "ul" || tagName == "li" || tagName == "body" || tagName == "html") {
        style.setProperty("display", Value("block"));
    } else if (tagName == "span" || tagName == "a" || tagName == "strong" || tagName == "em") {
        style.setProperty("display", Value("inline"));
    }
    
    
    // 2. Inherit from parent style
    style.inheritFrom(parentStyle);
    
    // 3. Apply matching style rules
    applyMatchingRules(element, style);
    
    // 4. Apply inline style (highest precedence)
    applyInlineStyle(element, style);
    
    // Store the computed style
    m_elementStyles[element] = style;
    
    // Process children
    for (const auto& child : element->childNodes()) {
        if (child->nodeType() == html::NodeType::ELEMENT_NODE) {
            resolveStyleForElement(static_cast<html::Element*>(child.get()), style);
        }
    }
}

void StyleResolver::applyMatchingRules(html::Element* element, ComputedStyle& style) {
    // Find all matching rules
    std::vector<MatchedRule> matchedRules = findMatchingRules(element);
    
    // Sort by specificity
    sortRulesBySpecificity(matchedRules);
    
    // Apply declarations in order of specificity
    for (const auto& match : matchedRules) {
        for (const auto& declaration : match.rule->declarations()) {
            // Skip if a higher specificity rule has already set this property
            // or if the current declaration is !important
            if (!style.hasProperty(declaration.property()) || declaration.important()) {
                style.setProperty(declaration.property(), declaration.value());
            }
        }
    }
}

void StyleResolver::applyInlineStyle(html::Element* element, ComputedStyle& style) {
    // Check for the style attribute
    if (element->hasAttribute("style")) {
        std::string inlineStyle = element->getAttribute("style");
        
        // Parse the inline style
        CSSParser parser;
        std::vector<Declaration> declarations = parser.parseDeclarations(inlineStyle);
        
        // Apply declarations
        for (const auto& declaration : declarations) {
            style.setProperty(declaration.property(), declaration.value());
        }
    }
}

std::vector<StyleResolver::MatchedRule> StyleResolver::findMatchingRules(html::Element* element) {
    std::vector<MatchedRule> matchedRules;
    
    // Check all style sheets
    for (const auto& styleSheet : m_styleSheets) {
        for (const auto& rule : styleSheet.rules()) {
            for (const auto& selector : rule.selectors()) {
                if (selector.matches(element)) {
                    matchedRules.push_back({&rule, &selector, selector.specificity()});
                    break;  // One matching selector per rule is enough
                }
            }
        }
    }
    
    return matchedRules;
}

void StyleResolver::sortRulesBySpecificity(std::vector<MatchedRule>& rules) {
    std::sort(rules.begin(), rules.end(), [](const MatchedRule& a, const MatchedRule& b) {
        return a.specificity < b.specificity;
    });
}

} // namespace css
} // namespace browser