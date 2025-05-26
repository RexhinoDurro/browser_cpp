// src/ui/window.h - Fixed window header with proper forward declarations

#ifndef BROWSER_UI_WINDOW_H
#define BROWSER_UI_WINDOW_H

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <map>

// Forward declarations to avoid circular dependencies
namespace browser {
    class Browser;
    namespace rendering {
        class Renderer;
        class RenderTarget;
    }
    namespace layout {
        class Box;
    }
}

namespace browser {
namespace ui {

// Forward declarations
class UIElement;
class UIControl;
class Canvas;
class Button;
class TextInput;

// Platform-specific window handle
#ifdef _WIN32
typedef void* NativeWindowHandle; // HWND on Windows
#elif defined(__APPLE__)
typedef void* NativeWindowHandle; // NSWindow* on macOS
#else
typedef unsigned long NativeWindowHandle; // Window on X11
#endif

// Window configuration
struct WindowConfig {
    std::string title = "Browser";
    int width = 1024;
    int height = 768;
    bool resizable = true;
    bool maximized = false;
};

// Mouse button definitions
enum class MouseButton {
    Left,
    Middle,
    Right
};

// Mouse action definitions
enum class MouseAction {
    Press,
    Release,
    Move
};

// Key definitions
enum class Key {
    // Special keys
    Backspace,
    Tab,
    Enter,
    Shift,
    Control,
    Alt,
    Escape,
    Space,
    PageUp,
    PageDown,
    End,
    Home,
    Left,
    Up,
    Right,
    Down,
    Insert,
    Delete,
    
    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    
    // Alphanumeric keys (ASCII values for simplicity)
    A = 'A', B = 'B', C = 'C', D = 'D', E = 'E', F = 'F', G = 'G', H = 'H', I = 'I',
    J = 'J', K = 'K', L = 'L', M = 'M', N = 'N', O = 'O', P = 'P', Q = 'Q', R = 'R',
    S = 'S', T = 'T', U = 'U', V = 'V', W = 'W', X = 'X', Y = 'Y', Z = 'Z',
    
    Num0 = '0', Num1 = '1', Num2 = '2', Num3 = '3', Num4 = '4',
    Num5 = '5', Num6 = '6', Num7 = '7', Num8 = '8', Num9 = '9'
};

// Key action definitions
enum class KeyAction {
    Press,
    Release,
    Repeat
};

// Base window class (platform-independent interface)
class Window {
public:
    Window(const WindowConfig& config);
    virtual ~Window();
    
    // Pure virtual methods to be implemented by platform-specific classes
    virtual bool create() = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void close() = 0;
    virtual bool processEvents() = 0;
    virtual void* getNativeHandle() const = 0;
    
    // Common window methods
    virtual void setTitle(const std::string& title);
    virtual std::string getTitle() const;
    
    virtual void setSize(int width, int height);
    virtual void getSize(int& width, int& height) const;
    
    virtual void setPosition(int x, int y);
    virtual void getPosition(int& x, int& y) const;
    
    // Event callbacks
    void setKeyCallback(std::function<void(Key, KeyAction)> callback);
    void setMouseButtonCallback(std::function<void(MouseButton, MouseAction, int, int)> callback);
    void setMouseMoveCallback(std::function<void(int, int)> callback);
    void setResizeCallback(std::function<void(int, int)> callback);
    void setCloseCallback(std::function<void()> callback);
    
    // UI methods
    void addControl(std::shared_ptr<UIControl> control);
    void removeControl(std::shared_ptr<UIControl> control);
    
    // Rendering
    virtual void beginPaint() = 0;
    virtual void endPaint() = 0;
    virtual Canvas* getCanvas() = 0;
    
protected:
    WindowConfig m_config;
    std::vector<std::shared_ptr<UIControl>> m_controls;
    
    // Callback handlers
    std::function<void(Key, KeyAction)> m_keyCallback;
    std::function<void(MouseButton, MouseAction, int, int)> m_mouseButtonCallback;
    std::function<void(int, int)> m_mouseMoveCallback;
    std::function<void(int, int)> m_resizeCallback;
    std::function<void()> m_closeCallback;
    
    // Protected methods for subclasses to invoke callbacks
    void notifyKeyEvent(Key key, KeyAction action);
    void notifyMouseButtonEvent(MouseButton button, MouseAction action, int x, int y);
    void notifyMouseMoveEvent(int x, int y);
    void notifyResizeEvent(int width, int height);
    void notifyCloseEvent();
};

// Factory function to create platform-specific window implementation
std::shared_ptr<Window> createPlatformWindow(const WindowConfig& config);

// Canvas class for drawing (platform-independent interface)
class Canvas {
public:
    Canvas(int width, int height);
    virtual ~Canvas();
    
    // Pure virtual methods to be implemented by platform-specific classes
    virtual void clear(unsigned int color) = 0;
    virtual void drawLine(int x1, int y1, int x2, int y2, unsigned int color, int thickness = 1) = 0;
    virtual void drawRect(int x, int y, int width, int height, unsigned int color, bool filled = false, int thickness = 1) = 0;
    virtual void drawEllipse(int x, int y, int width, int height, unsigned int color, bool filled = false, int thickness = 1) = 0;
    virtual void drawText(const std::string& text, int x, int y, unsigned int color, const std::string& fontName = "Arial", int fontSize = 12) = 0;
    
    // Helper methods for common operations
    void setSize(int width, int height);
    int width() const { return m_width; }
    int height() const { return m_height; }
    
    // Color helper methods
    static unsigned int rgb(unsigned char r, unsigned char g, unsigned char b);
    static unsigned int rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
    
protected:
    int m_width;
    int m_height;
};

// Factory function to create platform-specific canvas implementation
std::shared_ptr<Canvas> createPlatformCanvas(int width, int height);

// UI element base class
class UIElement {
public:
    UIElement(int x, int y, int width, int height);
    virtual ~UIElement();
    
    // Position and size
    void setPosition(int x, int y);
    void getPosition(int& x, int& y) const;
    
    void setSize(int width, int height);
    void getSize(int& width, int& height) const;
    
    // Visibility
    void setVisible(bool visible);
    bool isVisible() const;
    
    // Drawing
    virtual void draw(Canvas* canvas) = 0;
    
    // Hit testing
    bool contains(int x, int y) const;
    
protected:
    int m_x, m_y;
    int m_width, m_height;
    bool m_visible;
};

// UI control base class (interactive UI element)
class UIControl : public UIElement {
public:
    UIControl(int x, int y, int width, int height);
    virtual ~UIControl();
    
    // Enabled state
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    // Input handling
    virtual bool handleMouseMove(int x, int y);
    virtual bool handleMouseButton(MouseButton button, MouseAction action, int x, int y);
    virtual bool handleKeyInput(Key key, KeyAction action);
    
    // Focus
    void setFocus(bool focused);
    bool hasFocus() const;
    
protected:
    bool m_enabled;
    bool m_focused;
    bool m_hover;
};

// Forward declarations for UI controls
// These are defined in custom_controls.h
class Button;
class TextInput;

} // namespace ui
} // namespace browser

#endif // BROWSER_UI_WINDOW_H