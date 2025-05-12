#ifndef BROWSER_BOX_MODEL_H
#define BROWSER_BOX_MODEL_H

#include <string>
#include <memory>
#include <vector>
#include "../html/dom_tree.h"
#include "../css/style_resolver.h"

namespace browser {
namespace layout {

// Rectangle structure for box geometry
struct Rect {
    float x;
    float y;
    float width;
    float height;

    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(float x, float y, float width, float height) 
        : x(x), y(y), width(width), height(height) {}

    // Helper to get bottom coordinate
    float bottom() const { return y + height; }
    // Helper to get right coordinate
    float right() const { return x + width; }
};

// Display type enum
enum class DisplayType {
    NONE,
    BLOCK,
    INLINE,
    INLINE_BLOCK,
    FLEX,
    GRID,
    TABLE,
    TABLE_ROW,
    TABLE_CELL
};

// Position type enum
enum class PositionType {
    STATIC,
    RELATIVE,
    ABSOLUTE,
    FIXED,
    STICKY
};

// Float type enum
enum class FloatType {
    NONE,
    LEFT,
    RIGHT
};

// Box class representing a rendering box in the layout tree
class Box {
public:
    Box(html::Element* element, const css::ComputedStyle& style);
    virtual ~Box();

    // Box hierarchy
    Box* parent() const { return m_parent; }
    void setParent(Box* parent) { m_parent = parent; }
    
    const std::vector<std::shared_ptr<Box>>& children() const { return m_children; }
    void addChild(std::shared_ptr<Box> child);

    // Associated DOM element
    html::Element* element() const { return m_element; }
    
    // Get computed style
    const css::ComputedStyle& style() const { return m_style; }

    // Box geometry
    const Rect& contentRect() const { return m_contentRect; }
    void setContentRect(const Rect& rect) { m_contentRect = rect; }
    
    // Get the full border box (content + padding + border)
    Rect borderBox() const;
    
    // Get the margin box (content + padding + border + margin)
    Rect marginBox() const;

    // Display type
    DisplayType displayType() const { return m_displayType; }
    void setDisplayType(DisplayType type) { m_displayType = type; }

    // Position type
    PositionType positionType() const { return m_positionType; }
    void setPositionType(PositionType type) { m_positionType = type; }

    // Float type
    FloatType floatType() const { return m_floatType; }
    void setFloatType(FloatType type) { m_floatType = type; }

    // Layout calculation methods
    virtual void calculateWidth(float availableWidth);
    virtual void calculatePosition(float x, float y);
    virtual void calculateHeight();
    virtual void layout(float availableWidth);

    // Get margin, border, padding sizes
    float marginTop() const { return m_margin.top; }
    float marginRight() const { return m_margin.right; }
    float marginBottom() const { return m_margin.bottom; }
    float marginLeft() const { return m_margin.left; }

    float borderTop() const { return m_border.top; }
    float borderRight() const { return m_border.right; }
    float borderBottom() const { return m_border.bottom; }
    float borderLeft() const { return m_border.left; }

    float paddingTop() const { return m_padding.top; }
    float paddingRight() const { return m_padding.right; }
    float paddingBottom() const { return m_padding.bottom; }
    float paddingLeft() const { return m_padding.left; }

protected:
    // The associated DOM element
    html::Element* m_element;
    
    // The calculated style
    css::ComputedStyle m_style;
    
    // Box hierarchy
    Box* m_parent;
    std::vector<std::shared_ptr<Box>> m_children;
    
    // Box geometry
    Rect m_contentRect;
    
    // Box properties
    DisplayType m_displayType;
    PositionType m_positionType;
    FloatType m_floatType;

    // Box model values
    struct EdgeSizes {
        float top;
        float right;
        float bottom;
        float left;
        
        EdgeSizes() : top(0), right(0), bottom(0), left(0) {}
    };
    
    EdgeSizes m_margin;
    EdgeSizes m_border;
    EdgeSizes m_padding;

    // Initialize box properties from style
    void initializeBoxProperties();
    
    // Parse dimension values from style
    float parseLength(const css::Value& value, float containerSize, float defaultValue = 0);
};

// Block box implementation
class BlockBox : public Box {
public:
    BlockBox(html::Element* element, const css::ComputedStyle& style);
    virtual ~BlockBox();
    
    // Override layout methods for block layout
    virtual void calculateWidth(float availableWidth) override;
    virtual void calculatePosition(float x, float y) override;
    virtual void calculateHeight() override;
    virtual void layout(float availableWidth) override;
};

// Inline box implementation
class InlineBox : public Box {
public:
    InlineBox(html::Element* element, const css::ComputedStyle& style);
    virtual ~InlineBox();
    
    // Override layout methods for inline layout
    virtual void calculateWidth(float availableWidth) override;
    virtual void calculatePosition(float x, float y) override;
    virtual void calculateHeight() override;
    virtual void layout(float availableWidth) override;
};

// Text box implementation (special inline box for text nodes)
class TextBox : public InlineBox {
public:
    TextBox(html::Text* textNode, const css::ComputedStyle& style);
    virtual ~TextBox();
    
    html::Text* textNode() const { return m_textNode; }
    
    // Override layout methods for text layout
    virtual void calculateWidth(float availableWidth) override;
    virtual void calculateHeight() override;
    
private:
    html::Text* m_textNode;
    std::vector<std::string> m_lines; // Text lines after wrapping
};

// Box factory to create appropriate box types
class BoxFactory {
public:
    static std::shared_ptr<Box> createBox(html::Node* node, const css::ComputedStyle& style);
};

} // namespace layout
} // namespace browser

#endif // BROWSER_BOX_MODEL_H