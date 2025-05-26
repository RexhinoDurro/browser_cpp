// src/ui/custom_canvas.cpp
#include "custom_canvas.h"
#include <cmath>

namespace browser {
namespace ui {

//-----------------------------------------------------------------------------
// CustomCanvas Implementation
//-----------------------------------------------------------------------------

CustomCanvas::CustomCanvas(int width, int height)
    : Canvas(width, height)
{
    // Initialize custom renderer
    m_context = std::make_shared<rendering::CustomRenderContext>();
}

CustomCanvas::~CustomCanvas() {
}

bool CustomCanvas::initialize() {
    // Initialize the rendering context if needed
    return m_context != nullptr;
}

void CustomCanvas::clear(unsigned int color) {
    if (!m_context) return;
    
    // Clear the entire canvas with the specified color
    beginFrame();
    
    m_context->beginPath();
    m_context->rect(0, 0, static_cast<float>(m_width), static_cast<float>(m_height));
    
    rendering::Paint paint = createPaint(color);
    m_context->setFillPaint(paint);
    m_context->fill();
    
    endFrame();
}

void CustomCanvas::drawLine(int x1, int y1, int x2, int y2, unsigned int color, int thickness) {
    if (!m_context) return;
    
    m_context->beginPath();
    m_context->moveTo(static_cast<float>(x1), static_cast<float>(y1));
    m_context->lineTo(static_cast<float>(x2), static_cast<float>(y2));
    
    rendering::Paint paint = createPaint(color);
    m_context->setStrokePaint(paint);
    m_context->setStrokeWidth(static_cast<float>(thickness));
    m_context->stroke();
}

void CustomCanvas::drawRect(int x, int y, int width, int height, unsigned int color, bool filled, int thickness) {
    if (!m_context) return;
    
    m_context->beginPath();
    m_context->rect(static_cast<float>(x), static_cast<float>(y), 
                    static_cast<float>(width), static_cast<float>(height));
    
    rendering::Paint paint = createPaint(color);
    
    if (filled) {
        m_context->setFillPaint(paint);
        m_context->fill();
    } else {
        m_context->setStrokePaint(paint);
        m_context->setStrokeWidth(static_cast<float>(thickness));
        m_context->stroke();
    }
}

void CustomCanvas::drawEllipse(int x, int y, int width, int height, unsigned int color, bool filled, int thickness) {
    if (!m_context) return;
    
    // Calculate center and radii
    float cx = x + width / 2.0f;
    float cy = y + height / 2.0f;
    float rx = width / 2.0f;
    float ry = height / 2.0f;
    
    // Draw ellipse using arc with scaling
    m_context->beginPath();
    m_context->save();
    m_context->translate(cx, cy);
    
    // Scale to create ellipse from circle
    if (rx > 0) {
        m_context->scale(1.0f, ry / rx);
        m_context->arc(0, 0, rx, 0, 2 * 3.14159265f, false);
    }
    
    m_context->restore();
    
    rendering::Paint paint = createPaint(color);
    
    if (filled) {
        m_context->setFillPaint(paint);
        m_context->fill();
    } else {
        m_context->setStrokePaint(paint);
        m_context->setStrokeWidth(static_cast<float>(thickness));
        m_context->stroke();
    }
}

void CustomCanvas::drawText(const std::string& text, int x, int y, unsigned int color, 
                           const std::string& fontName, int fontSize) {
    if (!m_context) return;
    
    // Set font
    m_context->setFont(rendering::Font(fontName, static_cast<float>(fontSize)));
    
    // Set text color
    rendering::Paint paint = createPaint(color);
    m_context->setFillPaint(paint);
    
    // Draw text
    m_context->text(static_cast<float>(x), static_cast<float>(y), text);
}

void CustomCanvas::beginFrame() {
    if (m_context) {
        m_context->save();
    }
}

void CustomCanvas::endFrame() {
    if (m_context) {
        m_context->restore();
    }
}

rendering::Color CustomCanvas::createColor(unsigned int color) {
    // Extract color components
    unsigned char r = (color >> 16) & 0xFF;
    unsigned char g = (color >> 8) & 0xFF;
    unsigned char b = color & 0xFF;
    unsigned char a = (color >> 24) & 0xFF;
    
    // Convert to normalized float values
    float fr = r / 255.0f;
    float fg = g / 255.0f;
    float fb = b / 255.0f;
    float fa = a / 255.0f;
    
    // If alpha is 0 (default for RGB colors), set it to fully opaque
    if (a == 0) {
        fa = 1.0f;
    }
    
    return rendering::Color(r, g, b, fa);
}

rendering::Paint CustomCanvas::createPaint(unsigned int color) {
    rendering::Paint paint;
    paint.setColor(createColor(color));
    return paint;
}

} // namespace ui
} // namespace browser