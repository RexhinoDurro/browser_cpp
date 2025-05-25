#ifndef BROWSER_UI_CUSTOM_CANVAS_H
#define BROWSER_UI_CUSTOM_CANVAS_H

#include "window.h"
#include "../rendering/custom_renderer.h"

namespace browser {
namespace ui {

// Custom canvas implementation using our custom renderer
class CustomCanvas : public Canvas {
public:
    CustomCanvas(int width, int height);
    virtual ~CustomCanvas();
    
    // Initialize renderer
    bool initialize();
    
    // Canvas methods
    virtual void clear(unsigned int color) override;
    virtual void drawLine(int x1, int y1, int x2, int y2, unsigned int color, int thickness = 1) override;
    virtual void drawRect(int x, int y, int width, int height, unsigned int color, bool filled = false, int thickness = 1) override;
    virtual void drawEllipse(int x, int y, int width, int height, unsigned int color, bool filled = false, int thickness = 1) override;
    virtual void drawText(const std::string& text, int x, int y, unsigned int color, const std::string& fontName = "Arial", int fontSize = 12) override;
    
    // Get the rendering context
    rendering::CustomRenderContext* getContext() { return m_context.get(); }
    
    // Begin/end rendering frame
    void beginFrame();
    void endFrame();
    
    // Helper methods for color handling
    rendering::Color createColor(unsigned int color);
    rendering::Paint createPaint(unsigned int color);
    
private:
    std::shared_ptr<rendering::CustomRenderContext> m_context;
};

} // namespace ui
} // namespace browser

#endif // BROWSER_UI_CUSTOM_CANVAS_H