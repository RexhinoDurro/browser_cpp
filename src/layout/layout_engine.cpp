#include "layout_engine.h"
#include <iostream>
#include <string>
#include <algorithm>

namespace browser {
namespace layout {

LayoutEngine::LayoutEngine()
    : m_layoutRoot(nullptr)
{
}

LayoutEngine::~LayoutEngine() {
}

bool LayoutEngine::initialize() {
    return true;
}

bool LayoutEngine::layoutDocument(html::Document* document, css::StyleResolver* styleResolver,
                                 float viewportWidth, float viewportHeight) {
    if (!document || !styleResolver) {
        return false;
    }
    
    // Clear previous layout
    m_layoutRoot = nullptr;
    m_nodeToBoxMap.clear();
    
    // Make sure styles are resolved
    styleResolver->resolveStyles();
    
    // Build the layout tree
    html::Element* documentElement = document->documentElement();
    if (!documentElement) {
        return false;
    }
    
    m_layoutRoot = buildLayoutTree(documentElement, styleResolver, nullptr);
    if (!m_layoutRoot) {
        return false;
    }
    
    // Perform layout calculations
    calculateLayout(viewportWidth, viewportHeight);
    
    return true;
}

std::shared_ptr<Box> LayoutEngine::buildLayoutTree(html::Node* node, css::StyleResolver* styleResolver, Box* parent) {
    if (!node) {
        return nullptr;
    }
    
    std::shared_ptr<Box> box = nullptr;
    
    if (node->nodeType() == html::NodeType::ELEMENT_NODE) {
        html::Element* element = static_cast<html::Element*>(node);
        
        // Get computed style for this element
        css::ComputedStyle style = styleResolver->getComputedStyle(element);
        
        // Create a box for this element
        box = BoxFactory::createBox(node, style);
        
        // Skip elements with display: none
        if (box && box->displayType() != DisplayType::NONE) {
            // Add to parent
            if (parent) {
                parent->addChild(box);
            }
            
            // Add to node-box mapping
            m_nodeToBoxMap[node] = box.get();
            
            // Process children
            for (const auto& child : element->childNodes()) {
                buildLayoutTree(child.get(), styleResolver, box.get());
            }
        }
    } else if (node->nodeType() == html::NodeType::TEXT_NODE) {
        html::Text* textNode = static_cast<html::Text*>(node);
        
        // For text nodes, get style from parent element
        css::ComputedStyle parentStyle;
        if (parent && parent->element()) {
            parentStyle = styleResolver->getComputedStyle(parent->element());
        }
        
        // Create text box
        box = std::make_shared<TextBox>(textNode, parentStyle);
        
        // Add to parent
        if (parent) {
            parent->addChild(box);
        }
        
        // Add to node-box mapping
        m_nodeToBoxMap[node] = box.get();
    }
    
    return box;
}

void LayoutEngine::calculateLayout(float viewportWidth, float viewportHeight) {
    if (!m_layoutRoot) {
        return;
    }
    
    // Start layout from the root
    m_layoutRoot->layout(viewportWidth);
}

Box* LayoutEngine::getBoxForNode(html::Node* node) const {
    auto it = m_nodeToBoxMap.find(node);
    if (it != m_nodeToBoxMap.end()) {
        return it->second;
    }
    return nullptr;
}

void LayoutEngine::printLayoutTree(std::ostream& stream) const {
    if (!m_layoutRoot) {
        stream << "Empty layout tree" << std::endl;
        return;
    }
    
    stream << "Layout Tree:" << std::endl;
    printLayoutTreeRecursive(stream, m_layoutRoot.get(), 0);
}

void LayoutEngine::printLayoutTreeRecursive(std::ostream& stream, Box* box, int depth) const {
    if (!box) {
        return;
    }
    
    // Indentation
    std::string indent(depth * 2, ' ');
    
    // Box info
    stream << indent;
    
    if (box->element()) {
        stream << "<" << box->element()->tagName() << ">";
    } else if (dynamic_cast<TextBox*>(box)) {
        TextBox* textBox = dynamic_cast<TextBox*>(box);
        std::string text = textBox->textNode()->nodeValue();
        // Truncate long text
        if (text.length() > 20) {
            text = text.substr(0, 17) + "...";
        }
        stream << "\"" << text << "\"";
    } else {
        stream << "[Box]";
    }
    
    // Box geometry
    stream << " x=" << box->contentRect().x << " y=" << box->contentRect().y
           << " width=" << box->contentRect().width << " height=" << box->contentRect().height;
    
    // Display type
    stream << " (";
    switch (box->displayType()) {
        case DisplayType::NONE: stream << "none"; break;
        case DisplayType::BLOCK: stream << "block"; break;
        case DisplayType::INLINE: stream << "inline"; break;
        case DisplayType::INLINE_BLOCK: stream << "inline-block"; break;
        case DisplayType::FLEX: stream << "flex"; break;
        case DisplayType::GRID: stream << "grid"; break;
        case DisplayType::TABLE: stream << "table"; break;
        case DisplayType::TABLE_ROW: stream << "table-row"; break;
        case DisplayType::TABLE_CELL: stream << "table-cell"; break;
    }
    stream << ")";
    
    stream << std::endl;
    
    // Print children
    for (const auto& child : box->children()) {
        printLayoutTreeRecursive(stream, child.get(), depth + 1);
    }
}

} // namespace layout
} // namespace browser