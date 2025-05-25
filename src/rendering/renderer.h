#ifndef BROWSER_RENDERER_H
#define BROWSER_RENDERER_H

#include "../layout/layout_engine.h"
#include "../css/style_resolver.h"
#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace browser {
namespace rendering {

// Forward declarations
class PaintSystem;
class DisplayList;
class CustomRenderContext;

// Color class for RGBA colors
class Color {
public:
    // Default constructor - black
    Color() : r(0), g(0), b(0), a(1.0f) {}
    
    // RGB constructor (with optional alpha)
    Color(unsigned char red, unsigned char green, unsigned char blue, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
    
    // Copy constructor
    Color(const Color& other) 
        : r(other.r), g(other.g), b(other.b), a(other.a) {}
    
    // Assignment operator
    Color& operator=(const Color& other) {
        r = other.r;
        g = other.g;
        b = other.b;
        a = other.a;
        return *this;
    }
    
    // Color components
    unsigned char r, g, b;
    float a;
    
    // Static helper for creating colors from RGB values
    static Color fromRGB(unsigned char r, unsigned char g, unsigned char b) {
        return Color(r, g, b);
    }
};

// Abstract rendering context interface
class RenderingContext {
public:
    virtual ~RenderingContext() {}
    
    // Drawing operations
    virtual void setFillColor(const Color& color) = 0;
    virtual void fillRect(float x, float y, float width, float height) = 0;
    
    virtual void setStrokeColor(const Color& color) = 0;
    virtual void strokeRect(float x, float y, float width, float height, float lineWidth) = 0;
    
    virtual void setTextColor(const Color& color) = 0;
    virtual void drawText(const std::string& text, float x, float y, const std::string& fontFamily, float fontSize) = 0;
    
    // Transformations and state
    virtual void save() = 0;
    virtual void restore() = 0;
    virtual void translate(float x, float y) = 0;
    
    // Output (for console renderers)
    virtual std::string toASCII(int width, int height) = 0;
};

// Render target types
enum class RenderTargetType {
    CONSOLE,   // ASCII rendering to console
    BITMAP     // Bitmap (in-memory) rendering
};

// Render target interface
class RenderTarget {
public:
    virtual ~RenderTarget() {}
    
    // Get the rendering context
    virtual RenderingContext* context() = 0;
    
    // Get the target type
    virtual RenderTargetType type() const = 0;
    
    // Get the width and height
    virtual int width() const = 0;
    virtual int height() const = 0;
    
    // Convert to string (for console targets)
    virtual std::string toString() = 0;
};

// Renderer class for drawing the layout
class Renderer {
public:
    Renderer();
    ~Renderer();
    
    // Initialize the renderer
    bool initialize();
    
    // Set the paint system
    void setPaintSystem(std::shared_ptr<PaintSystem> paintSystem) { m_paintSystem = paintSystem; }
    
    // Create a render target
    std::shared_ptr<RenderTarget> createTarget(RenderTargetType type, int width, int height);
    
    // Render the layout tree to a target
    void render(layout::Box* rootBox, RenderTarget* target);
    
    // Render using a display list
    void renderDisplayList(const DisplayList& displayList, RenderTarget* target);
    
    // Render using our custom rendering context
    void renderCustomDisplayList(const DisplayList& displayList, CustomRenderContext* ctx);
    
    // For console-based renderers, output ASCII art
    std::string renderToASCII(layout::Box* rootBox, int width, int height);
    
private:
    // Helper functions
    Color getBoxBackgroundColor(layout::Box* box);
    Color getBoxBorderColor(layout::Box* box);
    Color getBoxTextColor(layout::Box* box);
    
    // The paint system
    std::shared_ptr<PaintSystem> m_paintSystem;
};

} // namespace rendering
} // namespace browser

#endif // BROWSER_RENDERER_H