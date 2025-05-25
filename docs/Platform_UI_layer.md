# Platform and UI Layer Documentation

## Overview

The platform and UI layer provides cross-platform window management, native controls integration, and custom rendering capabilities. It abstracts platform-specific APIs while providing a consistent interface for the browser engine.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Browser Window                           │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                  UI Controls                         │   │
│  │  ┌──────┐ ┌──────┐ ┌───────────┐ ┌─────────────┐ │   │
│  │  │ Back │ │Forward│ │  Address  │ │   Progress  │ │   │
│  │  │ Button│ │Button │ │    Bar    │ │     Bar     │ │   │
│  │  └──────┘ └──────┘ └───────────┘ └─────────────┘ │   │
│  └─────────────────────────────────────────────────────┘   │
│                           │                                 │
│  ┌────────────────────────┴─────────────────────────────┐  │
│  │              Platform Window Layer                    │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────────────┐  │  │
│  │  │  Win32   │  │  macOS   │  │      X11         │  │  │
│  │  │ Window   │  │  Window  │  │    Window        │  │  │
│  │  └──────────┘  └──────────┘  └──────────────────┘  │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Platform Window Abstraction

### Base Window Interface

```cpp
class Window {
public:
    // Window lifecycle
    virtual bool create() = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void close() = 0;
    virtual bool processEvents() = 0;
    
    // Window properties
    virtual void setTitle(const std::string& title);
    virtual void setSize(int width, int height);
    virtual void setPosition(int x, int y);
    
    // Event callbacks
    void setKeyCallback(std::function<void(Key, KeyAction)> callback);
    void setMouseButtonCallback(std::function<void(MouseButton, MouseAction, int, int)> callback);
    void setResizeCallback(std::function<void(int, int)> callback);
    
    // Rendering
    virtual Canvas* getCanvas() = 0;
};
```

### Window Configuration

```cpp
struct WindowConfig {
    std::string title = "Browser";
    int width = 1024;
    int height = 768;
    bool resizable = true;
    bool maximized = false;
};
```

## Platform Implementations

### Windows (Win32)

The Win32 implementation uses native Windows APIs:

```cpp
class Win32Window : public Window {
    HWND m_hwnd;                    // Window handle
    std::shared_ptr<Win32Canvas> m_canvas;
    
    // Window procedure for message handling
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, 
                                     WPARAM wParam, LPARAM lParam);
};
```

Key features:
- Uses `CreateWindow` for window creation
- GDI/GDI+ for drawing operations
- Message pump for event handling
- Double buffering for flicker-free rendering

### macOS (Cocoa)

The macOS implementation uses Objective-C and Cocoa:

```objc
@interface BrowserView : NSView
- (void)drawRect:(NSRect)dirtyRect;
- (void)mouseDown:(NSEvent*)event;
- (void)keyDown:(NSEvent*)event;
@end
```

```cpp
class MacOSWindow : public Window {
    NSWindow* m_window;
    BrowserView* m_view;
    BrowserWindowDelegate* m_delegate;
};
```

Key features:
- NSWindow/NSView for window management
- Core Graphics for drawing
- Responder chain for event handling
- Retina display support

### Linux (X11)

The X11 implementation for Linux/Unix systems:

```cpp
class X11Window : public Window {
    Display* m_display;
    Window m_window;
    GC m_gc;
    XImage* m_backBuffer;
};
```

Key features:
- Xlib for window management
- Cairo or raw X11 for drawing
- Event loop integration
- Window manager hints support

## Canvas Abstraction

### Base Canvas Interface

```cpp
class Canvas {
public:
    // Basic drawing operations
    virtual void clear(unsigned int color) = 0;
    virtual void drawLine(int x1, int y1, int x2, int y2, 
                         unsigned int color, int thickness = 1) = 0;
    virtual void drawRect(int x, int y, int width, int height, 
                         unsigned int color, bool filled = false, 
                         int thickness = 1) = 0;
    virtual void drawText(const std::string& text, int x, int y, 
                         unsigned int color, 
                         const std::string& fontName = "Arial", 
                         int fontSize = 12) = 0;
    
    // Color helpers
    static unsigned int rgb(unsigned char r, unsigned char g, unsigned char b);
    static unsigned int rgba(unsigned char r, unsigned char g, unsigned char b, 
                            unsigned char a);
};
```

### Platform Canvas Implementations

#### Win32Canvas

```cpp
class Win32Canvas : public Canvas {
    HDC m_hdc;         // Device context
    HBITMAP m_hBitmap; // Bitmap for double buffering
    HDC m_hdcMem;      // Memory DC
    
    // GDI object creation
    HPEN createPen(unsigned int color, int thickness);
    HBRUSH createBrush(unsigned int color);
};
```

#### MacOSCanvas

```cpp
class MacOSCanvas : public Canvas {
    NSView* m_view;
    std::vector<DrawCommand> m_drawCommands;
    
    // Deferred rendering with command queue
    void addDrawCommand(const DrawCommand& cmd);
};
```

## UI Controls

### Control Hierarchy

```cpp
// Base UI element
class UIElement {
protected:
    int m_x, m_y, m_width, m_height;
    bool m_visible;
    
public:
    virtual void draw(Canvas* canvas) = 0;
    bool contains(int x, int y) const;
};

// Interactive control base
class UIControl : public UIElement {
protected:
    bool m_enabled;
    bool m_focused;
    bool m_hover;
    
public:
    virtual bool handleMouseMove(int x, int y);
    virtual bool handleMouseButton(MouseButton button, 
                                  MouseAction action, int x, int y);
    virtual bool handleKeyInput(Key key, KeyAction action);
};
```

### Button Control

```cpp
class Button : public UIControl {
    std::string m_text;
    std::function<void()> m_clickHandler;
    
public:
    void setText(const std::string& text);
    void setClickHandler(std::function<void()> handler);
    
    virtual void draw(Canvas* canvas) override {
        // Draw button background
        unsigned int bgColor = m_hover ? 
            Canvas::rgb(230, 230, 255) : 
            Canvas::rgb(240, 240, 240);
        canvas->drawRect(m_x, m_y, m_width, m_height, bgColor, true);
        
        // Draw button border
        canvas->drawRect(m_x, m_y, m_width, m_height, 
                        Canvas::rgb(180, 180, 180));
        
        // Draw button text (centered)
        int textX = m_x + (m_width - m_text.length() * 8) / 2;
        int textY = m_y + (m_height - 12) / 2;
        canvas->drawText(m_text, textX, textY, Canvas::rgb(0, 0, 0));
    }
};
```

### Text Input Control

```cpp
class TextInput : public UIControl {
    std::string m_text;
    std::string m_placeholder;
    size_t m_cursorPos;
    size_t m_selectionStart;
    
public:
    void setText(const std::string& text);
    void setPlaceholder(const std::string& placeholder);
    
    // Text editing operations
    void insertText(const std::string& text);
    void deletePreviousChar();
    void moveCursor(int direction, bool selecting);
    
    // Event handlers
    void setSubmitHandler(std::function<void(const std::string&)> handler);
    void setTextChangeHandler(std::function<void(const std::string&)> handler);
};
```

### Progress Bar

```cpp
class ProgressBar : public UIControl {
    float m_value;           // 0.0 to 1.0
    bool m_indeterminate;    // Indeterminate mode
    float m_animationOffset; // For animation
    
public:
    void setValue(float value);
    void setIndeterminate(bool indeterminate);
    
    virtual void draw(Canvas* canvas) override {
        // Draw background
        canvas->drawRect(m_x, m_y, m_width, m_height, 
                        Canvas::rgb(220, 220, 220), true);
        
        if (m_indeterminate) {
            // Animated bar
            float barWidth = m_width * 0.3f;
            float x = m_x + m_animationOffset * (m_width - barWidth);
            canvas->drawRect(x, m_y + 1, barWidth, m_height - 2,
                           Canvas::rgb(100, 100, 255), true);
        } else {
            // Progress fill
            float progressWidth = m_width * m_value;
            canvas->drawRect(m_x + 1, m_y + 1, 
                           progressWidth - 2, m_height - 2,
                           Canvas::rgb(100, 100, 255), true);
        }
    }
};
```

## Custom Renderer Integration

### Custom Controls

Using the custom renderer for advanced UI:

```cpp
class CustomControls {
    rendering::CustomRenderContext* m_renderContext;
    
    void drawButton(Button* button) {
        auto ctx = m_renderContext;
        
        // Rounded rectangle background
        ctx->beginPath();
        ctx->roundedRect(button->x(), button->y(), 
                        button->width(), button->height(), 3.0f);
        
        // Gradient fill
        Paint gradient = Paint::linearGradient(
            x, y, x, y + height,
            Color(255, 255, 255),
            Color(230, 230, 230)
        );
        ctx->setFillPaint(gradient);
        ctx->fill();
        
        // Shadow effect
        ctx->setShadowColor(Color(0, 0, 0, 0.2f));
        ctx->setShadowOffset(0, 2);
        ctx->setShadowBlur(4);
    }
};
```

## Browser Window

### Main Browser Window

```cpp
class BrowserWindow {
    // Platform window
    std::shared_ptr<Window> m_window;
    
    // Browser engine
    std::shared_ptr<Browser> m_browser;
    
    // UI components
    std::shared_ptr<Toolbar> m_toolbar;
    std::shared_ptr<Button> m_backButton;
    std::shared_ptr<Button> m_forwardButton;
    std::shared_ptr<Button> m_reloadButton;
    std::shared_ptr<TextInput> m_addressBar;
    std::shared_ptr<ProgressBar> m_progressBar;
    
    // Navigation state
    std::vector<std::string> m_history;
    size_t m_historyIndex;
    
public:
    // Window lifecycle
    bool initialize();
    void show();
    void runEventLoop();
    
    // Browser navigation
    bool loadUrl(const std::string& url);
    bool goBack();
    bool goForward();
    bool reload();
};
```

### Event Handling

```cpp
void BrowserWindow::handleKeyEvent(Key key, KeyAction action) {
    if (action == KeyAction::Press) {
        // Browser shortcuts
        if (isCtrlPressed()) {
            switch (key) {
                case Key::L:
                    // Focus address bar
                    m_addressBar->setFocus(true);
                    break;
                case Key::R:
                    // Reload page
                    reload();
                    break;
                case Key::T:
                    // New tab (if supported)
                    createNewTab();
                    break;
                case Key::W:
                    // Close window/tab
                    close();
                    break;
            }
        } else {
            switch (key) {
                case Key::F5:
                    reload();
                    break;
                case Key::Escape:
                    if (m_isLoading) {
                        stopLoading();
                    }
                    break;
            }
        }
    }
}

void BrowserWindow::handleMouseEvent(MouseButton button, MouseAction action, int x, int y) {
    // Check if click is in content area
    int toolbarHeight = 40;
    if (y > toolbarHeight && m_browser) {
        int contentX = x;
        int contentY = y - toolbarHeight;
        
        if (button == MouseButton::Left && action == MouseAction::Press) {
            // Find element under cursor
            html::Element* element = findElementAtPosition(contentX, contentY);
            
            if (element && element->tagName() == "a") {
                std::string href = element->getAttribute("href");
                if (!href.empty()) {
                    loadUrl(resolveUrl(href, m_currentUrl));
                }
            }
        }
    }
}
```

### Navigation Management

```cpp
bool BrowserWindow::loadUrl(const std::string& input) {
    std::string url = normalizeUrl(input);
    
    // Security check
    Origin targetOrigin(url);
    if (!m_browser->securityManager()->canNavigate(m_currentOrigin, targetOrigin)) {
        showErrorPage("Navigation blocked by security policy");
        return false;
    }
    
    // Update UI state
    m_isLoading = true;
    m_progressBar->setIndeterminate(true);
    m_progressBar->setVisible(true);
    m_stopButton->setVisible(true);
    m_reloadButton->setVisible(false);
    
    // Load URL
    std::string error;
    bool success = m_browser->loadUrl(url, error);
    
    if (success) {
        // Update history
        if (m_historyIndex < m_history.size() - 1) {
            m_history.resize(m_historyIndex + 1);
        }
        m_history.push_back(url);
        m_historyIndex = m_history.size() - 1;
        
        // Update UI
        m_addressBar->setText(url);
        updateNavigationButtons();
        
        // Update title
        if (m_titleChangeCallback) {
            std::string title = m_browser->currentDocument()->title();
            m_titleChangeCallback(title);
        }
    } else {
        showErrorPage(error);
    }
    
    // Update loading state
    m_isLoading = false;
    m_progressBar->setVisible(false);
    m_stopButton->setVisible(false);
    m_reloadButton->setVisible(true);
    
    return success;
}

void BrowserWindow::updateNavigationButtons() {
    m_backButton->setEnabled(m_historyIndex > 0);
    m_forwardButton->setEnabled(m_historyIndex < m_history.size() - 1);
}
```

## Event Loop Integration

### Main Event Loop

```cpp
void BrowserWindow::runEventLoop() {
    const int TARGET_FPS = 60;
    const auto FRAME_TIME = std::chrono::milliseconds(1000 / TARGET_FPS);
    
    auto lastFrameTime = std::chrono::steady_clock::now();
    
    while (isOpen()) {
        auto frameStart = std::chrono::steady_clock::now();
        
        // Process platform events
        if (!m_window->processEvents()) {
            break; // Window closed
        }
        
        // Process network events
        m_browser->resourceLoader()->processPendingRequests();
        
        // Update animations
        float deltaTime = std::chrono::duration<float>(
            frameStart - lastFrameTime).count();
        updateAnimations(deltaTime);
        
        // Render if needed
        if (m_needsRender || hasAnimations()) {
            render();
            m_needsRender = false;
        }
        
        // Frame rate limiting
        auto frameEnd = std::chrono::steady_clock::now();
        auto frameDuration = frameEnd - frameStart;
        
        if (frameDuration < FRAME_TIME) {
            std::this_thread::sleep_for(FRAME_TIME - frameDuration);
        }
        
        lastFrameTime = frameStart;
    }
}
```

### Animation Support

```cpp
class AnimationController {
    struct Animation {
        std::function<bool(float)> update; // Returns true when complete
        float duration;
        float elapsed;
    };
    
    std::vector<Animation> m_animations;
    
public:
    void addAnimation(std::function<bool(float)> update, float duration) {
        m_animations.push_back({update, duration, 0.0f});
    }
    
    void update(float deltaTime) {
        for (auto it = m_animations.begin(); it != m_animations.end();) {
            it->elapsed += deltaTime;
            float progress = std::min(it->elapsed / it->duration, 1.0f);
            
            if (it->update(progress) || progress >= 1.0f) {
                it = m_animations.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    bool hasAnimations() const {
        return !m_animations.empty();
    }
};
```

## Rendering Pipeline

### Page Rendering

```cpp
void BrowserWindow::render() {
    // Begin paint
    m_window->beginPaint();
    Canvas* canvas = m_window->getCanvas();
    
    // Clear background
    canvas->clear(Canvas::rgb(255, 255, 255));
    
    // Draw toolbar
    drawToolbar(canvas);
    
    // Draw page content
    if (m_browser && m_browser->layoutRoot()) {
        // Create render target for content area
        int toolbarHeight = 40;
        int contentHeight = m_window->height() - toolbarHeight;
        
        // Clip to content area
        canvas->setClipRect(0, toolbarHeight, m_window->width(), contentHeight);
        
        // Render page
        m_browser->render(getRenderTarget());
        
        // Reset clip
        canvas->resetClip();
    }
    
    // End paint
    m_window->endPaint();
}

void BrowserWindow::drawToolbar(Canvas* canvas) {
    // Draw toolbar background
    canvas->drawRect(0, 0, m_window->width(), 40, 
                    Canvas::rgb(240, 240, 240), true);
    
    // Draw separator line
    canvas->drawLine(0, 40, m_window->width(), 40, 
                    Canvas::rgb(200, 200, 200));
    
    // Draw controls
    for (auto& control : m_toolbar->controls()) {
        if (control->isVisible()) {
            control->draw(canvas);
        }
    }
}
```

## Input Handling

### Mouse Input Processing

```cpp
class InputManager {
    struct MouseState {
        int x, y;
        bool buttons[3]; // Left, Middle, Right
        UIControl* hoveredControl;
        UIControl* capturedControl;
    } m_mouse;
    
public:
    void handleMouseMove(int x, int y) {
        m_mouse.x = x;
        m_mouse.y = y;
        
        // Update hover states
        UIControl* newHover = findControlAt(x, y);
        
        if (newHover != m_mouse.hoveredControl) {
            if (m_mouse.hoveredControl) {
                m_mouse.hoveredControl->onMouseLeave();
            }
            if (newHover) {
                newHover->onMouseEnter();
            }
            m_mouse.hoveredControl = newHover;
        }
        
        // Send move event
        if (m_mouse.capturedControl) {
            m_mouse.capturedControl->handleMouseMove(x, y);
        } else if (newHover) {
            newHover->handleMouseMove(x, y);
        }
    }
    
    void handleMouseButton(MouseButton button, MouseAction action, int x, int y) {
        int buttonIndex = static_cast<int>(button);
        
        if (action == MouseAction::Press) {
            m_mouse.buttons[buttonIndex] = true;
            
            // Capture control
            if (!m_mouse.capturedControl) {
                m_mouse.capturedControl = findControlAt(x, y);
            }
            
            // Send event
            if (m_mouse.capturedControl) {
                m_mouse.capturedControl->handleMouseButton(button, action, x, y);
            }
        } else {
            m_mouse.buttons[buttonIndex] = false;
            
            // Send event
            if (m_mouse.capturedControl) {
                m_mouse.capturedControl->handleMouseButton(button, action, x, y);
                
                // Release capture
                if (!m_mouse.buttons[0] && !m_mouse.buttons[1] && !m_mouse.buttons[2]) {
                    m_mouse.capturedControl = nullptr;
                }
            }
        }
    }
};
```

### Keyboard Focus Management

```cpp
class FocusManager {
    UIControl* m_focusedControl;
    std::vector<UIControl*> m_tabOrder;
    
public:
    void setFocus(UIControl* control) {
        if (m_focusedControl != control) {
            if (m_focusedControl) {
                m_focusedControl->onFocusLost();
            }
            
            m_focusedControl = control;
            
            if (m_focusedControl) {
                m_focusedControl->onFocusGained();
            }
        }
    }
    
    void handleTab(bool shiftPressed) {
        if (m_tabOrder.empty()) return;
        
        // Find current focus in tab order
        auto it = std::find(m_tabOrder.begin(), m_tabOrder.end(), 
                           m_focusedControl);
        
        if (shiftPressed) {
            // Previous control
            if (it == m_tabOrder.begin()) {
                setFocus(m_tabOrder.back());
            } else {
                setFocus(*(--it));
            }
        } else {
            // Next control
            if (it == m_tabOrder.end() || ++it == m_tabOrder.end()) {
                setFocus(m_tabOrder.front());
            } else {
                setFocus(*it);
            }
        }
    }
};
```

## Platform-Specific Features

### Windows Features

```cpp
// DPI awareness
void Win32Window::enableDPIAwareness() {
    SetProcessDPIAware();
    
    // Get DPI for scaling
    HDC hdc = GetDC(m_hwnd);
    m_dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    m_dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(m_hwnd, hdc);
}

// Dark mode support
void Win32Window::setDarkMode(bool enabled) {
    BOOL useDarkMode = enabled ? TRUE : FALSE;
    DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, 
                         &useDarkMode, sizeof(useDarkMode));
}
```

### macOS Features

```cpp
// Retina display support
void MacOSWindow::enableRetinaSupport() {
    [m_view setWantsBestResolutionOpenGLSurface:YES];
    
    // Get backing scale factor
    CGFloat scale = [[m_window screen] backingScaleFactor];
    m_canvas->setScale(scale);
}

// Full screen support
void MacOSWindow::toggleFullScreen() {
    [m_window toggleFullScreen:nil];
}

// Touch bar support (if available)
void MacOSWindow::setupTouchBar() {
    if (@available(macOS 10.12.2, *)) {
        // Create touch bar with browser controls
    }
}
```

### Linux Features

```cpp
// Window manager hints
void X11Window::setWindowType(WindowType type) {
    Atom windowType = None;
    
    switch (type) {
        case WindowType::Normal:
            windowType = XInternAtom(m_display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
            break;
        case WindowType::Dialog:
            windowType = XInternAtom(m_display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
            break;
    }
    
    XChangeProperty(m_display, m_window,
                   XInternAtom(m_display, "_NET_WM_WINDOW_TYPE", False),
                   XA_ATOM, 32, PropModeReplace,
                   (unsigned char*)&windowType, 1);
}

// Desktop integration
void X11Window::setDesktopFile(const std::string& desktopFile) {
    // Set application ID for desktop integration
    XChangeProperty(m_display, m_window,
                   XInternAtom(m_display, "_GTK_APPLICATION_ID", False),
                   XA_STRING, 8, PropModeReplace,
                   (unsigned char*)desktopFile.c_str(), 
                   desktopFile.length());
}
```

## Accessibility

### Screen Reader Support

```cpp
class AccessibilityManager {
public:
    void announceNavigation(const std::string& url) {
        announce("Navigating to " + url);
    }
    
    void announcePageLoaded(const std::string& title) {
        announce("Page loaded: " + title);
    }
    
private:
    void announce(const std::string& text) {
        #ifdef _WIN32
        // Use Windows Narrator API
        ISpVoice* voice;
        CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, 
                        IID_ISpVoice, (void**)&voice);
        voice->Speak(toWideString(text).c_str(), 0, NULL);
        voice->Release();
        #elif defined(__APPLE__)
        // Use macOS VoiceOver
        NSString* announcement = [NSString stringWithUTF8String:text.c_str()];
        NSAccessibilityPostNotificationWithUserInfo(
            NSApp, NSAccessibilityAnnouncementRequestedNotification,
            @{NSAccessibilityAnnouncementKey: announcement}
        );
        #endif
    }
};
```

## Performance Optimizations

### Dirty Region Tracking

```cpp
class DirtyRegionTracker {
    std::vector<Rect> m_dirtyRegions;
    
public:
    void markDirty(const Rect& region) {
        // Merge overlapping regions
        for (auto& existing : m_dirtyRegions) {
            if (existing.intersects(region)) {
                existing = existing.union(region);
                return;
            }
        }
        m_dirtyRegions.push_back(region);
    }
    
    void paint(Canvas* canvas) {
        for (const auto& region : m_dirtyRegions) {
            canvas->setClipRect(region);
            paintRegion(canvas, region);
        }
        m_dirtyRegions.clear();
    }
};
```

### Hardware Acceleration

```cpp
// Direct2D acceleration on Windows
class D2DCanvas : public Win32Canvas {
    ID2D1RenderTarget* m_renderTarget;
    ID2D1Factory* m_factory;
    
public:
    void drawLine(int x1, int y1, int x2, int y2, 
                 unsigned int color, int thickness) override {
        ID2D1SolidColorBrush* brush;
        m_renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(color), &brush);
        
        m_renderTarget->DrawLine(
            D2D1::Point2F(x1, y1),
            D2D1::Point2F(x2, y2),
            brush, thickness);
        
        brush->Release();
    }
};
```

## Testing

### UI Testing Framework

```cpp
class UITest {
protected:
    std::shared_ptr<BrowserWindow> m_window;
    
    void clickButton(const std::string& text) {
        Button* button = findButtonByText(text);
        ASSERT_NE(button, nullptr);
        
        simulateClick(button->x() + button->width() / 2,
                     button->y() + button->height() / 2);
    }
    
    void typeInAddressBar(const std::string& text) {
        TextInput* addressBar = findControl<TextInput>("addressBar");
        ASSERT_NE(addressBar, nullptr);
        
        addressBar->setFocus(true);
        for (char c : text) {
            simulateKeyPress(static_cast<Key>(c));
        }
    }
};

TEST_F(UITest, Navigation) {
    // Type URL
    typeInAddressBar("https://example.com");
    simulateKeyPress(Key::Enter);
    
    // Wait for load
    waitForPageLoad();
    
    // Verify
    EXPECT_EQ(m_window->currentUrl(), "https://example.com");
    
    // Test back button
    clickButton("Back");
    EXPECT_FALSE(m_window->canGoBack());
}
```

## Future Enhancements

1. **Multi-tab Support**: Tab management and switching
2. **GPU Acceleration**: OpenGL/Vulkan/Metal rendering
3. **Touch Support**: Gesture recognition
4. **High DPI**: Better scaling support
5. **Themes**: Customizable UI themes
6. **Extensions**: UI extension points
7. **Developer Tools**: Integrated debugging UI