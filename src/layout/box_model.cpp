#include "box_model.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace browser {
namespace layout {

//-----------------------------------------------------------------------------
// Box Implementation
//-----------------------------------------------------------------------------

Box::Box(html::Element* element, const css::ComputedStyle& style)
    : m_element(element)
    , m_style(style)
    , m_parent(nullptr)
    , m_displayType(DisplayType::BLOCK)
    , m_positionType(PositionType::STATIC)
    , m_floatType(FloatType::NONE)
{
    initializeBoxProperties();
}

Box::~Box() {
}

void Box::addChild(std::shared_ptr<Box> child) {
    if (child) {
        child->setParent(this);
        m_children.push_back(child);
    }
}

Rect Box::borderBox() const {
    return Rect(
        m_contentRect.x - m_padding.left - m_border.left,
        m_contentRect.y - m_padding.top - m_border.top,
        m_contentRect.width + m_padding.left + m_padding.right + m_border.left + m_border.right,
        m_contentRect.height + m_padding.top + m_padding.bottom + m_border.top + m_border.bottom
    );
}

Rect Box::marginBox() const {
    Rect border = borderBox();
    return Rect(
        border.x - m_margin.left,
        border.y - m_margin.top,
        border.width + m_margin.left + m_margin.right,
        border.height + m_margin.top + m_margin.bottom
    );
}

void Box::initializeBoxProperties() {
    // Parse display type
    css::Value displayValue = m_style.getProperty("display");
    std::string displayStr = displayValue.stringValue();
    
    if (displayStr == "none") {
        m_displayType = DisplayType::NONE;
    } else if (displayStr == "block") {
        m_displayType = DisplayType::BLOCK;
    } else if (displayStr == "inline") {
        m_displayType = DisplayType::INLINE;
    } else if (displayStr == "inline-block") {
        m_displayType = DisplayType::INLINE_BLOCK;
    } else if (displayStr == "flex") {
        m_displayType = DisplayType::FLEX;
    } else if (displayStr == "grid") {
        m_displayType = DisplayType::GRID;
    } else if (displayStr == "table") {
        m_displayType = DisplayType::TABLE;
    } else if (displayStr == "table-row") {
        m_displayType = DisplayType::TABLE_ROW;
    } else if (displayStr == "table-cell") {
        m_displayType = DisplayType::TABLE_CELL;
    } else {
        // Default to block if not recognized
        m_displayType = DisplayType::BLOCK;
    }
    
    // Parse position type
    css::Value positionValue = m_style.getProperty("position");
    std::string positionStr = positionValue.stringValue();
    
    if (positionStr == "static") {
        m_positionType = PositionType::STATIC;
    } else if (positionStr == "relative") {
        m_positionType = PositionType::RELATIVE;
    } else if (positionStr == "absolute") {
        m_positionType = PositionType::ABSOLUTE;
    } else if (positionStr == "fixed") {
        m_positionType = PositionType::FIXED;
    } else if (positionStr == "sticky") {
        m_positionType = PositionType::STICKY;
    } else {
        // Default to static if not recognized
        m_positionType = PositionType::STATIC;
    }
    
    // Parse float type
    css::Value floatValue = m_style.getProperty("float");
    std::string floatStr = floatValue.stringValue();
    
    if (floatStr == "left") {
        m_floatType = FloatType::LEFT;
    } else if (floatStr == "right") {
        m_floatType = FloatType::RIGHT;
    } else {
        m_floatType = FloatType::NONE;
    }
    
    // Parse margins
    float containerWidth = m_parent ? m_parent->contentRect().width : 0;
    
    m_margin.top = parseLength(m_style.getProperty("margin-top"), containerWidth);
    m_margin.right = parseLength(m_style.getProperty("margin-right"), containerWidth);
    m_margin.bottom = parseLength(m_style.getProperty("margin-bottom"), containerWidth);
    m_margin.left = parseLength(m_style.getProperty("margin-left"), containerWidth);
    
    // Parse borders
    m_border.top = parseLength(m_style.getProperty("border-top-width"), containerWidth);
    m_border.right = parseLength(m_style.getProperty("border-right-width"), containerWidth);
    m_border.bottom = parseLength(m_style.getProperty("border-bottom-width"), containerWidth);
    m_border.left = parseLength(m_style.getProperty("border-left-width"), containerWidth);
    
    // Parse padding
    m_padding.top = parseLength(m_style.getProperty("padding-top"), containerWidth);
    m_padding.right = parseLength(m_style.getProperty("padding-right"), containerWidth);
    m_padding.bottom = parseLength(m_style.getProperty("padding-bottom"), containerWidth);
    m_padding.left = parseLength(m_style.getProperty("padding-left"), containerWidth);
}

float Box::parseLength(const css::Value& value, float containerSize, float defaultValue) {
    if (value.type() == css::ValueType::LENGTH) {
        float numValue = value.numericValue();
        
        // Handle different units
        switch (value.unit()) {
            case css::Unit::PX:
                return numValue;
            case css::Unit::EM:
                // For simplicity, assuming 1em = 16px
                return numValue * 16.0f;
            case css::Unit::REM:
                // For simplicity, assuming 1rem = 16px
                return numValue * 16.0f;
            case css::Unit::VW:
                // Viewport width percentage (container width as approximation)
                return numValue * containerSize / 100.0f;
            case css::Unit::VH:
                // Viewport height percentage (not accurately calculable here)
                return numValue * 600.0f / 100.0f; // Assuming 600px viewport height
            case css::Unit::PERCENTAGE:
                return numValue * containerSize / 100.0f;
            default:
                return numValue;
        }
    } else if (value.type() == css::ValueType::PERCENTAGE) {
        return value.numericValue() * containerSize / 100.0f;
    }
    
    return defaultValue;
}

void Box::calculateWidth(float availableWidth) {
    // Default implementation for width calculation
    css::Value widthValue = m_style.getProperty("width");
    
    float width = availableWidth;
    
    if (widthValue.stringValue() != "auto") {
        width = parseLength(widthValue, availableWidth);
    }
    
    // Account for padding and border
    width -= (m_padding.left + m_padding.right + m_border.left + m_border.right);
    
    // Ensure width is at least 0
    width = std::max(0.0f, width);
    
    m_contentRect.width = width;
}

void Box::calculatePosition(float x, float y) {
    m_contentRect.x = x + m_margin.left + m_border.left + m_padding.left;
    m_contentRect.y = y + m_margin.top + m_border.top + m_padding.top;
}

void Box::calculateHeight() {
    // Default implementation for height calculation
    css::Value heightValue = m_style.getProperty("height");
    
    float height = 0;
    
    if (heightValue.stringValue() != "auto") {
        height = parseLength(heightValue, m_parent ? m_parent->contentRect().height : 0);
    } else {
        // For auto height, use the heights of children
        for (const auto& child : m_children) {
            height = std::max(height, child->marginBox().bottom() - m_contentRect.y);
        }
    }
    
    m_contentRect.height = height;
}

void Box::layout(float availableWidth) {
    // Default layout implementation
    calculateWidth(availableWidth);
    calculatePosition(0, 0);
    
    // Layout children
    for (const auto& child : m_children) {
        child->layout(m_contentRect.width);
    }
    
    calculateHeight();
}

//-----------------------------------------------------------------------------
// BlockBox Implementation
//-----------------------------------------------------------------------------

BlockBox::BlockBox(html::Element* element, const css::ComputedStyle& style)
    : Box(element, style)
{
}

BlockBox::~BlockBox() {
}

void BlockBox::calculateWidth(float availableWidth) {
    css::Value widthValue = m_style.getProperty("width");
    float containerWidth = availableWidth;
    
    // Calculate width based on the CSS width property
    float width = containerWidth - m_margin.left - m_margin.right;
    
    if (widthValue.stringValue() != "auto") {
        width = parseLength(widthValue, containerWidth);
    }
    
    // Set the content width
    m_contentRect.width = width;
}

void BlockBox::calculatePosition(float x, float y) {
    Box::calculatePosition(x, y);
}

void BlockBox::calculateHeight() {
    css::Value heightValue = m_style.getProperty("height");
    float containerHeight = m_parent ? m_parent->contentRect().height : 0;
    
    // Calculate height based on the CSS height property
    if (heightValue.stringValue() != "auto") {
        m_contentRect.height = parseLength(heightValue, containerHeight);
    } else {
        // For auto height, calculate based on children
        float maxChildBottom = m_contentRect.y;
        
        for (const auto& child : m_children) {
            maxChildBottom = std::max(maxChildBottom, child->marginBox().bottom());
        }
        
        m_contentRect.height = maxChildBottom - m_contentRect.y;
    }
}

void BlockBox::layout(float availableWidth) {
    // Calculate the box dimensions
    calculateWidth(availableWidth);
    calculatePosition(m_parent ? m_parent->contentRect().x : 0, 
                      m_parent ? m_parent->contentRect().y + m_parent->contentRect().height : 0);
    
    // Layout children
    float y = m_contentRect.y;
    
    for (const auto& child : m_children) {
        child->layout(m_contentRect.width);
        
        // Position the child
        if (child->displayType() == DisplayType::BLOCK) {
            child->calculatePosition(m_contentRect.x, y);
            y = child->marginBox().bottom();
        }
    }
    
    // Calculate the final height
    calculateHeight();
}

//-----------------------------------------------------------------------------
// InlineBox Implementation
//-----------------------------------------------------------------------------

InlineBox::InlineBox(html::Element* element, const css::ComputedStyle& style)
    : Box(element, style)
{
    m_displayType = DisplayType::INLINE;
}

InlineBox::~InlineBox() {
}

void InlineBox::calculateWidth(float availableWidth) {
    // For inline elements, width is based on content and cannot be set directly
    float width = 0;
    
    // Sum the widths of children for inline elements
    for (const auto& child : m_children) {
        width += child->marginBox().width;
    }
    
    // Add padding and border
    m_contentRect.width = width;
}

void InlineBox::calculatePosition(float x, float y) {
    Box::calculatePosition(x, y);
}

void InlineBox::calculateHeight() {
    // For inline elements, height is determined by line height
    css::Value lineHeightValue = m_style.getProperty("line-height");
    float lineHeight = 1.2f * 16.0f; // Default line height: 1.2em
    
    if (lineHeightValue.type() == css::ValueType::LENGTH || 
        lineHeightValue.type() == css::ValueType::NUMBER) {
        lineHeight = parseLength(lineHeightValue, 0, lineHeight);
    }
    
    // For inline boxes with children, use the max height of children
    float maxHeight = 0;
    for (const auto& child : m_children) {
        maxHeight = std::max(maxHeight, child->marginBox().height);
    }
    
    m_contentRect.height = std::max(lineHeight, maxHeight);
}

void InlineBox::layout(float availableWidth) {
    // Calculate dimensions
    calculateWidth(availableWidth);
    calculatePosition(m_parent ? m_parent->contentRect().x : 0, 
                     m_parent ? m_parent->contentRect().y : 0);
    
    // Layout children in a horizontal line
    float x = m_contentRect.x;
    
    for (const auto& child : m_children) {
        child->layout(availableWidth - (x - m_contentRect.x));
        child->calculatePosition(x, m_contentRect.y);
        x += child->marginBox().width;
    }
    
    // Calculate final height
    calculateHeight();
}

//-----------------------------------------------------------------------------
// TextBox Implementation
//-----------------------------------------------------------------------------

TextBox::TextBox(html::Text* textNode, const css::ComputedStyle& style)
    : InlineBox(nullptr, style)
    , m_textNode(textNode)
{
}

TextBox::~TextBox() {
}

void TextBox::calculateWidth(float availableWidth) {
    if (!m_textNode) {
        m_contentRect.width = 0;
        return;
    }
    
    // Get the text content
    std::string text = m_textNode->nodeValue();
    
    // Get the font properties
    css::Value fontSizeValue = m_style.getProperty("font-size");
    float fontSize = 16.0f; // Default font size
    
    if (fontSizeValue.type() == css::ValueType::LENGTH) {
        fontSize = parseLength(fontSizeValue, 0, fontSize);
    }
    
    // Simple text width estimation (very simplified)
    // In a real browser, this would use font metrics
    float averageCharWidth = fontSize * 0.5f;
    m_contentRect.width = text.length() * averageCharWidth;
    
    // Text wrapping (simplified)
    if (m_contentRect.width > availableWidth && availableWidth > 0) {
        // Approximate how many characters fit on each line
        int charsPerLine = std::max(1, static_cast<int>(availableWidth / averageCharWidth));
        m_lines.clear();
        
        for (size_t i = 0; i < text.length(); i += charsPerLine) {
            m_lines.push_back(text.substr(i, charsPerLine));
        }
        
        m_contentRect.width = availableWidth;
    } else {
        m_lines = {text};
    }
}

void TextBox::calculateHeight() {
    // Get line height
    css::Value lineHeightValue = m_style.getProperty("line-height");
    float lineHeight = 1.2f * 16.0f; // Default line height
    
    if (lineHeightValue.type() == css::ValueType::LENGTH || 
        lineHeightValue.type() == css::ValueType::NUMBER) {
        lineHeight = parseLength(lineHeightValue, 0, lineHeight);
    }
    
    // Height is the line height times the number of lines
    m_contentRect.height = lineHeight * m_lines.size();
}

//-----------------------------------------------------------------------------
// BoxFactory Implementation
//-----------------------------------------------------------------------------

std::shared_ptr<Box> BoxFactory::createBox(html::Node* node, const css::ComputedStyle& style) {
    if (!node) {
        return nullptr;
    }
    
    if (node->nodeType() == html::NodeType::ELEMENT_NODE) {
        html::Element* element = static_cast<html::Element*>(node);
        
        // Determine display type
        css::Value displayValue = style.getProperty("display");
        std::string displayStr = displayValue.stringValue();
        
        if (displayStr == "inline" || displayStr == "inline-block") {
            return std::make_shared<InlineBox>(element, style);
        } else {
            // Default to block for most elements
            return std::make_shared<BlockBox>(element, style);
        }
    } else if (node->nodeType() == html::NodeType::TEXT_NODE) {
        html::Text* textNode = static_cast<html::Text*>(node);
        return std::make_shared<TextBox>(textNode, style);
    }
    
    // Default to a generic box for other node types
    return std::make_shared<Box>(nullptr, style);
}

} // namespace layout
} // namespace browser