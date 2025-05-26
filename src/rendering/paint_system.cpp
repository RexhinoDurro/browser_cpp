#include "paint_system.h"
#include "custom_render_target.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace browser {
namespace rendering {

//-----------------------------------------------------------------------------
// DisplayItem Implementation
//-----------------------------------------------------------------------------

DisplayItem::DisplayItem(DisplayItemType type)
    : m_type(type)
{
}

DisplayItem::~DisplayItem() {
}

//-----------------------------------------------------------------------------
// BackgroundDisplayItem Implementation
//-----------------------------------------------------------------------------

BackgroundDisplayItem::BackgroundDisplayItem(const layout::Rect& rect, const Color& color)
    : DisplayItem(DisplayItemType::BACKGROUND)
    , m_rect(rect)
    , m_color(color)
{
}

BackgroundDisplayItem::~BackgroundDisplayItem() {
}

void BackgroundDisplayItem::paint(RenderingContext* context) {
    if (!context) {
        return;
    }
    
    // Try to cast to our custom context type
    auto customContext = dynamic_cast<CustomRenderingContext*>(context);
    if (customContext) {
        // Set fill color and draw rectangle
        customContext->setFillColor(m_color);
        customContext->fillRect(m_rect.x, m_rect.y, m_rect.width, m_rect.height);
    }
}

//-----------------------------------------------------------------------------
// BorderDisplayItem Implementation
//-----------------------------------------------------------------------------

BorderDisplayItem::BorderDisplayItem(const layout::Rect& rect, const Color& color, 
                                 float top, float right, float bottom, float left)
    : DisplayItem(DisplayItemType::BORDER)
    , m_rect(rect)
    , m_color(color)
    , m_topWidth(top)
    , m_rightWidth(right)
    , m_bottomWidth(bottom)
    , m_leftWidth(left)
{
}

BorderDisplayItem::~BorderDisplayItem() {
}

void BorderDisplayItem::paint(RenderingContext* context) {
    if (!context) {
        return;
    }
    
    // Try to cast to our custom context type
    auto customContext = dynamic_cast<CustomRenderingContext*>(context);
    if (customContext) {
        // Set stroke color
        customContext->setStrokeColor(m_color);
        
        // Draw border rectangle
        float maxWidth = std::max({m_topWidth, m_rightWidth, m_bottomWidth, m_leftWidth});
        customContext->strokeRect(m_rect.x, m_rect.y, m_rect.width, m_rect.height, maxWidth);
    }
}

//-----------------------------------------------------------------------------
// TextDisplayItem Implementation
//-----------------------------------------------------------------------------

TextDisplayItem::TextDisplayItem(const std::string& text, float x, float y, 
                             const Color& color, const std::string& fontFamily, float fontSize)
    : DisplayItem(DisplayItemType::TEXT)
    , m_text(text)
    , m_x(x)
    , m_y(y)
    , m_color(color)
    , m_fontFamily(fontFamily)
    , m_fontSize(fontSize)
{
}

TextDisplayItem::~TextDisplayItem() {
}

void TextDisplayItem::paint(RenderingContext* context) {
    if (!context) {
        return;
    }
    
    // Try to cast to our custom context type
    auto customContext = dynamic_cast<CustomRenderingContext*>(context);
    if (customContext) {
        // Set text color
        customContext->setTextColor(m_color);
        
        // Draw text
        customContext->drawText(m_text, m_x, m_y, m_fontFamily, m_fontSize);
    }
}

//-----------------------------------------------------------------------------
// ImageDisplayItem Implementation
//-----------------------------------------------------------------------------

ImageDisplayItem::ImageDisplayItem(const std::string& url, const layout::Rect& rect)
    : DisplayItem(DisplayItemType::IMAGE)
    , m_url(url)
    , m_rect(rect)
{
}

ImageDisplayItem::~ImageDisplayItem() {
}

void ImageDisplayItem::paint(RenderingContext* context) {
    if (!context) {
        return;
    }
    
    // Try to cast to our custom context type
    auto customContext = dynamic_cast<CustomRenderingContext*>(context);
    if (customContext) {
        // In a real implementation, we would load the image and draw it here
        // For now, just draw a placeholder rectangle
        customContext->setFillColor(Color(200, 200, 200)); // Light gray
        customContext->fillRect(m_rect.x, m_rect.y, m_rect.width, m_rect.height);
        
        customContext->setStrokeColor(Color(100, 100, 100)); // Dark gray
        customContext->strokeRect(m_rect.x, m_rect.y, m_rect.width, m_rect.height, 1.0f);
        
        // Draw image URL as text
        customContext->setTextColor(Color(0, 0, 0));
        customContext->drawText("Image: " + m_url, m_rect.x + 5, m_rect.y + 15, "Arial", 10.0f);
    }
}

//-----------------------------------------------------------------------------
// RectDisplayItem Implementation
//-----------------------------------------------------------------------------

RectDisplayItem::RectDisplayItem(const layout::Rect& rect, const Color& color, bool fill)
    : DisplayItem(DisplayItemType::RECT)
    , m_rect(rect)
    , m_color(color)
    , m_fill(fill)
{
}

RectDisplayItem::~RectDisplayItem() {
}

void RectDisplayItem::paint(RenderingContext* context) {
    if (!context) {
        return;
    }
    
    // Try to cast to our custom context type
    auto customContext = dynamic_cast<CustomRenderingContext*>(context);
    if (customContext) {
        if (m_fill) {
            customContext->setFillColor(m_color);
            customContext->fillRect(m_rect.x, m_rect.y, m_rect.width, m_rect.height);
        } else {
            customContext->setStrokeColor(m_color);
            customContext->strokeRect(m_rect.x, m_rect.y, m_rect.width, m_rect.height, 1.0f);
        }
    }
}

//-----------------------------------------------------------------------------
// TransformDisplayItem Implementation
//-----------------------------------------------------------------------------

TransformDisplayItem::TransformDisplayItem(float dx, float dy)
    : DisplayItem(DisplayItemType::TRANSFORM)
    , m_dx(dx)
    , m_dy(dy)
{
}

TransformDisplayItem::~TransformDisplayItem() {
}

void TransformDisplayItem::paint(RenderingContext* context) {
    if (!context) {
        return;
    }
    
    // Try to cast to our custom context type
    auto customContext = dynamic_cast<CustomRenderingContext*>(context);
    if (customContext) {
        // Apply translation
        customContext->translate(m_dx, m_dy);
    }
}

//-----------------------------------------------------------------------------
// ClipDisplayItem Implementation
//-----------------------------------------------------------------------------

ClipDisplayItem::ClipDisplayItem(const layout::Rect& rect)
    : DisplayItem(DisplayItemType::CLIP)
    , m_rect(rect)
{
}

ClipDisplayItem::~ClipDisplayItem() {
}

void ClipDisplayItem::paint(RenderingContext* context) {
    if (!context) {
        return;
    }
    
    // In a real implementation, we would set a clipping region here
    // For simplicity, this is not implemented in the console rendering context
}

//-----------------------------------------------------------------------------
// LineDisplayItem Implementation
//-----------------------------------------------------------------------------

LineDisplayItem::LineDisplayItem(float x1, float y1, float x2, float y2, const Color& color, float thickness)
    : DisplayItem(DisplayItemType::LINE)
    , m_x1(x1)
    , m_y1(y1)
    , m_x2(x2)
    , m_y2(y2)
    , m_color(color)
    , m_thickness(thickness)
{
}

LineDisplayItem::~LineDisplayItem() {
}

void LineDisplayItem::paint(RenderingContext* context) {
    if (!context) {
        return;
    }
    
    // Try to cast to our custom context type
    auto customContext = dynamic_cast<CustomRenderingContext*>(context);
    if (customContext) {
        // For now, we'll implement this as a thin rectangle
        // In a real implementation, we'd have a proper line drawing method
        customContext->setFillColor(m_color);
        
        // Calculate line as a rectangle
        float dx = m_x2 - m_x1;
        float dy = m_y2 - m_y1;
        float length = std::sqrt(dx * dx + dy * dy);
        
        if (length > 0) {
            // Normalize direction
            dx /= length;
            dy /= length;
            
            // Perpendicular vector for thickness
            float px = -dy * m_thickness * 0.5f;
            float py = dx * m_thickness * 0.5f;
            
            // Draw as a filled rectangle
            // This is a simplified approach
            customContext->fillRect(
                std::min(m_x1, m_x2), 
                std::min(m_y1, m_y2),
                std::abs(m_x2 - m_x1) + m_thickness,
                std::abs(m_y2 - m_y1) + m_thickness
            );
        }
    }
}

//-----------------------------------------------------------------------------
// DisplayList Implementation
//-----------------------------------------------------------------------------

DisplayList::DisplayList() {
}

DisplayList::~DisplayList() {
    clear();
}

void DisplayList::appendItem(std::unique_ptr<DisplayItem> item) {
    if (item) {
        m_items.push_back(std::move(item));
    }
}

void DisplayList::paint(RenderingContext* context) const {
    if (!context) {
        return;
    }
    
    // Try to cast to our custom context type
    auto customContext = dynamic_cast<CustomRenderingContext*>(context);
    if (customContext) {
        // Save the context state
        customContext->save();
        
        // Paint all items in order
        for (const auto& item : m_items) {
            if (item) {
                item->paint(context);
            }
        }
        
        // Restore the context state
        customContext->restore();
    }
}

void DisplayList::clear() {
    m_items.clear();
}

//-----------------------------------------------------------------------------
// PaintContext Implementation
//-----------------------------------------------------------------------------

PaintContext::PaintContext() {
}

PaintContext::~PaintContext() {
}

void PaintContext::drawBackground(const layout::Rect& rect, const Color& color) {
    m_displayList.appendItem(std::make_unique<BackgroundDisplayItem>(rect, color));
}

void PaintContext::drawBorder(const layout::Rect& rect, const Color& color, 
                           float top, float right, float bottom, float left) {
    m_displayList.appendItem(std::make_unique<BorderDisplayItem>(rect, color, top, right, bottom, left));
}

void PaintContext::drawText(const std::string& text, float x, float y, 
                         const Color& color, const std::string& fontFamily, float fontSize) {
    m_displayList.appendItem(std::make_unique<TextDisplayItem>(text, x, y, color, fontFamily, fontSize));
}

void PaintContext::drawImage(const std::string& url, const layout::Rect& rect) {
    m_displayList.appendItem(std::make_unique<ImageDisplayItem>(url, rect));
}

void PaintContext::drawRect(const layout::Rect& rect, const Color& color, bool fill) {
    m_displayList.appendItem(std::make_unique<RectDisplayItem>(rect, color, fill));
}

void PaintContext::drawLine(float x1, float y1, float x2, float y2, const Color& color, float thickness) {
    m_displayList.appendItem(std::make_unique<LineDisplayItem>(x1, y1, x2, y2, color, thickness));
}

void PaintContext::transform(float dx, float dy) {
    m_displayList.appendItem(std::make_unique<TransformDisplayItem>(dx, dy));
}

void PaintContext::clip(const layout::Rect& rect) {
    m_displayList.appendItem(std::make_unique<ClipDisplayItem>(rect));
}

//-----------------------------------------------------------------------------
// PaintSystem Implementation
//-----------------------------------------------------------------------------

PaintSystem::PaintSystem() {
}

PaintSystem::~PaintSystem() {
}

bool PaintSystem::initialize() {
    return true;
}

PaintContext PaintSystem::createContext(layout::Box* box) {
    // Create an empty paint context
    PaintContext context;
    
    // Add a transform to offset to the box position
    if (box) {
        context.transform(box->contentRect().x, box->contentRect().y);
    }
    
    return context;
}

void PaintSystem::paintBox(layout::Box* box, PaintContext& context) {
    if (!box) {
        return;
    }
    
    // Skip boxes with display: none
    if (box->displayType() == layout::DisplayType::NONE) {
        return;
    }
    
    // Get box dimensions
    layout::Rect contentRect = box->contentRect();
    layout::Rect borderBox = box->borderBox();
    
    // Save the current position to create a local coordinate system
    float x = contentRect.x;
    float y = contentRect.y;
    
    // Draw background
    Color bgColor = getBackgroundColor(box);
    if (bgColor.a > 0) {
        context.drawBackground(borderBox, bgColor);
    }
    
    // Draw border
    if (box->borderTop() > 0 || box->borderRight() > 0 || 
        box->borderBottom() > 0 || box->borderLeft() > 0) {
        Color borderColor = getBorderColor(box);
        context.drawBorder(borderBox, borderColor, 
                         box->borderTop(), box->borderRight(), 
                         box->borderBottom(), box->borderLeft());
    }
    
    // Draw text content if this is a text box
    if (auto textBox = dynamic_cast<layout::TextBox*>(box)) {
        paintText(textBox, context);
    }
    
    // Add a transform to position children relative to this box
    context.transform(x, y);
    
    // Paint children
    for (const auto& child : box->children()) {
        paintBox(child.get(), context);
    }
    
    // Reset transform
    context.transform(-x, -y);
}

void PaintSystem::paintText(layout::TextBox* textBox, PaintContext& context) {
    if (!textBox || !textBox->textNode()) {
        return;
    }
    
    // Get text properties
    std::string text = textBox->textNode()->nodeValue();
    Color textColor = getTextColor(textBox);
    
    // Get font properties
    css::Value fontFamilyValue = textBox->style().getProperty("font-family");
    css::Value fontSizeValue = textBox->style().getProperty("font-size");
    
    std::string fontFamily = fontFamilyValue.stringValue();
    if (fontFamily.empty()) {
        fontFamily = "Arial";
    }
    
    float fontSize = 16.0f; // Default
    if (fontSizeValue.type() == css::ValueType::LENGTH) {
        fontSize = fontSizeValue.numericValue();
    }
    
    // Get content rect
    layout::Rect contentRect = textBox->contentRect();
    
    // Draw text
    context.drawText(text, contentRect.x, contentRect.y + fontSize, textColor, fontFamily, fontSize);
}

Color PaintSystem::getBackgroundColor(layout::Box* box) {
    if (!box) {
        return Color(255, 255, 255, 0); // Transparent
    }
    
    // Get background color from style
    css::Value bgColorValue = box->style().getProperty("background-color");
    return Color::fromCssColor(bgColorValue);
}

Color PaintSystem::getBorderColor(layout::Box* box) {
    if (!box) {
        return Color(0, 0, 0); // Black
    }
    
    // Get border color from style
    css::Value borderColorValue = box->style().getProperty("border-color");
    if (borderColorValue.stringValue().empty()) {
        // Default to black if not specified
        return Color(0, 0, 0);
    }
    return Color::fromCssColor(borderColorValue);
}

Color PaintSystem::getTextColor(layout::Box* box) {
    if (!box) {
        return Color(0, 0, 0); // Black
    }
    
    // Get text color from style
    css::Value textColorValue = box->style().getProperty("color");
    if (textColorValue.stringValue().empty()) {
        // Default to black if not specified
        return Color(0, 0, 0);
    }
    return Color::fromCssColor(textColorValue);
}

void PaintSystem::paintDisplayList(const DisplayList& displayList, RenderingContext* context) {
    if (!context) {
        return;
    }
    
    // Paint the display list to the context
    displayList.paint(context);
}

} // namespace rendering
} // namespace browser