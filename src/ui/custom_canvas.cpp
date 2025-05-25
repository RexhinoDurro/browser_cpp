#include "custom_canvas.h"
#include "../rendering/renderer_integration.h"
#include <iostream>

namespace browser {
namespace ui {

CustomCanvas::CustomCanvas(int width, int height)
    : Canvas(width, height)
{
    // Create a custom render context
    m_context = std::make_shared<rendering::CustomRenderContext>();
}

CustomCanvas::~CustomCanvas() {
    // Cleanup resources
}

bool CustomCanvas::initialize() {
    // Initialize the render context
    if (m_context) {
        m_context->beginFrame(m_width, m_height, 1.0f);
        return true;
    }
    return false;
}

void CustomCanvas::clear(unsigned int color) {
    if (!m_context) return;
    
    // Create a paint with the specified color
    rendering::Paint paint;
    paint.setColor(createColor(color));
    
    // Clear the canvas
    m_context->beginPath();
    m_context->rect(0, 0, m_width, m_height);
    m_context->setFillPaint(paint);
    m_context->fill();
}

void CustomCanvas::drawLine(int x1, int y1, int x2, int y2, unsigned int color, int thickness) {
    if (!m_context) return;
    
    // Create a paint with the specified color
    rendering::Paint paint;
    paint.setColor(createColor(color));
    
    // Draw the line
    m_context->beginPath();
    m_context->moveTo(x1, y1);
    m_context->lineTo(x2, y2);
    m_context->setStrokePaint(paint);
    m_context->setStrokeWidth(thickness);
    m_context->stroke();
}

void CustomCanvas::drawRect(int x, int y, int width, int height, unsigned int color, bool filled, int thickness) {
    if (!m_context) return;
    
    // Create a paint with the specified color
    rendering::Paint paint;
    paint.setColor(createColor(color));
    
    // Draw the rectangle
    m_context->beginPath();
    m_context->rect(x, y, width, height);
    
    if (filled) {
        m_context->setFillPaint(paint);
        m_context->fill();
    } else {
        m_context->setStrokePaint(paint);
        m_context->setStrokeWidth(thickness);
        m_context->stroke();
    }
}

void CustomCanvas::drawEllipse(int x, int y, int width, int height, unsigned int color, bool filled, int thickness) {
    if (!m_context) return;
    
    // Create a paint with the specified color
    rendering::Paint paint;
    paint.setColor(createColor(color));
    
    // Draw the ellipse
    m_context->beginPath();
    m_context->ellipse(x + width/2.0f, y + height/2.0f, width/2.0f, height/2.0f);
    
    if (filled) {
        m_context->setFillPaint(paint);
        m_context->fill();
    } else {
        m_context->setStrokePaint(paint);
        m_context->setStrokeWidth(thickness);
        m_context->stroke();
    }
}

void CustomCanvas::drawText(const std::string& text, int x, int y, unsigned int color, const std::string& fontName, int fontSize) {
    if (!m_context) return;
    
    // Create a paint with the specified color
    rendering::Paint paint;
    paint.setColor(createColor(color));
    
    // Set font
    rendering::Font font(fontName, fontSize);
    m_context->setFont(font);
    
    // Set fill paint for text
    m_context->setFillPaint(paint);
    
    // Draw the text
    m_context->text(x, y, text);
}

void CustomCanvas::beginFrame() {
    if (m_context) {
        m_context->beginFrame(m_width, m_height, 1.0f);
    }
}

void CustomCanvas::endFrame() {
    if (m_context) {
        m_context->endFrame();
    }
}

rendering::Color CustomCanvas::createColor(unsigned int color) {
    // Extract color components
    unsigned char r = (color >> 16) & 0xFF;
    unsigned char g = (color >> 8) & 0xFF;
    unsigned char b = color & 0xFF;
    unsigned char a = (color >> 24) & 0xFF;
    
    // Create color object
    rendering::Color renderColor;
    renderColor.r = r;
    renderColor.g = g;
    renderColor.b = b;
    renderColor.a = a / 255.0f;
    
    return renderColor;
}

rendering::Paint CustomCanvas::createPaint(unsigned int color) {
    rendering::Paint paint;
    paint.setColor(createColor(color));
    return paint;
}

} // namespace ui
} // namespace browser