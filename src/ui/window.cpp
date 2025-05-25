// src/ui/window.cpp - Base window implementation

#include "window.h"
#include <algorithm>

#ifdef _WIN32
#include "window_win32.h"
#elif defined(__APPLE__)
#include "window_macos.h"
#else
#include "window_x11.h"
#endif

namespace browser {
namespace ui {

//-----------------------------------------------------------------------------
// Window Implementation
//-----------------------------------------------------------------------------

Window::Window(const WindowConfig& config)
    : m_config(config)
{
}

Window::~Window() {
    // Clear controls
    m_controls.clear();
}

void Window::setTitle(const std::string& title) {
    m_config.title = title;
}

std::string Window::getTitle() const {
    return m_config.title;
}

void Window::setSize(int width, int height) {
    m_config.width = width;
    m_config.height = height;
}

void Window::getSize(int& width, int& height) const {
    width = m_config.width;
    height = m_config.height;
}

void Window::setPosition(int x, int y) {
    // Base implementation does nothing
}

void Window::getPosition(int& x, int& y) const {
    // Base implementation returns 0,0
    x = 0;
    y = 0;
}

void Window::setKeyCallback(std::function<void(Key, KeyAction)> callback) {
    m_keyCallback = callback;
}

void Window::setMouseButtonCallback(std::function<void(MouseButton, MouseAction, int, int)> callback) {
    m_mouseButtonCallback = callback;
}

void Window::setMouseMoveCallback(std::function<void(int, int)> callback) {
    m_mouseMoveCallback = callback;
}

void Window::setResizeCallback(std::function<void(int, int)> callback) {
    m_resizeCallback = callback;
}

void Window::setCloseCallback(std::function<void()> callback) {
    m_closeCallback = callback;
}

void Window::addControl(std::shared_ptr<UIControl> control) {
    if (control) {
        m_controls.push_back(control);
    }
}

void Window::removeControl(std::shared_ptr<UIControl> control) {
    auto it = std::find(m_controls.begin(), m_controls.end(), control);
    if (it != m_controls.end()) {
        m_controls.erase(it);
    }
}

void Window::notifyKeyEvent(Key key, KeyAction action) {
    // First give focused controls a chance to handle the event
    for (auto& control : m_controls) {
        if (control->isVisible() && control->isEnabled() && control->hasFocus()) {
            if (control->handleKeyInput(key, action)) {
                return; // Event handled
            }
        }
    }
    
    // Then call the window's key callback
    if (m_keyCallback) {
        m_keyCallback(key, action);
    }
}

void Window::notifyMouseButtonEvent(MouseButton button, MouseAction action, int x, int y) {
    // First give controls under the cursor a chance to handle the event
    for (auto& control : m_controls) {
        if (control->isVisible() && control->isEnabled() && control->contains(x, y)) {
            if (control->handleMouseButton(button, action, x, y)) {
                return; // Event handled
            }
        }
    }
    
    // Then call the window's mouse button callback
    if (m_mouseButtonCallback) {
        m_mouseButtonCallback(button, action, x, y);
    }
}

void Window::notifyMouseMoveEvent(int x, int y) {
    // First notify controls
    for (auto& control : m_controls) {
        if (control->isVisible() && control->isEnabled()) {
            control->handleMouseMove(x, y);
        }
    }
    
    // Then call the window's mouse move callback
    if (m_mouseMoveCallback) {
        m_mouseMoveCallback(x, y);
    }
}

void Window::notifyResizeEvent(int width, int height) {
    // Update config
    m_config.width = width;
    m_config.height = height;
    
    // Call the window's resize callback
    if (m_resizeCallback) {
        m_resizeCallback(width, height);
    }
}

void Window::notifyCloseEvent() {
    // Call the window's close callback
    if (m_closeCallback) {
        m_closeCallback();
    }
}

//-----------------------------------------------------------------------------
// Canvas Implementation
//-----------------------------------------------------------------------------

Canvas::Canvas(int width, int height)
    : m_width(width)
    , m_height(height)
{
}

Canvas::~Canvas() {
}

void Canvas::setSize(int width, int height) {
    m_width = width;
    m_height = height;
}

unsigned int Canvas::rgb(unsigned char r, unsigned char g, unsigned char b) {
    return ((unsigned int)r << 16) | ((unsigned int)g << 8) | b;
}

unsigned int Canvas::rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return ((unsigned int)a << 24) | ((unsigned int)r << 16) | ((unsigned int)g << 8) | b;
}

//-----------------------------------------------------------------------------
// UIElement Implementation
//-----------------------------------------------------------------------------

UIElement::UIElement(int x, int y, int width, int height)
    : m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_visible(true)
{
}

UIElement::~UIElement() {
}

void UIElement::setPosition(int x, int y) {
    m_x = x;
    m_y = y;
}

void UIElement::getPosition(int& x, int& y) const {
    x = m_x;
    y = m_y;
}

void UIElement::setSize(int width, int height) {
    m_width = width;
    m_height = height;
}

void UIElement::getSize(int& width, int& height) const {
    width = m_width;
    height = m_height;
}

void UIElement::setVisible(bool visible) {
    m_visible = visible;
}

bool UIElement::isVisible() const {
    return m_visible;
}

bool UIElement::contains(int x, int y) const {
    return x >= m_x && x < m_x + m_width && y >= m_y && y < m_y + m_height;
}

//-----------------------------------------------------------------------------
// UIControl Implementation
//-----------------------------------------------------------------------------

UIControl::UIControl(int x, int y, int width, int height)
    : UIElement(x, y, width, height)
    , m_enabled(true)
    , m_focused(false)
    , m_hover(false)
{
}

UIControl::~UIControl() {
}

void UIControl::setEnabled(bool enabled) {
    m_enabled = enabled;
}

bool UIControl::isEnabled() const {
    return m_enabled;
}

bool UIControl::handleMouseMove(int x, int y) {
    bool wasHover = m_hover;
    m_hover = contains(x, y);
    
    // Return true if hover state changed
    return wasHover != m_hover;
}

bool UIControl::handleMouseButton(MouseButton button, MouseAction action, int x, int y) {
    // Focus the control if clicked
    if (button == MouseButton::Left && action == MouseAction::Press && contains(x, y)) {
        setFocus(true);
        return true;
    }
    
    return false;
}

bool UIControl::handleKeyInput(Key key, KeyAction action) {
    // Base implementation doesn't handle keys
    return false;
}

void UIControl::setFocus(bool focused) {
    m_focused = focused;
}

bool UIControl::hasFocus() const {
    return m_focused;
}

//-----------------------------------------------------------------------------
// Button Implementation
//-----------------------------------------------------------------------------

Button::Button(int x, int y, int width, int height, const std::string& text)
    : UIControl(x, y, width, height)
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

void Button::draw(Canvas* canvas) {
    if (!canvas || !m_visible) return;
    
    // Choose colors based on state
    unsigned int bgColor, textColor, borderColor;
    
    if (!m_enabled) {
        // Disabled state
        bgColor = Canvas::rgb(200, 200, 200);
        textColor = Canvas::rgb(128, 128, 128);
        borderColor = Canvas::rgb(180, 180, 180);
    } else if (m_hover) {
        // Hover state
        bgColor = Canvas::rgb(230, 230, 255);
        textColor = Canvas::rgb(0, 0, 0);
        borderColor = Canvas::rgb(100, 100, 255);
    } else {
        // Normal state
        bgColor = Canvas::rgb(240, 240, 240);
        textColor = Canvas::rgb(0, 0, 0);
        borderColor = Canvas::rgb(180, 180, 180);
    }
    
    // Draw button background
    canvas->drawRect(m_x, m_y, m_width, m_height, bgColor, true);
    
    // Draw button border
    canvas->drawRect(m_x, m_y, m_width, m_height, borderColor);
    
    // Draw button text
    int textX = m_x + (m_width - m_text.length() * 8) / 2; // Rough text centering
    int textY = m_y + (m_height - 12) / 2; // Assume 12px font height
    canvas->drawText(m_text, textX, textY, textColor);
}

bool Button::handleMouseButton(MouseButton button, MouseAction action, int x, int y) {
    // Call parent handler first
    UIControl::handleMouseButton(button, action, x, y);
    
    // Handle clicks
    if (m_enabled && button == MouseButton::Left && action == MouseAction::Press && contains(x, y)) {
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
    : UIControl(x, y, width, height)
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

void TextInput::draw(Canvas* canvas) {
    if (!canvas || !m_visible) return;
    
    // Choose colors based on state
    unsigned int bgColor, textColor, borderColor;
    
    if (!m_enabled) {
        // Disabled state
        bgColor = Canvas::rgb(220, 220, 220);
        textColor = Canvas::rgb(128, 128, 128);
        borderColor = Canvas::rgb(180, 180, 180);
    } else if (m_focused) {
        // Focused state
        bgColor = Canvas::rgb(255, 255, 255);
        textColor = Canvas::rgb(0, 0, 0);
        borderColor = Canvas::rgb(100, 100, 255);
    } else {
        // Normal state
        bgColor = Canvas::rgb(255, 255, 255);
        textColor = Canvas::rgb(0, 0, 0);
        borderColor = Canvas::rgb(180, 180, 180);
    }
    
    // Draw text field background
    canvas->drawRect(m_x, m_y, m_width, m_height, bgColor, true);
    
    // Draw text field border
    canvas->drawRect(m_x, m_y, m_width, m_height, borderColor);
    
    // Draw text or placeholder
    int textPadding = 5;
    if (m_text.empty() && !m_focused) {
        // Draw placeholder text
        canvas->drawText(m_placeholder, m_x + textPadding, m_y + (m_height - 12) / 2, Canvas::rgb(180, 180, 180));
    } else {
        // Draw actual text
        canvas->drawText(m_text, m_x + textPadding, m_y + (m_height - 12) / 2, textColor);
        
        // Draw cursor if focused
        if (m_focused) {
            // Simple cursor positioning - assumes monospace font
            int cursorX = m_x + textPadding + m_cursorPos * 8; // Assuming 8px per character
            canvas->drawLine(cursorX, m_y + 4, cursorX, m_y + m_height - 4, Canvas::rgb(0, 0, 0));
            
            // Draw selection if present
            if (m_cursorPos != m_selectionStart) {
                int selStart = std::min(m_cursorPos, m_selectionStart);
                int selEnd = std::max(m_cursorPos, m_selectionStart);
                int selX = m_x + textPadding + selStart * 8;
                int selWidth = (selEnd - selStart) * 8;
                
                // Draw selection background
                canvas->drawRect(selX, m_y + 2, selWidth, m_height - 4, Canvas::rgba(100, 100, 255, 128), true);
            }
        }
    }
}

bool TextInput::handleKeyInput(Key key, KeyAction action) {
    if (!m_enabled || !m_focused || action != KeyAction::Press) return false;
    
    switch (key) {
        case Key::Left:
            moveCursor(-1, (int)Key::Shift & 0x01); // Check if Shift is pressed
            return true;
            
        case Key::Right:
            moveCursor(1, (int)Key::Shift & 0x01); // Check if Shift is pressed
            return true;
            
        case Key::Home:
            m_cursorPos = 0;
            if (!((int)Key::Shift & 0x01)) { // If Shift is not pressed
                m_selectionStart = m_cursorPos;
            }
            return true;
            
        case Key::End:
            m_cursorPos = m_text.length();
            if (!((int)Key::Shift & 0x01)) { // If Shift is not pressed
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
            if (m_submitHandler) {
                m_submitHandler(m_text);
            }
            return true;
            
        default:
            // Handle printable characters
            if ((int)key >= 32 && (int)key <= 126) {
                char c = (char)key;
                insertText(std::string(1, c));
                return true;
            }
    }
    
    return false;
}

bool TextInput::handleMouseButton(MouseButton button, MouseAction action, int x, int y) {
    // Call parent handler first
    UIControl::handleMouseButton(button, action, x, y);
    
    // Handle mouse click for cursor positioning
    if (m_enabled && m_focused && button == MouseButton::Left) {
        if (action == MouseAction::Press) {
            // Calculate cursor position from click
            int textPadding = 5;
            int clickX = x - (m_x + textPadding);
            m_cursorPos = std::max(0, std::min((int)m_text.length(), clickX / 8)); // Assuming 8px per character
            
            // Start selection
            m_selectionStart = m_cursorPos;
            m_selecting = true;
            
            return true;
        } else if (action == MouseAction::Release) {
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

//-----------------------------------------------------------------------------
// Factory Functions
//-----------------------------------------------------------------------------

std::shared_ptr<Window> createPlatformWindow(const WindowConfig& config) {
#ifdef _WIN32
    return std::make_shared<Win32Window>(config);
#elif defined(__APPLE__)
    return std::make_shared<MacOSWindow>(config);
#else
    return std::make_shared<X11Window>(config);
#endif
}

std::shared_ptr<Canvas> createPlatformCanvas(int width, int height) {
#ifdef _WIN32
    return std::make_shared<Win32Canvas>(width, height);
#elif defined(__APPLE__)
    return std::make_shared<MacOSCanvas>(width, height);
#else
    return std::make_shared<X11Canvas>(width, height);
#endif
}

} // namespace ui
} // namespace browser