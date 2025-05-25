// src/ui/custom_controls.cpp
#include "custom_controls.h"
#include "browser_window.h"
#include "../rendering/paint_system.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace browser {
namespace ui {

//-----------------------------------------------------------------------------
// Control Implementation
//-----------------------------------------------------------------------------

Control::Control(int x, int y, int width, int height)
    : m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_visible(true)
    , m_enabled(true)
    , m_hover(false)
    , m_focused(false)
{
}

Control::~Control() {
}

// [Keep all the unchanged methods...]

//-----------------------------------------------------------------------------
// Button Implementation
//-----------------------------------------------------------------------------

Button::Button(int x, int y, int width, int height, const std::string& text)
    : Control(x, y, width, height)
    , m_text(text)
{
}

Button::~Button() {
}

void Button::setText(const std::string& text) {
    m_text = text;
}

std::string Button::getText() const {
    return m_text;
}

void Button::setClickHandler(std::function<void()> handler) {
    m_clickHandler = handler;
}

void Button::draw(rendering::CustomRenderContext* ctx) {
    if (!ctx) return;
    
    // Create a paint context
    rendering::PaintContext paintContext;
    
    // Choose colors based on state
    unsigned int bgColor;
    unsigned int textColor;
    unsigned int borderColor;
    
    if (!m_enabled) {
        // Disabled state
        bgColor = 0xC8C8C8; // 200, 200, 200
        textColor = 0x808080; // 128, 128, 128
        borderColor = 0xB4B4B4; // 180, 180, 180
    } else if (m_hover) {
        // Hover state
        bgColor = 0xE6E6FF; // 230, 230, 255
        textColor = 0x000000; // 0, 0, 0
        borderColor = 0x6464FF; // 100, 100, 255
    } else {
        // Normal state
        bgColor = 0xF0F0F0; // 240, 240, 240
        textColor = 0x000000; // 0, 0, 0
        borderColor = 0xB4B4B4; // 180, 180, 180
    }
    
    // Create layout rectangle for button
    layout::Rect buttonRect(m_x, m_y, m_width, m_height);
    
    // Draw button background (rounded rectangle)
    paintContext.drawRect(buttonRect, rendering::Color::fromRGB(bgColor), true);
    
    // Draw button border
    paintContext.drawRect(buttonRect, rendering::Color::fromRGB(borderColor), false, 1);
    
    // Draw button text
    float textX = m_x + (m_width - m_text.length() * 8) / 2; // Approximate text centering
    float textY = m_y + (m_height + 12) / 2; // Approximate vertical centering
    paintContext.drawText(m_text, textX, textY, rendering::Color::fromRGB(textColor), "sans", 14);
    
    // Paint the display list to the rendering context
    rendering::PaintSystem paintSystem;
    paintSystem.paintDisplayList(paintContext.displayList(), ctx);
}

bool Button::handleMouseButton(int button, int action, int mods) {
    // Call base handler
    Control::handleMouseButton(button, action, mods);
    
    // Handle click
    if (m_enabled && button == 0 && action == 1 && m_hover) {
        if (m_clickHandler) {
            m_clickHandler();
        }
        return true;
    }
    
    return false;
}

//-----------------------------------------------------------------------------
// TextInput Implementation
//-----------------------------------------------------------------------------

TextInput::TextInput(int x, int y, int width, int height)
    : Control(x, y, width, height)
    , m_text("")
    , m_placeholder("Enter text...")
    , m_cursorPos(0)
    , m_selectionStart(0)
    , m_selecting(false)
{
}

TextInput::~TextInput() {
}

// [Keep other unchanged methods...]

void TextInput::draw(rendering::CustomRenderContext* ctx) {
    if (!ctx) return;
    
    // Create a paint context
    rendering::PaintContext paintContext;
    
    // Choose colors based on state
    unsigned int bgColor;
    unsigned int textColor;
    unsigned int borderColor;
    unsigned int placeholderColor;
    
    if (!m_enabled) {
        // Disabled state
        bgColor = 0xDCDCDC; // 220, 220, 220
        textColor = 0x808080; // 128, 128, 128
        borderColor = 0xB4B4B4; // 180, 180, 180
        placeholderColor = 0xA0A0A0; // 160, 160, 160
    } else if (m_focused) {
        // Focused state
        bgColor = 0xFFFFFF; // 255, 255, 255
        textColor = 0x000000; // 0, 0, 0
        borderColor = 0x6464FF; // 100, 100, 255
        placeholderColor = 0xB4B4B4; // 180, 180, 180
    } else {
        // Normal state
        bgColor = 0xFFFFFF; // 255, 255, 255
        textColor = 0x000000; // 0, 0, 0
        borderColor = 0xB4B4B4; // 180, 180, 180
        placeholderColor = 0xB4B4B4; // 180, 180, 180
    }
    
    // Create layout rectangle for text input
    layout::Rect inputRect(m_x, m_y, m_width, m_height);
    
    // Draw text input background
    paintContext.drawRect(inputRect, rendering::Color::fromRGB(bgColor), true);
    
    // Draw text input border
    paintContext.drawRect(inputRect, rendering::Color::fromRGB(borderColor), false, 1);
    
    // Set up text rendering
    float textPadding = 5.0f;
    float textY = m_y + (m_height + 12) / 2; // Approximate vertical centering
    
    // Draw text or placeholder
    if (m_text.empty()) {
        if (!m_focused) {
            // Draw placeholder
            paintContext.drawText(m_placeholder, m_x + textPadding, textY,
                                 rendering::Color::fromRGB(placeholderColor), "sans", 14);
        }
    } else {
        // Draw text
        paintContext.drawText(m_text, m_x + textPadding, textY,
                             rendering::Color::fromRGB(textColor), "sans", 14);
    }
    
    // Draw cursor and selection if focused
    if (m_focused) {
        // Set up text measurements
        float charWidth = 8.0f; // Approximate character width
        float cursorX = m_x + textPadding + m_cursorPos * charWidth;
        
        // Draw selection if present
        if (m_cursorPos != m_selectionStart) {
            size_t start = std::min(m_cursorPos, m_selectionStart);
            size_t end = std::max(m_cursorPos, m_selectionStart);
            
            float selX = m_x + textPadding + start * charWidth;
            float selWidth = (end - start) * charWidth;
            
            // Draw selection background
            layout::Rect selectionRect(selX, m_y + 2.0f, selWidth, m_height - 4.0f);
            paintContext.drawRect(selectionRect, rendering::Color(100, 100, 255, 128), true);
        }
        
        // Draw cursor
        paintContext.drawLine(cursorX, m_y + 4.0f, cursorX, m_y + m_height - 4.0f,
                             rendering::Color::fromRGB(0x000000), 1);
    }
    
    // Paint the display list to the rendering context
    rendering::PaintSystem paintSystem;
    paintSystem.paintDisplayList(paintContext.displayList(), ctx);
}

// [Keep other unchanged methods...]

} // namespace ui
} // namespace browser