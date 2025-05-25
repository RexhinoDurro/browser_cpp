// src/rendering/renderer_integration.h
#ifndef BROWSER_RENDERER_INTEGRATION_H
#define BROWSER_RENDERER_INTEGRATION_H

#include "custom_renderer.h"
#include "custom_render_target.h"
#include "renderer.h"
#include "paint_system.h"

namespace browser {
namespace rendering {

// Factory function to create a renderer that uses our custom rendering system
inline std::shared_ptr<Renderer> createRenderer() {
    auto renderer = std::make_shared<Renderer>();
    renderer->initialize();
    return renderer;
}

// Factory function to create a paint system that uses our custom rendering system
inline std::shared_ptr<PaintSystem> createPaintSystem() {
    auto paintSystem = std::make_shared<PaintSystem>();
    paintSystem->initialize();
    return paintSystem;
}

// Factory function to create a render target using our custom rendering system
inline std::shared_ptr<RenderTarget> createRenderTarget(int width, int height) {
    return std::make_shared<CustomRenderTarget>(width, height);
}

// Helper function to initialize the renderer in browser window
inline void initializeRenderer(ui::BrowserWindow& window) {
    auto renderer = createRenderer();
    auto paintSystem = createPaintSystem();
    renderer->setPaintSystem(paintSystem);
    
    window.setRenderer(renderer);
}

} // namespace rendering
} // namespace browser

#endif // BROWSER_RENDERER_INTEGRATION_H