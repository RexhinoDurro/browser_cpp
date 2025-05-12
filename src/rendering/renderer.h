#ifndef BROWSER_RENDERER_H
#define BROWSER_RENDERER_H

#include "../layout/layout_engine.h"
#include "../css/style_resolver.h"
#include <string>
#include <memory>
#include <functional>

namespace browser {
namespace rendering {

// Color class for RGBA colors
class Color {
public:
    Color() : r(0), g(0), b(0), a(1.0f) {}
    Color(unsigned char r, unsigned char g, unsigned char b, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {}
    
    // Parse a CSS color value
    static Color fromCssColor(const css::Value& value);
    
    unsigned char r, g, b;
    float a;
};

// Forward declaration
class RenderingContext;

// Renderer class for drawing the layout
class Renderer {
public:
    Renderer();
    ~Renderer();
    
    // Initialize the renderer
    bool initialize();
    
    // Render the layout tree to a context
    void render(layout::Box* rootBox, RenderingContext* context);
    
    // Render a specific box
    void renderBox(layout::Box* box, RenderingContext* context);
    
    // For console-based renderers, output ASCII art
    std::string renderToASCII(layout::Box* rootBox, int width, int height);
    
private:
    // Helper functions
    Color getBoxBackgroundColor(layout::Box* box);
    Color getBoxBorderColor(layout::Box* box);
    Color getBoxTextColor(layout::Box* box);
    
    // Drawing functions
    void drawBoxBackground(layout::Box* box, RenderingContext* context);
    void drawBoxBorder(layout::Box* box, RenderingContext* context);
    void drawBoxText(layout::Box* box, RenderingContext* context);
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

// Simple console-based rendering context (for testing)
class ConsoleRenderingContext : public RenderingContext {
public:
    ConsoleRenderingContext(int width, int height);
    virtual ~ConsoleRenderingContext();
    
    // Implement drawing operations
    virtual void setFillColor(const Color& color) override;
    virtual void fillRect(float x, float y, float width, float height) override;
    
    virtual void setStrokeColor(const Color& color) override;
    virtual void strokeRect(float x, float y, float width, float height, float lineWidth) override;
    
    virtual void setTextColor(const Color& color) override;
    virtual void drawText(const std::string& text, float x, float y, const std::string& fontFamily, float fontSize) override;
    
    // Transformations and state
    virtual void save() override;
    virtual void restore() override;
    virtual void translate(float x, float y) override;
    
    // Output
    virtual std::string toASCII(int width, int height) override;
    
private:
    // Simple character-based buffer for console rendering
    std::vector<std::vector<char>> m_buffer;
    int m_width;
    int m_height;
    
    // Current state
    Color m_fillColor;
    Color m_strokeColor;
    Color m_textColor;
    float m_translateX;
    float m_translateY;
    
    // State stack for save/restore
    struct State {
        Color fillColor;
        Color strokeColor;
        Color textColor;
        float translateX;
        float translateY;
    };
    std::vector<State> m_stateStack;
};

} // namespace rendering
} // namespace browser

#endif // BROWSER_RENDERER_H