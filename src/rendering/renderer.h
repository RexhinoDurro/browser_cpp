#ifndef BROWSER_RENDERER_H
#define BROWSER_RENDERER_H

#include "../layout/layout_engine.h"
#include "../css/style_resolver.h"
#include "render_target.h"
#include "custom_renderer.h"  // Include this to get the Color class
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

// Extended rendering context interface with drawing operations
class ExtendedRenderingContext : public RenderingContext {
public:
    virtual ~ExtendedRenderingContext() {}
    
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