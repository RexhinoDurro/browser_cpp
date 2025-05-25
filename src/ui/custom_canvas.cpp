// src/rendering/custom_render_target.cpp
#include "custom_render_target.h"
#include <sstream>

namespace browser {
namespace rendering {

//-----------------------------------------------------------------------------
// CustomRenderTarget Implementation
//-----------------------------------------------------------------------------

CustomRenderTarget::CustomRenderTarget(int width, int height)
    : m_width(width)
    , m_height(height)
{
    // Initialize custom renderer
    m_context = std::make_shared<CustomRenderContext>();
    
    // Create adapter
    m_renderingContext = std::make_shared<CustomRenderingContext>(m_context.get());
}

CustomRenderTarget::~CustomRenderTarget() {
}

RenderingContext* CustomRenderTarget::context() {
    return m_renderingContext.get();
}

RenderTargetType CustomRenderTarget::type() const {
    return RenderTargetType::BITMAP;
}

int CustomRenderTarget::width() const {
    return m_width;
}

int CustomRenderTarget::height() const {
    return m_height;
}

std::string CustomRenderTarget::toString() {
    // Render to ASCII representation
    if (m_renderingContext) {
        return m_renderingContext->toASCII(m_width, m_height);
    }
    return "";
}

//-----------------------------------------------------------------------------
// CustomRenderingContext Implementation
//-----------------------------------------------------------------------------

CustomRenderingContext::CustomRenderingContext(CustomRenderContext* ctx)
    : m_context(ctx)
    , m_fillColor(255, 255, 255)
    , m_strokeColor(0, 0, 0)
    , m_textColor(0, 0, 0)
{
}

CustomRenderingContext::~CustomRenderingContext() {
}

void CustomRenderingContext::setFillColor(const Color& color) {
    m_fillColor = color;
}

void CustomRenderingContext::fillRect(float x, float y, float width, float height) {
    if (!m_context) return;
    
    m_context->beginPath();
    m_context->rect(x, y, width, height);
    
    // Use the setColor method instead of constructor
    Paint paint;
    paint.setColor(m_fillColor);
    m_context->setFillPaint(paint);
    m_context->fill();
}

void CustomRenderingContext::setStrokeColor(const Color& color) {
    m_strokeColor = color;
}

void CustomRenderingContext::strokeRect(float x, float y, float width, float height, float lineWidth) {
    if (!m_context) return;
    
    m_context->beginPath();
    m_context->rect(x, y, width, height);
    
    // Use the setColor method instead of constructor
    Paint paint;
    paint.setColor(m_strokeColor);
    m_context->setStrokePaint(paint);
    m_context->setStrokeWidth(lineWidth);
    m_context->stroke();
}

void CustomRenderingContext::setTextColor(const Color& color) {
    m_textColor = color;
}

void CustomRenderingContext::drawText(const std::string& text, float x, float y, const std::string& fontFamily, float fontSize) {
    if (!m_context) return;
    
    m_context->setFont(Font(fontFamily, fontSize));
    
    // Use the setColor method instead of constructor
    Paint paint;
    paint.setColor(m_textColor);
    m_context->setFillPaint(paint);
    m_context->text(x, y, text);
}

void CustomRenderingContext::save() {
    if (m_context) {
        m_context->save();
    }
}

void CustomRenderingContext::restore() {
    if (m_context) {
        m_context->restore();
    }
}

void CustomRenderingContext::translate(float x, float y) {
    if (m_context) {
        m_context->translate(x, y);
    }
}

std::string CustomRenderingContext::toASCII(int width, int height) {
    // Simple ASCII art conversion
    std::stringstream ss;
    
    // Draw border
    ss << "+" << std::string(width, '-') << "+" << std::endl;
    
    // Draw content area
    for (int y = 0; y < height; y++) {
        ss << "|";
        
        // For each character in the line
        for (int x = 0; x < width; x++) {
            // For now, just use a simple character
            // In a real implementation, we would look at paths in the custom renderer
            // and determine which character to use based on the fill color at each position
            ss << " ";
        }
        
        ss << "|" << std::endl;
    }
    
    // Draw bottom border
    ss << "+" << std::string(width, '-') << "+" << std::endl;
    
    return ss.str();
}

} // namespace rendering
} 