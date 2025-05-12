// style_resolver.h
#ifndef BROWSER_STYLE_RESOLVER_H
#define BROWSER_STYLE_RESOLVER_H

#include "css_parser.h"
#include "../html/dom_tree.h"
#include <map>
#include <string>
#include <memory>

namespace browser {
namespace css {

// Computed style for an element
class ComputedStyle {
public:
    ComputedStyle();
    ~ComputedStyle();
    
    // Get a property value
    Value getProperty(const std::string& property) const;
    
    // Set a property value
    void setProperty(const std::string& property, const Value& value);
    
    // Check if a property exists
    bool hasProperty(const std::string& property) const;
    
    // Get all properties
    const std::map<std::string, Value>& properties() const { return m_properties; }
    
    // Inherit properties from parent style
    void inheritFrom(const ComputedStyle& parentStyle);
    
    // Apply initial values for properties
    void applyInitialValues();
    
private:
    std::map<std::string, Value> m_properties;
    
    // Helper methods
    bool isInherited(const std::string& property) const;
};

// Style resolver class
class StyleResolver {
public:
    StyleResolver();
    ~StyleResolver();
    
    // Set the document to style
    void setDocument(html::Document* document) { m_document = document; }
    
    // Add a stylesheet to the resolver
    void addStyleSheet(const StyleSheet& styleSheet);
    
    // Resolve styles for the entire document
    void resolveStyles();
    
    // Get computed style for an element
    ComputedStyle getComputedStyle(html::Element* element) const;
    
private:
    html::Document* m_document;
    std::vector<StyleSheet> m_styleSheets;
    std::map<html::Element*, ComputedStyle> m_elementStyles;
    
    // Helper methods
    void resolveStyleForElement(html::Element* element, const ComputedStyle& parentStyle);
    void applyMatchingRules(html::Element* element, ComputedStyle& style);
    void applyInlineStyle(html::Element* element, ComputedStyle& style);
    
    // Rule matching
    struct MatchedRule {
        const StyleRule* rule;
        const Selector* selector;
        int specificity;
    };
    
    std::vector<MatchedRule> findMatchingRules(html::Element* element);
    void sortRulesBySpecificity(std::vector<MatchedRule>& rules);
};

} // namespace css
} // namespace browser

#endif // BROWSER_STYLE_RESOLVER_H