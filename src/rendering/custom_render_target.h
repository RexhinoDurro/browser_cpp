#ifndef BROWSER_RENDERING_CUSTOM_RENDER_TARGET_H
#define BROWSER_RENDERING_CUSTOM_RENDER_TARGET_H

#include "render_target.h"
#include "custom_renderer.h"
#include <memory>
#include <string>

namespace browser {
namespace rendering {

// Forward declarations
class CustomRenderContext;

// Custom render target implementation
class CustomRenderTarget : public RenderTarget {
public:
    CustomRenderTarget(int width, int height);
    virtual ~CustomRenderTarget();
    
    // RenderTarget interface
    virtual RenderingContext* context() override;
    virtual RenderTargetType type() const override;
    virtual int width() const override;
    virtual int height() const override;
    
    // Get the custom context
    CustomRenderContext* getCustomContext() const { return m_context.get(); }
    
    // Convert to string representation (ASCII art)
    std::string toString();
    
private:
    int m_width;
    int m_height;
    std::shared_ptr<CustomRenderContext> m_context;
    std::shared_ptr<RenderingContext> m_renderingContext;
};

// Adapter to bridge CustomRenderContext with RenderingContext
class CustomRenderingContext : public RenderingContext {
public:
    CustomRenderingContext(CustomRenderContext* ctx);
    virtual ~CustomRenderingContext();
    
    // Basic drawing operations
    void setFillColor(const Color& color);
    void fillRect(float x, float y, float width, float height);
    
    void setStrokeColor(const Color& color);
    void strokeRect(float x, float y, float width, float height, float lineWidth = 1.0f);
    
    void setTextColor(const Color& color);
    void drawText(const std::string& text, float x, float y, 
                  const std::string& fontFamily = "Arial", float fontSize = 12.0f);
    
    // Transform operations
    void save();
    void restore();
    void translate(float x, float y);
    
    // Convert to ASCII representation
    std::string toASCII(int width, int height);
    
private:
    CustomRenderContext* m_context;
    Color m_fillColor;
    Color m_strokeColor;
    Color m_textColor;
};

} // namespace rendering
} // namespace browser

#endif // BROWSER_RENDERING_CUSTOM_RENDER_TARGET_H