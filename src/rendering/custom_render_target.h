// src/rendering/custom_render_target.h
#ifndef BROWSER_CUSTOM_RENDER_TARGET_H
#define BROWSER_CUSTOM_RENDER_TARGET_H

#include "renderer.h"
#include "custom_renderer.h"

namespace browser {
namespace rendering {

// Custom render target implementation using our custom renderer
class CustomRenderTarget : public RenderTarget {
public:
    CustomRenderTarget(int width, int height);
    virtual ~CustomRenderTarget();
    
    // RenderTarget interface
    virtual RenderingContext* context() override;
    virtual RenderTargetType type() const override;
    virtual int width() const override;
    virtual int height() const override;
    virtual std::string toString() override;
    
    // Get custom context
    CustomRenderContext* getCustomContext() { return m_context.get(); }
    
private:
    int m_width;
    int m_height;
    std::shared_ptr<CustomRenderContext> m_context;
    std::shared_ptr<RenderingContext> m_renderingContext;
};

// Custom rendering context adapter
class CustomRenderingContext : public RenderingContext {
public:
    CustomRenderingContext(CustomRenderContext* ctx);
    virtual ~CustomRenderingContext();
    
    // RenderingContext interface
    virtual void setFillColor(const Color& color) override;
    virtual void fillRect(float x, float y, float width, float height) override;
    
    virtual void setStrokeColor(const Color& color) override;
    virtual void strokeRect(float x, float y, float width, float height, float lineWidth) override;
    
    virtual void setTextColor(const Color& color) override;
    virtual void drawText(const std::string& text, float x, float y, const std::string& fontFamily, float fontSize) override;
    
    virtual void save() override;
    virtual void restore() override;
    virtual void translate(float x, float y) override;
    
    virtual std::string toASCII(int width, int height) override;
    
    // Draw line method (needed for LineDisplayItem)
    virtual void drawLine(float x1, float y1, float x2, float y2, float lineWidth) {
        if (!m_context) return;
        
        m_context->beginPath();
        m_context->moveTo(x1, y1);
        m_context->lineTo(x2, y2);
        m_context->setStrokePaint(Paint(m_strokeColor));
        m_context->setStrokeWidth(lineWidth);
        m_context->stroke();
    }
    
    // Get underlying context
    CustomRenderContext* getCustomContext() const { return m_context; }
    
private:
    CustomRenderContext* m_context;
    Color m_fillColor;
    Color m_strokeColor;
    Color m_textColor;
};

} // namespace rendering
} // namespace browser

#endif // BROWSER_CUSTOM_RENDER_TARGET_H