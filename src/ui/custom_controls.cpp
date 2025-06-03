// Include minimal headers first
#ifdef _WIN32
#define NOMINMAX  // Prevent Windows.h from defining min/max macros
#endif

#include <iostream>
#include <algorithm>

#include "../rendering/custom_renderer.h"
// Include our own header first
#include "custom_controls.h"

// Then include necessary rendering headers 


// Finally include browser_window.h
#include "browser_window.h"

namespace browser {
namespace ui {

// Helper namespace for color operations
namespace ColorHelper {
    // Create a Color directly using its constructor
    inline rendering::Color createColor(unsigned char r, unsigned char g, unsigned char b, float a = 1.0f) {
        return rendering::Color(r, g, b, a);
    }
    
    // Avoid using assignment operator by setting fields directly
    inline void setPaintColor(rendering::Paint& paint, unsigned char r, unsigned char g, unsigned char b, float a = 1.0f) {
        paint.type = rendering::PaintType::COLOR;
        // Set color components directly instead of creating a temporary Color object
        paint.color.r = r;
        paint.color.g = g;
        paint.color.b = b;
        paint.color.a = a;
    }
}


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
    return wasHover != m_hover;
}

bool Control::handleMouseButton(int button, int action, int mods, int x, int y) {
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
    if (!ctx || !m_visible) return;
    
    // Draw button background
    ctx->beginPath();
    ctx->roundedRect(m_x, m_y, m_width, m_height, 3.0f);
    
    rendering::Paint fillPaint;
    if (!m_enabled) {
        // Disabled state
        ColorHelper::setPaintColor(fillPaint, 200, 200, 200);
    } else if (m_hover) {
        // Hover state
        ColorHelper::setPaintColor(fillPaint, 230, 230, 255);
    } else {
        // Normal state
        ColorHelper::setPaintColor(fillPaint, 240, 240, 240);
    }
    
    ctx->setFillPaint(fillPaint);
    ctx->fill();
    
    // Draw button border
    ctx->beginPath();
    ctx->roundedRect(m_x, m_y, m_width, m_height, 3.0f);
    
    rendering::Paint strokePaint;
    if (!m_enabled) {
        // Disabled state
        ColorHelper::setPaintColor(strokePaint, 180, 180, 180);
    } else if (m_hover) {
        // Hover state
        ColorHelper::setPaintColor(strokePaint, 100, 100, 255);
    } else {
        // Normal state
        ColorHelper::setPaintColor(strokePaint, 180, 180, 180);
    }
    
    ctx->setStrokePaint(strokePaint);
    ctx->setStrokeWidth(1.0f);
    ctx->stroke();
    
    // Draw button text
    ctx->setFont(rendering::Font("sans", 12.0f));
    
    rendering::Paint textPaint;
    if (!m_enabled) {
        // Disabled state
        ColorHelper::setPaintColor(textPaint, 128, 128, 128);
    } else {
        // Normal state
        ColorHelper::setPaintColor(textPaint, 0, 0, 0);
    }
    
    ctx->setFillPaint(textPaint);
    
    // Measure text width for centering
    float bounds[4];
    ctx->textBounds(0, 0, m_text, bounds);
    float textWidth = bounds[2];
    
    // Draw centered text
    float textX = m_x + (m_width - textWidth) / 2.0f;
    float textY = m_y + (m_height + 12.0f) / 2.0f;
    ctx->text(textX, textY, m_text);
}

bool Button::handleMouseButton(int button, int action, int mods, int x, int y) {
    if (m_enabled && button == 0 && action == 1 && contains(x, y)) { // Left button press
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
    if (!ctx || !m_visible) return;
    
    // Draw text field background
    ctx->beginPath();
    ctx->roundedRect(m_x, m_y, m_width, m_height, 3.0f);
    
    rendering::Paint bgPaint;
    if (!m_enabled) {
        // Disabled state
        ColorHelper::setPaintColor(bgPaint, 220, 220, 220);
    } else if (m_focused) {
        // Focused state
        ColorHelper::setPaintColor(bgPaint, 255, 255, 255);
    } else {
        // Normal state
        ColorHelper::setPaintColor(bgPaint, 255, 255, 255);
    }
    
    ctx->setFillPaint(bgPaint);
    ctx->fill();
    
    // Draw text field border
    ctx->beginPath();
    ctx->roundedRect(m_x, m_y, m_width, m_height, 3.0f);
    
    rendering::Paint borderPaint;
    if (!m_enabled) {
        // Disabled state
        ColorHelper::setPaintColor(borderPaint, 180, 180, 180);
    } else if (m_focused) {
        // Focused state
        ColorHelper::setPaintColor(borderPaint, 100, 100, 255);
    } else {
        // Normal state
        ColorHelper::setPaintColor(borderPaint, 180, 180, 180);
    }
    
    ctx->setStrokePaint(borderPaint);
    ctx->setStrokeWidth(1.0f);
    ctx->stroke();
    
    // Set font
    ctx->setFont(rendering::Font("sans", 12.0f));
    
    // Text padding
    float padding = 5.0f;
    
    // Draw text or placeholder
    if (m_text.empty() && !m_focused) {
        // Draw placeholder
        rendering::Paint placeholderPaint;
        ColorHelper::setPaintColor(placeholderPaint, 180, 180, 180);
        ctx->setFillPaint(placeholderPaint);
        ctx->text(m_x + padding, m_y + (m_height + 12.0f) / 2.0f, m_placeholder);
    } else {
        // Draw selection if present
        if (m_focused && m_cursorPos != m_selectionStart) {
            size_t selStart = std::min(m_cursorPos, m_selectionStart);
            size_t selEnd = std::max(m_cursorPos, m_selectionStart);
            
            // Get selection text bounds
            std::string beforeSel = m_text.substr(0, selStart);
            std::string selText = m_text.substr(selStart, selEnd - selStart);
            
            float beforeWidth = 0;
            if (!beforeSel.empty()) {
                float bounds[4];
                ctx->textBounds(0, 0, beforeSel, bounds);
                beforeWidth = bounds[2];
            }
            
            float selWidth = 0;
            if (!selText.empty()) {
                float bounds[4];
                ctx->textBounds(0, 0, selText, bounds);
                selWidth = bounds[2];
            }
            
            // Draw selection background
            ctx->beginPath();
            ctx->rect(m_x + padding + beforeWidth, 
                     m_y + (m_height - 12.0f) / 2.0f, 
                     selWidth, 
                     12.0f);
            
            rendering::Paint selPaint;
            ColorHelper::setPaintColor(selPaint, 100, 100, 255, 0.3f);
            ctx->setFillPaint(selPaint);
            ctx->fill();
        }
        
        // Draw text
        rendering::Paint textPaint;
        if (!m_enabled) {
            // Disabled state
            ColorHelper::setPaintColor(textPaint, 128, 128, 128);
        } else {
            // Normal state
            ColorHelper::setPaintColor(textPaint, 0, 0, 0);
        }
        
        ctx->setFillPaint(textPaint);
        ctx->text(m_x + padding, m_y + (m_height + 12.0f) / 2.0f, m_text);
        
        // Draw cursor if focused
        if (m_focused) {
            std::string beforeCursor = m_text.substr(0, m_cursorPos);
            float cursorX = m_x + padding;
            
            if (!beforeCursor.empty()) {
                float bounds[4];
                ctx->textBounds(0, 0, beforeCursor, bounds);
                cursorX += bounds[2];
            }
            
            ctx->beginPath();
            ctx->moveTo(cursorX, m_y + (m_height - 12.0f) / 2.0f);
            ctx->lineTo(cursorX, m_y + (m_height + 12.0f) / 2.0f);
            
            rendering::Paint cursorPaint;
            if (!m_enabled) {
                ColorHelper::setPaintColor(cursorPaint, 128, 128, 128);
            } else {
                ColorHelper::setPaintColor(cursorPaint, 0, 0, 0);
            }
            
            ctx->setStrokePaint(cursorPaint);
            ctx->setStrokeWidth(1.0f);
            ctx->stroke();
        }
    }
}

bool TextInput::handleMouseButton(int button, int action, int mods, int x, int y) {
    if (!m_enabled) return false;
    
    if (button == 0) { // Left button
        if (action == 1) { // Press
            if (contains(x, y)) {
                // First unfocus all other controls (you'll need to add this mechanism)
                m_focused = true;
                
                // Calculate cursor position based on click
                // For now, put cursor at end
                m_cursorPos = m_text.length();
                m_selectionStart = m_cursorPos;
                m_selecting = true;
                
                std::cout << "TextInput focused at " << x << "," << y << std::endl;
                return true;
            } else {
                m_focused = false;
                return false;
            }
        } else if (action == 0) { // Release
            m_selecting = false;
        }
    }
    
    return false;
}

bool TextInput::handleKeyInput(int key, int scancode, int action, int mods) {
    if (!m_enabled || !m_focused || action != 1) return false; // Only handle key press
    
    // Convert Key enum to actual values if needed
    // The Key enum values are being passed, not raw key codes
    Key keyEnum = static_cast<Key>(key);
    
    // Handle special keys first
    switch (keyEnum) {
        case Key::Left:
            moveCursor(-1, (mods & 0x0001) != 0);
            return true;
            
        case Key::Right:
            moveCursor(1, (mods & 0x0001) != 0);
            return true;
            
        case Key::Home:
            m_cursorPos = 0;
            if ((mods & 0x0001) == 0) {
                m_selectionStart = m_cursorPos;
            }
            return true;
            
        case Key::End:
            m_cursorPos = m_text.length();
            if ((mods & 0x0001) == 0) {
                m_selectionStart = m_cursorPos;
            }
            return true;
            
        case Key::Backspace:
            deletePreviousChar();
            return true;
            
        case Key::Delete:
            deleteNextChar();
            return true;
            
        case Key::Enter:
            if (m_submitHandler && !m_text.empty()) {
                std::cout << "Enter key pressed in TextInput, submitting: " << m_text << std::endl;
                m_submitHandler(m_text);
            }
            return true;
            
        case Key::Escape:
            m_focused = false;
            return true;
            
        default:
            // Handle alphanumeric keys
            if (key >= static_cast<int>(Key::A) && key <= static_cast<int>(Key::Z)) {
                char c = 'A' + (key - static_cast<int>(Key::A));
                if ((mods & 0x0001) == 0) { // Shift not pressed
                    c = c + 32; // Convert to lowercase
                }
                insertText(std::string(1, c));
                return true;
            }
            else if (key >= static_cast<int>(Key::Num0) && key <= static_cast<int>(Key::Num9)) {
                char c = '0' + (key - static_cast<int>(Key::Num0));
                insertText(std::string(1, c));
                return true;
            }
            else if (keyEnum == Key::Space) {
                insertText(" ");
                return true;
            }
            // Handle other printable characters
            else if (key >= 32 && key <= 126) {
                insertText(std::string(1, static_cast<char>(key)));
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

//-----------------------------------------------------------------------------
// ProgressBar Implementation
//-----------------------------------------------------------------------------

ProgressBar::ProgressBar(int x, int y, int width, int height)
    : Control(x, y, width, height)
    , m_value(0.0f)
    , m_indeterminate(false)
    , m_animationOffset(0.0f)
{
}

ProgressBar::~ProgressBar() {
}

void ProgressBar::setValue(float value) {
    m_value = std::max(0.0f, std::min(1.0f, value));
}

float ProgressBar::getValue() const {
    return m_value;
}

void ProgressBar::setIndeterminate(bool indeterminate) {
    m_indeterminate = indeterminate;
}

bool ProgressBar::isIndeterminate() const {
    return m_indeterminate;
}

void ProgressBar::draw(rendering::CustomRenderContext* ctx) {
    if (!ctx || !m_visible) return;
    
    // Draw progress bar background
    ctx->beginPath();
    ctx->roundedRect(m_x, m_y, m_width, m_height, 3.0f);
    
    rendering::Paint bgPaint;
    ColorHelper::setPaintColor(bgPaint, 220, 220, 220);
    ctx->setFillPaint(bgPaint);
    ctx->fill();
    
    // Draw progress bar border
    ctx->beginPath();
    ctx->roundedRect(m_x, m_y, m_width, m_height, 3.0f);
    
    rendering::Paint borderPaint;
    ColorHelper::setPaintColor(borderPaint, 180, 180, 180);
    ctx->setStrokePaint(borderPaint);
    ctx->setStrokeWidth(1.0f);
    ctx->stroke();
    
    if (m_indeterminate) {
        // Draw indeterminate progress
        float barWidth = m_width * 0.3f; // 30% of the width
        float x = m_x + m_animationOffset * (m_width - barWidth);
        
        ctx->beginPath();
        ctx->roundedRect(x, m_y + 1, barWidth, m_height - 2, 2.0f);
        
        rendering::Paint progressPaint;
        ColorHelper::setPaintColor(progressPaint, 100, 100, 255);
        ctx->setFillPaint(progressPaint);
        ctx->fill();
        
        // Update animation
        m_animationOffset += 0.01f;
        if (m_animationOffset > 1.0f) {
            m_animationOffset = 0.0f;
        }
    } else {
        // Draw determinate progress
        float progressWidth = m_width * m_value;
        
        if (progressWidth > 0) {
            ctx->beginPath();
            ctx->roundedRect(m_x + 1, m_y + 1, progressWidth - 2, m_height - 2, 2.0f);
            
            rendering::Paint progressPaint;
            ColorHelper::setPaintColor(progressPaint, 100, 100, 255);
            ctx->setFillPaint(progressPaint);
            ctx->fill();
        }
    }
}

//-----------------------------------------------------------------------------
// Toolbar Implementation
//-----------------------------------------------------------------------------

Toolbar::Toolbar(int x, int y, int width, int height)
    : Control(x, y, width, height)
{
}

Toolbar::~Toolbar() {
}

void Toolbar::addControl(std::shared_ptr<Control> control) {
    if (control) {
        m_controls.push_back(control);
    }
}

void Toolbar::draw(rendering::CustomRenderContext* ctx) {
    if (!ctx || !m_visible) return;
    
    // Draw toolbar background
    ctx->beginPath();
    ctx->rect(m_x, m_y, m_width, m_height);
    
    rendering::Paint bgPaint;
    ColorHelper::setPaintColor(bgPaint, 240, 240, 240);
    ctx->setFillPaint(bgPaint);
    ctx->fill();
    
    // Draw toolbar border
    ctx->beginPath();
    ctx->rect(m_x, m_y, m_width, m_height);
    
    rendering::Paint borderPaint;
    ColorHelper::setPaintColor(borderPaint, 200, 200, 200);
    ctx->setStrokePaint(borderPaint);
    ctx->setStrokeWidth(1.0f);
    ctx->stroke();
    
    // Draw controls
    for (auto& control : m_controls) {
        if (control->isVisible()) {
            control->draw(ctx);
        }
    }
}

bool Control::handleKeyInput(int key, int scancode, int action, int mods) {
    // Base implementation doesn't handle keys
    return false;
}

bool Toolbar::handleMouseMove(int x, int y) {
    bool handled = Control::handleMouseMove(x, y);
    
    // Forward to controls
    for (auto& control : m_controls) {
        if (control->isVisible() && control->isEnabled()) {
            handled |= control->handleMouseMove(x, y);
        }
    }
    
    return handled;
}

bool Toolbar::handleMouseButton(int button, int action, int mods, int mouseX, int mouseY) {
    bool handled = Control::handleMouseButton(button, action, mods, mouseX, mouseY);
    
    // Forward to controls
    for (auto& control : m_controls) {
        if (control->isVisible() && control->isEnabled() && control->contains(mouseX, mouseY)) {
            handled |= control->handleMouseButton(button, action, mods, mouseX, mouseY);
        }
    }
    
    return handled;
}

bool Toolbar::handleKeyInput(int key, int scancode, int action, int mods) {
    bool handled = Control::handleKeyInput(key, scancode, action, mods);
    
    // Forward to focused control
    for (auto& control : m_controls) {
        if (control->isVisible() && control->isEnabled() && control->hasFocus()) {
            handled |= control->handleKeyInput(key, scancode, action, mods);
        }
    }
    
    return handled;
}

//-----------------------------------------------------------------------------
// BrowserControls Implementation
//-----------------------------------------------------------------------------

BrowserControls::BrowserControls(BrowserWindow* window)
    : m_window(window)
    , m_renderContext(nullptr)
    , m_toolbarHeight(40)
{
}

BrowserControls::~BrowserControls() {
}

bool BrowserControls::initialize() {
    if (!m_window) {
        return false;
    }
    
    // Get window size
    int width, height;
    m_window->getSize(width, height);
    
    // Initialize renderer
    if (!initializeRenderer()) {
        return false;
    }
    
    // Create toolbar
    m_toolbar = std::make_shared<Toolbar>(0, 0, width, m_toolbarHeight);
    
    // Create controls
    layoutControls();
    
    return true;
}

void BrowserControls::draw(rendering::CustomRenderContext* ctx) {
    if (!ctx) return;
    
    m_renderContext = ctx;
    
    // Draw toolbar
    if (m_toolbar) {
        m_toolbar->draw(ctx);
    }
}

bool BrowserControls::handleMouseMove(int x, int y) {
    // Forward to toolbar
    if (m_toolbar && m_toolbar->isVisible()) {
        return m_toolbar->handleMouseMove(x, y);
    }
    
    return false;
}

bool BrowserControls::handleMouseButton(int button, int action, int mods, int x, int y) {
    // Forward to toolbar
    if (m_toolbar && m_toolbar->isVisible()) {
        return m_toolbar->handleMouseButton(button, action, mods, x, y);
    }
    
    return false;
}

bool BrowserControls::handleKeyInput(int key, int scancode, int action, int mods) {
    // Forward to toolbar
    if (m_toolbar && m_toolbar->isVisible()) {
        return m_toolbar->handleKeyInput(key, scancode, action, mods);
    }
    
    return false;
}

bool BrowserControls::handleResize(int width, int height) {
    // Resize toolbar
    if (m_toolbar) {
        m_toolbar->setSize(width, m_toolbarHeight);
    }
    
    // Re-layout controls
    layoutControls();
    
    return true;
}

std::string BrowserControls::getAddressBarText() const {
    if (m_addressBar) {
        return m_addressBar->getText();
    }
    return "";
}

void BrowserControls::setAddressBarText(const std::string& text) {
    if (m_addressBar) {
        m_addressBar->setText(text);
    }
}

void BrowserControls::setLoading(bool loading) {
    if (m_reloadButton) {
        m_reloadButton->setVisible(!loading);
    }
    
    if (m_stopButton) {
        m_stopButton->setVisible(loading);
    }
    
    if (m_progressBar) {
        m_progressBar->setVisible(loading);
        if (loading) {
            m_progressBar->setIndeterminate(true);
        } else {
            m_progressBar->setValue(1.0f); // 100% complete
            // Hide after a short delay
            // In a real implementation, you'd use a timer
        }
    }
}

bool BrowserControls::initializeRenderer() {
    // In a real implementation, this would initialize the renderer
    // For this simple example, we'll assume the renderer is already initialized
    return true;
}

void BrowserControls::layoutControls() {
    if (!m_toolbar) return;
    
    // Get toolbar size
    int width, height;
    m_toolbar->getSize(width, height);
    
    // Define button dimensions
    int buttonWidth = 32;
    int buttonHeight = 32;
    int buttonPadding = 4;
    int buttonY = (height - buttonHeight) / 2;
    int addressBarHeight = 28;
    int addressBarY = (height - addressBarHeight) / 2;
    
    // Create back button
    if (!m_backButton) {
        m_backButton = std::make_shared<Button>(
            buttonPadding, 
            buttonY, 
            buttonWidth, 
            buttonHeight, 
            "◀"
        );
        m_backButton->setClickHandler([this]() {
            if (m_window) {
                std::cout << "Back button clicked" << std::endl;
                m_window->goBack();
            }
        });
        m_toolbar->addControl(m_backButton);
    } else {
        m_backButton->setPosition(buttonPadding, buttonY);
        m_backButton->setSize(buttonWidth, buttonHeight);
    }
    
    // Create forward button
    int forwardX = buttonPadding * 2 + buttonWidth;
    if (!m_forwardButton) {
        m_forwardButton = std::make_shared<Button>(
            forwardX, 
            buttonY, 
            buttonWidth, 
            buttonHeight, 
            "▶"
        );
        m_forwardButton->setClickHandler([this]() {
            if (m_window) {
                std::cout << "Forward button clicked" << std::endl;
                m_window->goForward();
            }
        });
        m_toolbar->addControl(m_forwardButton);
    } else {
        m_forwardButton->setPosition(forwardX, buttonY);
        m_forwardButton->setSize(buttonWidth, buttonHeight);
    }
    
    // Create reload button
    int reloadX = buttonPadding * 3 + buttonWidth * 2;
    if (!m_reloadButton) {
        m_reloadButton = std::make_shared<Button>(
            reloadX, 
            buttonY, 
            buttonWidth, 
            buttonHeight, 
            "↻"
        );
        m_reloadButton->setClickHandler([this]() {
            if (m_window) {
                std::cout << "Reload button clicked" << std::endl;
                m_window->reload();
            }
        });
        m_toolbar->addControl(m_reloadButton);
    } else {
        m_reloadButton->setPosition(reloadX, buttonY);
        m_reloadButton->setSize(buttonWidth, buttonHeight);
    }
    
    // Create stop button (hidden initially, shares position with reload)
    if (!m_stopButton) {
        m_stopButton = std::make_shared<Button>(
            reloadX, 
            buttonY, 
            buttonWidth, 
            buttonHeight, 
            "✕"
        );
        m_stopButton->setClickHandler([this]() {
            if (m_window) {
                std::cout << "Stop button clicked" << std::endl;
                m_window->stopLoading();
            }
        });
        m_stopButton->setVisible(false); // Hide initially
        m_toolbar->addControl(m_stopButton);
    } else {
        m_stopButton->setPosition(reloadX, buttonY);
        m_stopButton->setSize(buttonWidth, buttonHeight);
    }
    
    // Create home button (optional, but useful)
    int homeX = buttonPadding * 4 + buttonWidth * 3;
    if (!m_homeButton) {
        m_homeButton = std::make_shared<Button>(
            homeX,
            buttonY,
            buttonWidth,
            buttonHeight,
            "⌂"
        );
        m_homeButton->setClickHandler([this]() {
            if (m_window) {
                std::cout << "Home button clicked" << std::endl;
                m_window->loadUrl("about:home");
            }
        });
        m_toolbar->addControl(m_homeButton);
    } else {
        m_homeButton->setPosition(homeX, buttonY);
        m_homeButton->setSize(buttonWidth, buttonHeight);
    }
    
    // Create address bar
    int addressBarX = buttonPadding * 5 + buttonWidth * 4;
    int addressBarWidth = width - addressBarX - buttonPadding * 2 - buttonWidth; // Leave space for go button
    
    if (!m_addressBar) {
        m_addressBar = std::make_shared<TextInput>(
            addressBarX,
            addressBarY,
            addressBarWidth,
            addressBarHeight
        );
        m_addressBar->setPlaceholder("Enter URL or search...");
        
        // Handle Enter key press
        m_addressBar->setSubmitHandler([this](const std::string& text) {
            if (m_window && !text.empty()) {
                std::cout << "Address bar submitted: " << text << std::endl;
                m_window->loadUrl(text);
            }
        });
        
        // Handle text changes (optional - for auto-suggestions in future)
        m_addressBar->setTextChangeHandler([this](const std::string& text) {
            // Could implement auto-suggestions here
            std::cout << "Address bar text changed: " << text << std::endl;
        });
        
        m_toolbar->addControl(m_addressBar);
    } else {
        m_addressBar->setPosition(addressBarX, addressBarY);
        m_addressBar->setSize(addressBarWidth, addressBarHeight);
    }
    
    // Create go button (next to address bar)
    int goButtonX = addressBarX + addressBarWidth + buttonPadding;
    if (!m_goButton) {
        m_goButton = std::make_shared<Button>(
            goButtonX,
            buttonY,
            buttonWidth,
            buttonHeight,
            "→"
        );
        m_goButton->setClickHandler([this]() {
            if (m_window && m_addressBar) {
                std::string url = m_addressBar->getText();
                if (!url.empty()) {
                    std::cout << "Go button clicked with URL: " << url << std::endl;
                    m_window->loadUrl(url);
                }
            }
        });
        m_toolbar->addControl(m_goButton);
    } else {
        m_goButton->setPosition(goButtonX, buttonY);
        m_goButton->setSize(buttonWidth, buttonHeight);
    }
    
    // Create progress bar (at bottom of toolbar)
    if (!m_progressBar) {
        m_progressBar = std::make_shared<ProgressBar>(
            0,
            height - 3,  // 3 pixels from bottom
            width,
            3           // 3 pixels tall
        );
        m_progressBar->setVisible(false); // Hide initially
        m_toolbar->addControl(m_progressBar);
    } else {
        m_progressBar->setPosition(0, height - 3);
        m_progressBar->setSize(width, 3);
    }
    
    // Update initial button states
    updateNavigationState();
}

// Add this helper method to update navigation button states
void BrowserControls::updateNavigationState() {
    if (!m_window) return;
    
    // Update back button state
    if (m_backButton) {
        // You'll need to add a method to check if can go back
        // For now, just enable it
        m_backButton->setEnabled(true);
    }
    
    // Update forward button state
    if (m_forwardButton) {
        // You'll need to add a method to check if can go forward
        // For now, just enable it
        m_forwardButton->setEnabled(true);
    }
}

} // namespace ui
} // namespace browser