//
// Complete macOS Window Implementation
//

// src/ui/window_macos.h
#ifndef BROWSER_UI_WINDOW_MACOS_H
#define BROWSER_UI_WINDOW_MACOS_H

#include "window.h"
#include <string>
#include <map>
#include <mutex>

// Forward declarations of Objective-C classes
#ifdef __OBJC__
@class BrowserWindowDelegate;
@class BrowserView;
@class NSWindow;
@class NSView;
@class NSMutableDictionary;
@class NSColor;
@class NSFont;
@class NSTextField;
@class NSTrackingArea;
#else
class BrowserWindowDelegate;
class BrowserView;
class NSWindow;
class NSView;
class NSMutableDictionary;
class NSColor;
class NSFont;
class NSTextField;
class NSTrackingArea;
#endif

namespace browser {
namespace ui {

// macOS-specific canvas implementation
class MacOSCanvas : public Canvas {
public:
    MacOSCanvas(int width, int height);
    virtual ~MacOSCanvas();
    
    // Initialize with NSView
    void initialize(NSView* view);
    
    // Canvas methods
    virtual void clear(unsigned int color) override;
    virtual void drawLine(int x1, int y1, int x2, int y2, unsigned int color, int thickness = 1) override;
    virtual void drawRect(int x, int y, int width, int height, unsigned int color, bool filled = false, int thickness = 1) override;
    virtual void drawEllipse(int x, int y, int width, int height, unsigned int color, bool filled = false, int thickness = 1) override;
    virtual void drawText(const std::string& text, int x, int y, unsigned int color, const std::string& fontName = "Arial", int fontSize = 12) override;
    
    // Get underlying NSView
    NSView* getView() const { return m_view; }
    
    // Force a redraw
    void setNeedsDisplay();
    
    // Drawing commands queue
    struct DrawCommand {
        enum Type {
            CLEAR,
            LINE,
            RECT,
            ELLIPSE,
            TEXT
        };
        
        Type type;
        int x1, y1, x2, y2;
        int width, height;
        unsigned int color;
        int thickness;
        bool filled;
        std::string text;
        std::string fontName;
        int fontSize;
        
        static DrawCommand Clear(unsigned int color) {
            DrawCommand cmd;
            cmd.type = CLEAR;
            cmd.color = color;
            return cmd;
        }
        
        static DrawCommand Line(int x1, int y1, int x2, int y2, unsigned int color, int thickness) {
            DrawCommand cmd;
            cmd.type = LINE;
            cmd.x1 = x1;
            cmd.y1 = y1;
            cmd.x2 = x2;
            cmd.y2 = y2;
            cmd.color = color;
            cmd.thickness = thickness;
            return cmd;
        }
        
        static DrawCommand Rect(int x, int y, int width, int height, unsigned int color, bool filled, int thickness) {
            DrawCommand cmd;
            cmd.type = RECT;
            cmd.x1 = x;
            cmd.y1 = y;
            cmd.width = width;
            cmd.height = height;
            cmd.color = color;
            cmd.filled = filled;
            cmd.thickness = thickness;
            return cmd;
        }
        
        static DrawCommand Ellipse(int x, int y, int width, int height, unsigned int color, bool filled, int thickness) {
            DrawCommand cmd;
            cmd.type = ELLIPSE;
            cmd.x1 = x;
            cmd.y1 = y;
            cmd.width = width;
            cmd.height = height;
            cmd.color = color;
            cmd.filled = filled;
            cmd.thickness = thickness;
            return cmd;
        }
        
        static DrawCommand Text(const std::string& text, int x, int y, unsigned int color, const std::string& fontName, int fontSize) {
            DrawCommand cmd;
            cmd.type = TEXT;
            cmd.text = text;
            cmd.x1 = x;
            cmd.y1 = y;
            cmd.color = color;
            cmd.fontName = fontName;
            cmd.fontSize = fontSize;
            return cmd;
        }
    };
    
    // Add a draw command to the queue
    void addDrawCommand(const DrawCommand& cmd);
    
    // Get all draw commands
    std::vector<DrawCommand> getDrawCommands();
    
    // Clear draw commands
    void clearDrawCommands();
    
private:
    NSView* m_view;
    std::vector<DrawCommand> m_drawCommands;
    std::mutex m_drawMutex;
    
    // Color and font caches
    std::map<unsigned int, NSColor*> m_colorCache;
    std::map<std::string, NSFont*> m_fontCache;
    
    // Helper methods
    NSColor* getColor(unsigned int color);
    NSFont* getFont(const std::string& fontName, int fontSize);
};

// macOS-specific window implementation
class MacOSWindow : public Window {
public:
    MacOSWindow(const WindowConfig& config);
    virtual ~MacOSWindow();
    
    // Window methods
    virtual bool create() override;
    virtual void show() override;
    virtual void hide() override;
    virtual void close() override;
    virtual bool processEvents() override;
    virtual void* getNativeHandle() const override;
    
    // Painting methods
    virtual void beginPaint() override;
    virtual void endPaint() override;
    virtual Canvas* getCanvas() override;
    
    // Window property methods
    virtual void setTitle(const std::string& title) override;
    virtual void setSize(int width, int height) override;
    virtual void setPosition(int x, int y) override;
    virtual void getPosition(int& x, int& y) const override;
    
    // Event handling methods
    void handleKeyEvent(int keyCode, bool pressed);
    void handleMouseButtonEvent(MouseButton button, MouseAction action, int x, int y);
    void handleMouseMoveEvent(int x, int y);
    void handleResizeEvent(int width, int height);
    void handleCloseEvent();
    
private:
    NSWindow* m_window;
    BrowserView* m_view;
    BrowserWindowDelegate* m_delegate;
    std::shared_ptr<MacOSCanvas> m_canvas;
    
    // Track if window is being resized
    bool m_resizing;
    
    // Key mapping
    Key mapKeyCode(int keyCode);
};

} // namespace ui
} // namespace browser

#endif // BROWSER_UI_WINDOW_MACOS_H










