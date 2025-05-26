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