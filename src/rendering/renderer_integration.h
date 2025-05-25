// src/rendering/renderer_integration.h
#ifndef BROWSER_RENDERER_INTEGRATION_H
#define BROWSER_RENDERER_INTEGRATION_H

#include "renderer.h"
#include "paint_system.h"
#include "custom_render_target.h"

// Conditional include to prevent double inclusion of custom_renderer.h
#ifndef BROWSER_RENDERING_CUSTOM_RENDERER_INCLUDED
#define BROWSER_RENDERING_CUSTOM_RENDERER_INCLUDED
#include "custom_renderer.h"
#endif

#include "../layout/layout_engine.h"

namespace browser {
namespace rendering {

// Define a rendering Rectangle structure if not already defined
struct RenderRect {
    float x, y, width, height;
    
    RenderRect() : x(0), y(0), width(0), height(0) {}
    RenderRect(float x, float y, float width, float height) 
        : x(x), y(y), width(width), height(height) {}
    
    float right() const { return x + width; }
    float bottom() const { return y + height; }
};

// Color conversion helper class
class ColorConverter {
public:
    // Convert from rendering::Color to custom renderer Color
    static Color toCustomColor(const Color& renderColor) {
        return Color(renderColor.r, renderColor.g, renderColor.b, renderColor.a);
    }
    
    // Convert from custom renderer Color to rendering::Color
    static Color fromCustomColor(const Color& customColor) {
        return Color(customColor.r, customColor.g, customColor.b, customColor.a);
    }
};

// Helper class for integrating different rendering backends
class RendererIntegration {
public:
    // Initialize the renderer with a specific backend
    static bool initializeRenderer(Renderer* renderer);
    
    // Create appropriate render target based on platform
    static std::shared_ptr<RenderTarget> createTargetForPlatform(RenderTargetType type, int width, int height);
    
    // Helper methods for color conversion
    static Color cssValueToColor(const css::Value& value);
    static unsigned int colorToRGBA(const Color& color);
    
    // Render a layout tree to a custom renderer context directly
    static void renderToCustomContext(layout::Box* rootBox, CustomRenderContext* ctx);
    
    // Convert a layout rect to a rendering rect
    static RenderRect layoutRectToRendererRect(const layout::Rect& rect) {
        return RenderRect(rect.x, rect.y, rect.width, rect.height);
    }
};

} // namespace rendering
} // namespace browser

#endif // BROWSER_RENDERER_INTEGRATION_H