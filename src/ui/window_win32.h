// src/ui/window_win32.h - Windows platform implementation

#ifndef BROWSER_UI_WINDOW_WIN32_H
#define BROWSER_UI_WINDOW_WIN32_H

#include "window.h"
#include <windows.h>

namespace browser {
namespace ui {

// Windows-specific canvas implementation
class Win32Canvas : public Canvas {
public:
  HDC getGC() const;
  Win32Canvas(int width, int height);
  virtual ~Win32Canvas();

  // Initialize with DC
  void initialize(HDC hdc);

  // Canvas methods
  virtual void clear(unsigned int color) override;
  virtual void drawLine(int x1, int y1, int x2, int y2, unsigned int color, int thickness = 1) override;
  virtual void drawRect(int x, int y, int width, int height, unsigned int color, bool filled = false, int thickness = 1) override;
  virtual void drawEllipse(int x, int y, int width, int height, unsigned int color, bool filled = false, int thickness = 1) override;
  virtual void drawText(const std::string &text, int x, int y, unsigned int color, const std::string &fontName = "Arial", int fontSize = 12) override;
    
private:
    HDC m_hdc;            // Device context
    HBITMAP m_hBitmap;    // Bitmap for double buffering
    HDC m_hdcMem;         // Memory DC for double buffering
    
    // Helper methods
    HPEN createPen(unsigned int color, int thickness);
    HBRUSH createBrush(unsigned int color);
};

// Windows-specific window implementation
class Win32Window : public Window {
public:
    Win32Window(const WindowConfig& config);
    virtual ~Win32Window();
    
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
    
    // Static window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
private:
    HWND m_hwnd;                  // Window handle
    std::shared_ptr<Win32Canvas> m_canvas;  // Canvas for drawing
    HDC m_hdc;                    // Device context
    PAINTSTRUCT m_ps;             // Paint structure
    
    // Key mapping
    static Key mapVirtualKey(WPARAM wParam);
    
    // Mouse button mapping
    static MouseButton mapMouseButton(UINT uMsg);
    
    // Static window map
    static std::map<HWND, Win32Window*> s_windowMap;
};

} // namespace ui
} // namespace browser

#endif // BROWSER_UI_WINDOW_WIN32_H