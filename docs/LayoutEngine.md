# Layout Engine Documentation

## Overview

The layout engine transforms the styled DOM tree into a layout tree with calculated positions and dimensions. It implements the CSS box model and handles different layout modes.

## Architecture

### Core Components

```cpp
namespace browser::layout {
    class LayoutEngine {
        // Main layout coordinator
        bool layoutDocument(html::Document* document, 
                          css::StyleResolver* styleResolver,
                          float viewportWidth, 
                          float viewportHeight);
    };
    
    class Box {
        // Base class for all layout boxes
        Rect contentRect() const;
        Rect borderBox() const;
        Rect marginBox() const;
    };
    
    class BlockBox : public Box {
        // Block-level layout
    };
    
    class InlineBox : public Box {
        // Inline layout
    };
}
```

## Box Model

### Box Structure

```
┌─────────────────────────────────────┐
│            Margin Box               │
│  ┌─────────────────────────────┐   │
│  │        Border Box           │   │
│  │  ┌─────────────────────┐   │   │
│  │  │    Padding Box      │   │   │
│  │  │  ┌─────────────┐   │   │   │
│  │  │  │Content Box  │   │   │   │
│  │  │  └─────────────┘   │   │   │
│  │  └─────────────────────┘   │   │
│  └─────────────────────────────┘   │
└─────────────────────────────────────┘
```

### Box Dimensions

```cpp
struct Rect {
    float x, y;           // Position
    float width, height;  // Dimensions
    
    float bottom() const { return y + height; }
    float right() const { return x + width; }
};

class Box {
    Rect m_contentRect;   // Content area
    EdgeSizes m_margin;   // Margin sizes
    EdgeSizes m_border;   // Border widths
    EdgeSizes m_padding;  // Padding sizes
};
```

## Layout Process

### 1. Layout Tree Construction

```cpp
std::shared_ptr<Box> LayoutEngine::buildLayoutTree(
    html::Node* node, 
    css::StyleResolver* styleResolver, 
    Box* parent) 
{
    // Get computed style
    css::ComputedStyle style = styleResolver->getComputedStyle(element);
    
    // Create appropriate box type
    auto box = BoxFactory::createBox(node, style);
    
    // Skip display:none elements
    if (box->displayType() == DisplayType::NONE) {
        return nullptr;
    }
    
    // Process children
    for (const auto& child : element->childNodes()) {
        buildLayoutTree(child.get(), styleResolver, box.get());
    }
    
    return box;
}
```

### 2. Width Calculation

```cpp
void BlockBox::calculateWidth(float availableWidth) {
    css::Value widthValue = m_style.getProperty("width");
    
    if (widthValue.stringValue() == "auto") {
        // Fill available width minus margins
        m_contentRect.width = availableWidth 
                            - m_margin.left 
                            - m_margin.right
                            - m_border.left
                            - m_border.right
                            - m_padding.left
                            - m_padding.right;
    } else {
        // Use specified width
        m_contentRect.width = parseLength(widthValue, availableWidth);
    }
}
```

### 3. Position Calculation

```cpp
void Box::calculatePosition(float x, float y) {
    // Account for margin, border, and padding
    m_contentRect.x = x + m_margin.left + m_border.left + m_padding.left;
    m_contentRect.y = y + m_margin.top + m_border.top + m_padding.top;
}
```

### 4. Height Calculation

```cpp
void BlockBox::calculateHeight() {
    css::Value heightValue = m_style.getProperty("height");
    
    if (heightValue.stringValue() == "auto") {
        // Height based on children
        float maxChildBottom = m_contentRect.y;
        
        for (const auto& child : m_children) {
            maxChildBottom = std::max(maxChildBottom, 
                                    child->marginBox().bottom());
        }
        
        m_contentRect.height = maxChildBottom - m_contentRect.y;
    } else {
        // Use specified height
        m_contentRect.height = parseLength(heightValue, containerHeight);
    }
}
```

## Layout Modes

### Block Layout

Block boxes stack vertically:

```cpp
void BlockBox::layout(float availableWidth) {
    calculateWidth(availableWidth);
    calculatePosition(parentX, parentY);
    
    // Layout children vertically
    float y = m_contentRect.y;
    
    for (const auto& child : m_children) {
        if (child->displayType() == DisplayType::BLOCK) {
            child->layout(m_contentRect.width);
            child->calculatePosition(m_contentRect.x, y);
            y = child->marginBox().bottom();
        }
    }
    
    calculateHeight();
}
```

### Inline Layout

Inline boxes flow horizontally:

```cpp
void InlineBox::layout(float availableWidth) {
    float x = m_contentRect.x;
    float lineHeight = getLineHeight();
    
    for (const auto& child : m_children) {
        child->layout(availableWidth - (x - m_contentRect.x));
        
        // Check if child fits on current line
        if (x + child->marginBox().width > availableWidth) {
            // Wrap to next line
            x = m_contentRect.x;
            y += lineHeight;
        }
        
        child->calculatePosition(x, y);
        x += child->marginBox().width;
    }
}
```

### Text Layout

```cpp
class TextBox : public InlineBox {
    void calculateWidth(float availableWidth) {
        std::string text = m_textNode->nodeValue();
        float fontSize = getFontSize();
        float charWidth = fontSize * 0.5f; // Approximation
        
        // Simple text wrapping
        if (text.length() * charWidth > availableWidth) {
            int charsPerLine = availableWidth / charWidth;
            wrapText(text, charsPerLine);
        }
        
        m_contentRect.width = std::min(text.length() * charWidth, 
                                      availableWidth);
    }
};
```

## Display Types

### Supported Display Values

```cpp
enum class DisplayType {
    NONE,         // Not displayed
    BLOCK,        // Block-level box
    INLINE,       // Inline box
    INLINE_BLOCK, // Inline-level block container
    FLEX,         // Flex container (partial)
    GRID,         // Grid container (planned)
    TABLE,        // Table (planned)
};
```

### Position Types

```cpp
enum class PositionType {
    STATIC,    // Normal flow
    RELATIVE,  // Offset from normal position
    ABSOLUTE,  // Out of flow, positioned
    FIXED,     // Out of flow, viewport positioned
    STICKY     // Hybrid positioning (planned)
};
```

### Float Types

```cpp
enum class FloatType {
    NONE,   // No floating
    LEFT,   // Float to left
    RIGHT   // Float to right
};
```

## Box Factory

### Creating Boxes

```cpp
std::shared_ptr<Box> BoxFactory::createBox(
    html::Node* node, 
    const css::ComputedStyle& style) 
{
    if (node->nodeType() == html::NodeType::TEXT_NODE) {
        return std::make_shared<TextBox>(
            static_cast<html::Text*>(node), style);
    }
    
    css::Value display = style.getProperty("display");
    
    if (display.stringValue() == "inline") {
        return std::make_shared<InlineBox>(element, style);
    } else {
        return std::make_shared<BlockBox>(element, style);
    }
}
```

## Length Parsing

### Supported Units

```cpp
float Box::parseLength(const css::Value& value, float containerSize) {
    switch (value.unit()) {
        case css::Unit::PX:
            return value.numericValue();
            
        case css::Unit::EM:
            return value.numericValue() * getFontSize();
            
        case css::Unit::REM:
            return value.numericValue() * getRootFontSize();
            
        case css::Unit::PERCENTAGE:
            return value.numericValue() * containerSize / 100.0f;
            
        case css::Unit::VW:
            return value.numericValue() * viewportWidth / 100.0f;
            
        case css::Unit::VH:
            return value.numericValue() * viewportHeight / 100.0f;
    }
}
```

## Usage Examples

### Basic Layout

```cpp
// Create layout engine
LayoutEngine engine;
engine.initialize();

// Perform layout
float viewportWidth = 1024;
float viewportHeight = 768;
engine.layoutDocument(document, styleResolver, 
                     viewportWidth, viewportHeight);

// Access layout tree
auto layoutRoot = engine.layoutRoot();
```

### Accessing Box Information

```cpp
// Get box for DOM node
html::Element* element = document->getElementById("content");
Box* box = engine.getBoxForNode(element);

// Get dimensions
Rect content = box->contentRect();
Rect border = box->borderBox();
Rect margin = box->marginBox();

// Get computed position
float absoluteX = content.x;
float absoluteY = content.y;
```

### Debug Output

```cpp
// Print layout tree
engine.printLayoutTree(std::cout);

// Output:
// <html> x=0 y=0 width=1024 height=768 (block)
//   <body> x=8 y=8 width=1008 height=752 (block)
//     <div> x=8 y=8 width=1008 height=100 (block)
//       "Hello World" x=8 y=8 width=88 height=16 (inline)
```

## Margin Collapsing

Vertical margins between blocks collapse:

```cpp
void BlockBox::collapseMargins() {
    // Adjacent vertical margins collapse to the larger value
    for (size_t i = 0; i < m_children.size() - 1; i++) {
        auto& current = m_children[i];
        auto& next = m_children[i + 1];
        
        float collapsed = std::max(current->marginBottom(), 
                                 next->marginTop());
        
        // Adjust positions
        next->setMarginTop(0);
        current->setMarginBottom(collapsed);
    }
}
```

## Limitations

1. **No Floats**: Float layout not implemented
2. **No Absolute Positioning**: Position absolute/fixed not supported
3. **No Flexbox**: Flex layout partially implemented
4. **No Grid**: Grid layout not implemented
5. **No Tables**: Table layout not supported
6. **Simple Line Boxes**: Advanced inline layout missing

## Performance Considerations

### Optimization Strategies

1. **Incremental Layout**: Only re-layout changed subtrees
2. **Layout Caching**: Cache computed dimensions
3. **Dirty Flags**: Mark boxes that need recalculation

### Memory Usage

- Boxes are heap allocated with shared_ptr
- Parent holds children, no circular references
- Layout tree parallel to DOM tree

## Future Enhancements

1. **Flexbox Layout**: Full flex container support
2. **Grid Layout**: CSS Grid implementation
3. **Absolute Positioning**: Out-of-flow layout
4. **Writing Modes**: Vertical text support
5. **Fragmentation**: Page breaks, columns
6. **Performance**: Parallel layout calculation