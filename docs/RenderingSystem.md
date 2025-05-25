# Rendering System Documentation

## Overview

The rendering system transforms the layout tree into visual output. It uses a paint system to create display lists and a custom renderer for drawing operations.

## Architecture

### Core Components

```cpp
namespace browser::rendering {
    class Renderer {
        // Main rendering coordinator
        void render(layout::Box* rootBox, RenderTarget* target);
    };
    
    class PaintSystem {
        // Creates display lists from layout tree
        PaintContext createContext(layout::Box* box);
        void paintBox(layout::Box* box, PaintContext& context);
    };
    
    class CustomRenderContext {
        // Low-level drawing API
        void beginPath();
        void fill();
        void stroke();
    };
}
```

## Rendering Pipeline

### 1. Display List Generation

```
Layout Tree → Paint System → Display List → Renderer → Output
```

### 2. Display Item Types

```cpp
enum class DisplayItemType {
    BACKGROUND,   // Background color/image
    BORDER,       // Box borders
    TEXT,         // Text content
    IMAGE,        // Images
    RECT,         // Generic rectangles
    TRANSFORM,    // Coordinate transforms
    CLIP,         // Clipping regions
    LINE          // Lines
};
```

## Paint System

### Display List Creation

```cpp
class PaintContext {
    DisplayList m_displayList;
    
public:
    void drawBackground(const Rect& rect, const Color& color) {
        m_displayList.appendItem(
            std::make_unique<BackgroundDisplayItem>(rect, color)
        );
    }
    
    void drawText(const std::string& text, float x, float y, 
                  const Color& color, const std::string& font, 
                  float fontSize) {
        m_displayList.appendItem(
            std::make_unique<TextDisplayItem>(
                text, x, y, color, font, fontSize)
        );
    }
};
```

### Box Painting

```cpp
void PaintSystem::paintBox(layout::Box* box, PaintContext& context) {
    // Skip invisible boxes
    if (box->displayType() == DisplayType::NONE) return;
    
    // Get box dimensions
    Rect borderBox = box->borderBox();
    
    // 1. Draw background
    Color bgColor = getBackgroundColor(box);
    if (bgColor.a > 0) {
        context.drawBackground(borderBox, bgColor);
    }
    
    // 2. Draw borders
    if (box->borderTop() > 0 || box->borderRight() > 0 || 
        box->borderBottom() > 0 || box->borderLeft() > 0) {
        Color borderColor = getBorderColor(box);
        context.drawBorder(borderBox, borderColor, 
                         box->borderTop(), box->borderRight(), 
                         box->borderBottom(), box->borderLeft());
    }
    
    // 3. Draw content (text)
    if (auto textBox = dynamic_cast<TextBox*>(box)) {
        paintText(textBox, context);
    }
    
    // 4. Paint children
    for (const auto& child : box->children()) {
        paintBox(child.get(), context);
    }
}
```

## Custom Renderer

### Path API

```cpp
class Path {
    // Path construction
    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void bezierTo(float c1x, float c1y, float c2x, float c2y, 
                  float x, float y);
    void quadTo(float cx, float cy, float x, float y);
    void arcTo(float x1, float y1, float x2, float y2, float radius);
    void closePath();
    
    // Shape helpers
    void rect(float x, float y, float w, float h);
    void roundedRect(float x, float y, float w, float h, float r);
    void ellipse(float cx, float cy, float rx, float ry);
    void circle(float cx, float cy, float r);
};
```

### Paint Types

```cpp
class Paint {
    PaintType type;  // COLOR, LINEAR_GRADIENT, RADIAL_GRADIENT, PATTERN
    
    // Solid color
    void setColor(Color c);
    
    // Gradients
    static Paint linearGradient(float sx, float sy, float ex, float ey,
                              const Color& startColor, 
                              const Color& endColor);
    
    static Paint radialGradient(float cx, float cy, 
                              float innerRadius, float outerRadius,
                              const Color& innerColor, 
                              const Color& outerColor);
};
```

### Transform Operations

```cpp
class CustomRenderContext {
    // State management
    void save();
    void restore();
    
    // Transformations
    void translate(float x, float y);
    void rotate(float angle);
    void scale(float x, float y);
    void transform(float a, float b, float c, float d, float e, float f);
    
    // Transform matrix (affine 2D)
    struct Transform {
        float a, b, c, d, e, f;
        
        void apply(float& x, float& y) const {
            float tx = x;
            float ty = y;
            x = a * tx + c * ty + e;
            y = b * tx + d * ty + f;
        }
    };
};
```

## Render Targets

### Target Types

```cpp
enum class RenderTargetType {
    CONSOLE,   // ASCII art output
    BITMAP     // In-memory bitmap
};

class RenderTarget {
    virtual RenderingContext* context() = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
};
```

### Platform Integration

```cpp
// Custom render target using our renderer
class CustomRenderTarget : public RenderTarget {
    std::shared_ptr<CustomRenderContext> m_context;
    std::shared_ptr<RenderingContext> m_renderingContext;
    
public:
    CustomRenderTarget(int width, int height) {
        m_context = std::make_shared<CustomRenderContext>();
        m_renderingContext = std::make_shared<CustomRenderingContext>(
            m_context.get()
        );
    }
};
```

## Color Management

### Color Representation

```cpp
class Color {
    unsigned char r, g, b;  // RGB components (0-255)
    float a;                // Alpha (0.0-1.0)
    
    // Constructors
    Color() : r(0), g(0), b(0), a(1.0f) {}
    Color(unsigned char red, unsigned char green, unsigned char blue, 
          float alpha = 1.0f);
    
    // CSS color parsing
    static Color fromCssColor(const css::Value& value);
    
    // Utility methods
    unsigned int toRGBA() const;
    static Color fromRGB(unsigned int rgb);
};
```

### CSS Color Parsing

```cpp
Color Color::fromCssColor(const css::Value& value) {
    std::string colorStr = value.stringValue();
    
    // Named colors
    if (colorStr == "black") return Color(0, 0, 0);
    if (colorStr == "white") return Color(255, 255, 255);
    if (colorStr == "red") return Color(255, 0, 0);
    // ... more named colors
    
    // Hex colors (#RGB or #RRGGBB)
    if (colorStr[0] == '#') {
        return parseHexColor(colorStr);
    }
    
    // RGB/RGBA functions
    if (colorStr.substr(0, 4) == "rgb(") {
        return parseRgbColor(colorStr);
    }
    
    // Default
    return Color(0, 0, 0);
}
```

## Text Rendering

### Font Management

```cpp
class Font {
    std::string m_name;
    float m_size;
    
public:
    Font(const std::string& name, float size) 
        : m_name(name), m_size(size) {}
};

// Text metrics
void CustomRenderContext::textMetrics(float* ascender, 
                                     float* descender, 
                                     float* lineHeight) {
    float size = m_currentState.font.size();
    
    if (ascender) *ascender = size * 0.75f;
    if (descender) *descender = size * 0.25f;
    if (lineHeight) *lineHeight = size * 1.25f;
}
```

### Text Layout

```cpp
float CustomRenderContext::text(float x, float y, 
                               const std::string& string) {
    // Apply current transform
    m_currentState.transform.apply(x, y);
    
    // Estimate text width (simplified)
    float charWidth = m_currentState.font.size() * 0.6f;
    float textWidth = string.length() * charWidth;
    
    // Add to render commands
    // (Actual rendering would happen here)
    
    return textWidth;
}
```

## Image Handling

### Image Loading

```cpp
class Image {
    int m_width, m_height;
    std::vector<unsigned char> m_data;  // RGBA data
    
public:
    bool load(const std::string& filename);
    bool save(const std::string& filename) const;
};

// Image management in context
int CustomRenderContext::createImage(const std::string& filename) {
    auto image = std::make_shared<Image>();
    
    if (!image->load(filename)) {
        return 0;  // Failed
    }
    
    int id = m_nextImageId++;
    m_images[id] = image;
    
    return id;
}
```

## Clipping and Masking

### Scissor Rect

```cpp
void CustomRenderContext::scissor(float x, float y, float w, float h) {
    // Transform scissor rectangle
    m_currentState.transform.apply(x, y);
    
    // Set scissor state
    m_currentState.scissorX = x;
    m_currentState.scissorY = y;
    m_currentState.scissorWidth = w;
    m_currentState.scissorHeight = h;
    m_currentState.scissoring = true;
}
```

## ASCII Rendering

### Console Output

```cpp
std::string CustomRenderingContext::toASCII(int width, int height) {
    std::stringstream ss;
    
    // Draw border
    ss << "+" << std::string(width, '-') << "+" << std::endl;
    
    // Draw content area
    for (int y = 0; y < height; y++) {
        ss << "|";
        
        for (int x = 0; x < width; x++) {
            // Determine character based on what's at this position
            char c = getCharacterAt(x, y);
            ss << c;
        }
        
        ss << "|" << std::endl;
    }
    
    // Draw bottom border
    ss << "+" << std::string(width, '-') << "+" << std::endl;
    
    return ss.str();
}
```

## Performance Optimizations

### Display List Optimization

1. **Batching**: Group similar operations
2. **Culling**: Skip off-screen items
3. **Caching**: Reuse display lists for static content

### Rendering Optimizations

```cpp
class DisplayList {
    // Optimize consecutive operations
    void optimize() {
        // Merge adjacent fills with same color
        mergeAdjacentFills();
        
        // Remove redundant state changes
        removeRedundantTransforms();
        
        // Sort by operation type for batching
        sortByType();
    }
};
```

## Usage Examples

### Basic Rendering

```cpp
// Create renderer
Renderer renderer;
renderer.initialize();

// Create render target
auto target = renderer.createTarget(RenderTargetType::BITMAP, 
                                  1024, 768);

// Render layout tree
renderer.render(layoutRoot, target.get());

// Get output (for console)
std::string ascii = renderer.renderToASCII(layoutRoot, 80, 24);
std::cout << ascii;
```

### Custom Drawing

```cpp
// Get custom context
auto customTarget = dynamic_cast<CustomRenderTarget*>(target.get());
auto ctx = customTarget->getCustomContext();

// Begin frame
ctx->beginFrame(1024, 768, 1.0f);

// Draw custom shapes
ctx->beginPath();
ctx->roundedRect(10, 10, 200, 100, 5);
ctx->setFillPaint(Paint(Color(100, 100, 255)));
ctx->fill();

// Draw text
ctx->setFont(Font("Arial", 16));
ctx->setFillPaint(Paint(Color(0, 0, 0)));
ctx->text(20, 50, "Hello World");

// End frame
ctx->endFrame();
```

## Limitations

1. **No GPU Acceleration**: CPU-based rendering only
2. **Basic Compositing**: No advanced blend modes
3. **Limited Effects**: No shadows, filters,