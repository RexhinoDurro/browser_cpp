// src/ui/custom_canvas.cpp
#include "custom_canvas.h"

namespace browser {
namespace ui {

CustomCanvas::CustomCanvas(int width, int height)
    : Canvas(width, height)
{
    initialize();
}

CustomCanvas::~CustomCanvas() {
}

bool CustomCanvas::initialize() {
    m_context = std::make_shared<rendering::CustomRenderContext>();
    return m_context != nullptr;
}

void CustomCanvas::clear(unsigned int color) {
    if (!m_context) return;
    
    m_context->beginPath();
    m_context->rect(0, 0, m_width, m_height);
    m_context->setFillPaint(createPaint(color));
    m_context->fill();
}

void CustomCanvas::drawLine(int x1, int y1, int x2, int y2, unsigned int color, int thickness) {
    if (!m_context) return;
    
    m_context->beginPath();
    m_context->moveTo(x1, y1);
    m_context->lineTo(x2, y2);
    m_context->setStrokePaint(createPaint(color));
    m_context->setStrokeWidth(thickness);
    m_context->stroke();
}

void CustomCanvas::drawRect(int x, int y, int width, int height, unsigned int color, bool filled, int thickness) {
    if (!m_context) return;
    
    m_context->beginPath();
    m_context->rect(x, y, width, height);
    
    if (filled) {
        m_context->setFillPaint(createPaint(color));
        m_context->fill();
    } else {
        m_context->setStrokePaint(createPaint(color));
        m_context->setStrokeWidth(thickness);
        m_context->stroke();
    }
}

void CustomCanvas::drawEllipse(int x, int y, int width, int height, unsigned int color, bool filled, int thickness) {
    if (!m_context) return;
    
    float cx = x + width * 0.5f;
    float cy = y + height * 0.5f;
    float rx = width * 0.5f;
    float ry = height * 0.5f;
    
    m_context->beginPath();
    m_context->ellipse(cx, cy, rx, ry);
    
    if (filled) {
        m_context->setFillPaint(createPaint(color));
        m_context->fill();
    } else {
        m_context->setStrokePaint(createPaint(color));
        m_context->setStrokeWidth(thickness);
        m_context->stroke();
    }
}

void CustomCanvas::drawText(const std::string& text, int x, int y, unsigned int color, const std::string& fontName, int fontSize) {
    if (!m_context) return;
    
    m_context->setFont(rendering::Font(fontName, fontSize));
    m_context->setFillPaint(createPaint(color));
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

rendering::Paint CustomCanvas::createPaint(unsigned int color) {
    // Extract RGBA components
    unsigned char r = (color >> 16) & 0xFF;
    unsigned char g = (color >> 8) & 0xFF;
    unsigned char b = color & 0xFF;
    unsigned char a = (color >> 24) & 0xFF;
    
    // If alpha is 0 (not specified), set it to 255 (fully opaque)
    if (a == 0) {
        a = 255;
    }
    
    // Create color and paint
    rendering::Color c(r, g, b, a / 255.0f);
    rendering::Paint paint;
    paint.setColor(c);
    return paint;
}

} // namespace ui
} // namespace browser