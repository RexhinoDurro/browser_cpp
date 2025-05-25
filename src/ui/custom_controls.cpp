// src/ui/custom_controls.cpp
#include "custom_controls.h"
#include "browser_window.h"
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

void Control::setPosition(int x, int y) {
    m_x = x;
    m_y = y;
}

void Control::getPosition(int& x, int& y) const {
    x = m_x;
    y = m_y;
}

void Control::setSize(int width, int height) {
    m_width = width;
    m_height = height;
}

void Control::getSize(int& width, int& height) const {
    width = m_width;
    height = m_height;
}

void Control::setVisible(bool visible) {
    m_visible = visible;
}

bool Control::isVisible() const {
    return m_visible;
}

void Control::setEnabled(bool enabled) {
    m_enabled = enabled;
}

bool Control::isEnabled() const {
    return m_enabled;
}

void Control::setFocus(bool focus) {
    m_focused = focus;
}

bool Control::hasFocus() const {
    return m_focused;
}

bool Control::handleMouseMove(int x, int y) {
    bool wasHover = m_hover;
    m_hover = contains(x, y);
    
    // Return true if hover state changed
    return wasHover != m_hover;
}

bool Control::handleMouseButton(int button, int action, int mods) {
    // Base implementation just handles focus
    if (button == 0 && action == 1 && contains(button, action)) {
        m_focused = true;
        return true;
    }
    
    return false;
}

bool Control::handleKeyInput(int key, int scancode, int action, int mods) {
    // Base implementation doesn't handle keys
    return false;
}

bool Control::contains(int x, int y) const {
    return x >= m_x && x < m_x + m_width && y >= m_y && y < m_y + m_height;
}

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
    
    // Save state
    ctx->save();
    
    // Choose colors based on state
    rendering::Color bgColor, textColor, borderColor;
    
    if (!m_enabled) {
        // Disabled state
        bgColor = rendering::Color(200, 200, 200);
        textColor = rendering::Color(128, 128, 128);
        borderColor = rendering::Color(180, 180, 180);
    } else if (m_hover) {
        // Hover state
        bgColor = rendering::Color(230, 230, 255);
        textColor = rendering::Color(0, 0, 0);
        borderColor = rendering::Color(100, 100, 255);
    } else {
        // Normal state
        bgColor = rendering::Color(240, 240, 240);
        textColor = rendering::Color(0, 0, 0);
        borderColor = rendering::Color(180, 180, 180);
    }
    
    // Draw button background
    ctx->beginPath();
    ctx->roundedRect(m_x, m_y, m_width, m_height, 3.0f);
    
    rendering::Paint fillPaint;
    fillPaint.setColor(bgColor);
    ctx->setFillPaint(fillPaint);
    ctx->fill();
    
    // Draw button border
    ctx->beginPath();
    ctx->roundedRect(m_x, m_y, m_width, m_height, 3.0f);
    
    rendering::Paint strokePaint;
    strokePaint.setColor(borderColor);
    ctx->setStrokePaint(strokePaint);
    ctx->setStrokeWidth(1.0f);
    ctx->stroke();
    
    // Draw button text
    rendering::Paint textPaint;
    textPaint.setColor(textColor);
    ctx->setFillPaint(textPaint);
    ctx->setFont(rendering::Font("sans", 14.0f));
    
    // Measure text
    float bounds[4];
    ctx->textBounds(0, 0, m_text, bounds);
    float textWidth = bounds[2] - bounds[0];
    
    // Center text
    float textX = m_x + (m_width - textWidth) / 2;
    float textY = m_y + (m_height + 14.0f) / 2;
    
    ctx->text(textX, textY, m_text);
    
    // Restore state
    ctx->restore();
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

void TextInput::setText(const std::string& text) {
    m_text = text;
    m_cursorPos = text.length();
    m_selectionStart = m_cursorPos;
    
    // Notify change handler
    if (m_textChangeHandler) {
        m_textChangeHandler(m_text);
    }
}

std::string TextInput::getText() const {
    return m_text;
}

void TextInput::setPlaceholder(const std::string& placeholder) {
    m_placeholder = placeholder;
}

void TextInput::setSubmitHandler(std::function<void(const std::string&)> handler) {
    m_submitHandler = handler;
}

void TextInput::setTextChangeHandler(std::function<void(const std::string&)> handler) {
    m_textChangeHandler = handler;
}

void TextInput::draw(rendering::CustomRenderContext* ctx) {
    if (!ctx) return;
    
    // Save state
    ctx->save();
    
    // Choose colors based on state
    rendering::Color bgColor, textColor, borderColor, placeholderColor;
    
    if (!m_enabled) {
        // Disabled state
        bgColor = rendering::Color(220, 220, 220);
        textColor = rendering::Color(128, 128, 128);
        borderColor = rendering::Color(180, 180, 180);
        placeholderColor = rendering::Color(160, 160, 160);
    } else if (m_focused) {
        // Focused state
        bgColor = rendering::Color(255, 255, 255);
        textColor = rendering::Color(0, 0, 0);
        borderColor = rendering::Color(100, 100, 255);
        placeholderColor = rendering::Color(180, 180, 180);
    } else {
        // Normal state
        bgColor = rendering::Color(255, 255, 255);
        textColor = rendering::Color(0, 0, 0);
        borderColor = rendering::Color(180, 180, 180);
        placeholderColor = rendering::Color(180, 180, 180);
    }
    
    // Draw text input background
    ctx->beginPath();
    ctx->roundedRect(m_x, m_y, m_width, m_height, 3.0f);
    
    rendering::Paint bgPaint;
    bgPaint.setColor(bgColor);
    ctx->setFillPaint(bgPaint);
    ctx->fill();
    
    // Draw text input border
    ctx->beginPath();
    ctx->roundedRect(m_x, m_y, m_width, m_height, 3.0f);
    
    rendering::Paint borderPaint;
    borderPaint.setColor(borderColor);
    ctx->setStrokePaint(borderPaint);
    ctx->setStrokeWidth(1.0f);
    ctx->stroke();
    
    // Set up text rendering
    ctx->setFont(rendering::Font("sans", 14.0f));
    
    float textPadding = 5.0f;
    float textY = m_y + (m_height + 14.0f) / 2;
    
    // Draw text or placeholder
    if (m_text.empty()) {
        if (!m_focused) {
            // Draw placeholder
            rendering::Paint placeholderPaint;
            placeholderPaint.setColor(placeholderColor);
            ctx->setFillPaint(placeholderPaint);
            ctx->text(m_x + textPadding, textY, m_placeholder);
        }
    } else {
        // Draw text
        rendering::Paint textPaint;
        textPaint.setColor(textColor);
        ctx->setFillPaint(textPaint);
        ctx->text(m_x + textPadding, textY, m_text);
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
            ctx->beginPath();
            ctx->rect(selX, m_y + 2.0f, selWidth, m_height - 4.0f);
            
            rendering::Color selColor(100, 100, 255, 0.5f);
            rendering::Paint selPaint;
            selPaint.setColor(selColor);
            ctx->setFillPaint(selPaint);
            ctx->fill();
        }
        
        // Draw cursor
        ctx->beginPath();
        ctx->moveTo(cursorX, m_y + 4.0f);
        ctx->lineTo(cursorX, m_y + m_height - 4.0f);
        
        rendering::Paint cursorPaint;
        cursorPaint.setColor(rendering::Color(0, 0, 0));
        ctx->setStrokePaint(cursorPaint);
        ctx->setStrokeWidth(1.0f);
        ctx->stroke();
    }
    
    // Restore state
    ctx->restore();
}

bool TextInput::handleKeyInput(int key, int scancode, int action, int mods) {
    if (!m_focused || !m_enabled || action != 1) return false;
    
    // Check for key presses
    switch (key) {
        case 263: // Left arrow
            moveCursor(-1, (mods & 0x01) != 0); // Shift modifier
            return true;
            
        case 262: // Right arrow
            moveCursor(1, (mods & 0x01) != 0); // Shift modifier
            return true;
            
        case 268: // Home
            m_cursorPos = 0;
            if (!(mods & 0x01)) { // If Shift is not pressed
                m_selectionStart = m_cursorPos;
            }
            return true;
            
        case 269: // End
            m_cursorPos = m_text.length();
            if (!(mods & 0x01)) { // If Shift is not pressed
                m_selectionStart = m_cursorPos;
            }
            return true;
            
        case 259: // Backspace
            deletePreviousChar();
            return true;
            
        case 261: // Delete
            deleteNextChar();
            return true;
            
        case 257: // Enter
            if (m_submitHandler) {
                m_submitHandler(m_text);
            }
            return true;
            
        default:
            // Handle printable characters
            if (key >= 32 && key <= 126) {
                char c = (char)key;
                insertText(std::string(1, c));
                return true;
            }
    }
    
    return false;
}

bool TextInput::handleMouseButton(int button, int action, int mods) {
    // Call base handler
    Control::handleMouseButton(button, action, mods);
    
    // Handle mouse interactions
    if (m_enabled && button == 0) {
        if (action == 1 && m_hover) { // Mouse down
            // Calculate cursor position from click
            float textPadding = 5.0f;
            float charWidth = 8.0f; // Approximate character width
            
            int clickX = button - (m_x + textPadding);
            m_cursorPos = std::max(0, std::min((int)m_text.length(), (int)(clickX / charWidth)));
            
            // Start selection
            m_selectionStart = m_cursorPos;
            m_selecting = true;
            
            return true;
        } else if (action == 0) { // Mouse up
            m_selecting = false;
            return true;
        }
    }
    
    return false;
}

void TextInput::moveCursor(int direction, bool selecting) {
    // Move cursor left or right
    if (direction < 0) {
        m_cursorPos = m_cursorPos > 0 ? m_cursorPos - 1 : 0;
    } else {
        m_cursorPos = std::min(m_cursorPos + 1, m_text.length());
    }
    
    // Update selection
    if (!selecting) {
        m_selectionStart = m_cursorPos;
    }
}

void TextInput::deletePreviousChar() {
    if (m_cursorPos != m_selectionStart) {
        // Delete selection
        size_t start = std::min(m_cursorPos, m_selectionStart);
        size_t end = std::max(m_cursorPos, m_selectionStart);
        m_text.erase(start, end - start);
        m_cursorPos = start;
        m_selectionStart = m_cursorPos;
    } else if (m_cursorPos > 0) {
        // Delete previous character
        m_text.erase(m_cursorPos - 1, 1);
        m_cursorPos--;
        m_selectionStart = m_cursorPos;
    }
    
    // Notify change handler
    if (m_textChangeHandler) {
        m_textChangeHandler(m_text);
    }
}

void TextInput::deleteNextChar() {
    if (m_cursorPos != m_selectionStart) {
        // Delete selection
        size_t start = std::min(m_cursorPos, m_selectionStart);
        size_t end = std::max(m_cursorPos, m_selectionStart);
        m_text.erase(start, end - start);
        m_cursorPos = start;
        m_selectionStart = m_cursorPos;
    } else if (m_cursorPos < m_text.length()) {
        // Delete next character
        m_text.erase(m_cursorPos, 1);
    }
    
    // Notify change handler
    if (m_textChangeHandler) {
        m_textChangeHandler(m_text);
    }
}

void TextInput::insertText(const std::string& text) {
    if (m_cursorPos != m_selectionStart) {
        // Replace selection
        size_t start = std::min(m_cursorPos, m_selectionStart);
        size_t end = std::max(m_cursorPos, m_selectionStart);
        m_text.replace(start, end - start, text);
        m_cursorPos = start + text.length();
    } else {
        // Insert at cursor
        m_text.insert(m_cursorPos, text);
        m_cursorPos += text.length();
    }
    
    m_selectionStart = m_cursorPos;
    
    // Notify change handler
    if (m_textChangeHandler) {
        m_textChangeHandler(m_text);
    }
}

} // namespace ui
} // namespace browser