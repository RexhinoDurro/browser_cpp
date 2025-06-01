// src/ui/window_win32.cpp - Windows platform implementation

#define NOMINMAX  // Prevent Windows.h from defining min/max macros
#include "window_win32.h"
#include <windowsx.h> // For GET_X_LPARAM, GET_Y_LPARAM
#include <iostream>
#include <vector>

namespace browser {
namespace ui {

// Static window map
std::map<HWND, Win32Window*> Win32Window::s_windowMap;

//-----------------------------------------------------------------------------
// Win32Canvas Implementation
//-----------------------------------------------------------------------------

Win32Canvas::Win32Canvas(int width, int height)
    : Canvas(width, height)
    , m_hdc(NULL)
    , m_hBitmap(NULL)
    , m_hdcMem(NULL)
{
}

Win32Canvas::~Win32Canvas() {
    // Clean up GDI resources
    if (m_hdcMem) {
        DeleteDC(m_hdcMem);
        m_hdcMem = NULL;
    }
    
    if (m_hBitmap) {
        DeleteObject(m_hBitmap);
        m_hBitmap = NULL;
    }
}

void Win32Canvas::initialize(HDC hdc) {
    m_hdc = hdc;
    
    // Create a compatible DC for double buffering
    if (m_hdcMem) {
        DeleteDC(m_hdcMem);
    }
    m_hdcMem = CreateCompatibleDC(m_hdc);
    
    // Create a compatible bitmap
    if (m_hBitmap) {
        DeleteObject(m_hBitmap);
    }
    m_hBitmap = CreateCompatibleBitmap(m_hdc, m_width, m_height);
    
    // Select the bitmap into the memory DC
    SelectObject(m_hdcMem, m_hBitmap);
}

HDC Win32Canvas::getGC() const {
    return m_hdcMem;
}

void Win32Canvas::clear(unsigned int color) {
    if (!m_hdcMem) return;
    
    // Extract color components
    COLORREF colorref = RGB(
        (color >> 16) & 0xFF,  // R
        (color >> 8) & 0xFF,   // G
        color & 0xFF           // B
    );
    
    // Create a brush with the specified color
    HBRUSH brush = CreateSolidBrush(colorref);
    
    // Fill the entire canvas
    RECT rect = { 0, 0, m_width, m_height };
    FillRect(m_hdcMem, &rect, brush);
    
    // Clean up
    DeleteObject(brush);
}

void Win32Canvas::drawLine(int x1, int y1, int x2, int y2, unsigned int color, int thickness) {
    if (!m_hdcMem) return;
    
    // Create a pen for the line
    HPEN pen = createPen(color, thickness);
    HPEN oldPen = (HPEN)SelectObject(m_hdcMem, pen);
    
    // Draw the line
    MoveToEx(m_hdcMem, x1, y1, NULL);
    LineTo(m_hdcMem, x2, y2);
    
    // Clean up
    SelectObject(m_hdcMem, oldPen);
    DeleteObject(pen);
}

void Win32Canvas::drawRect(int x, int y, int width, int height, unsigned int color, bool filled, int thickness) {
    if (!m_hdcMem) return;
    
    // Extract color components
    COLORREF colorref = RGB(
        (color >> 16) & 0xFF,  // R
        (color >> 8) & 0xFF,   // G
        color & 0xFF           // B
    );
    
    if (filled) {
        // Create a brush for filling
        HBRUSH brush = createBrush(color);
        HBRUSH oldBrush = (HBRUSH)SelectObject(m_hdcMem, brush);
        
        // Create a pen for the outline (null pen if we don't want an outline)
        HPEN pen = thickness > 0 ? createPen(color, thickness) : (HPEN)GetStockObject(NULL_PEN);
        HPEN oldPen = (HPEN)SelectObject(m_hdcMem, pen);
        
        // Draw the filled rectangle
        Rectangle(m_hdcMem, x, y, x + width, y + height);
        
        // Clean up
        SelectObject(m_hdcMem, oldBrush);
        SelectObject(m_hdcMem, oldPen);
        DeleteObject(brush);
        if (thickness > 0) {
            DeleteObject(pen);
        }
    } else {
        // Create a pen for the outline
        HPEN pen = createPen(color, thickness);
        HPEN oldPen = (HPEN)SelectObject(m_hdcMem, pen);
        
        // Use a null brush for transparent fill
        HBRUSH oldBrush = (HBRUSH)SelectObject(m_hdcMem, GetStockObject(NULL_BRUSH));
        
        // Draw the rectangle outline
        Rectangle(m_hdcMem, x, y, x + width, y + height);
        
        // Clean up
        SelectObject(m_hdcMem, oldPen);
        SelectObject(m_hdcMem, oldBrush);
        DeleteObject(pen);
    }
}

void Win32Canvas::drawEllipse(int x, int y, int width, int height, unsigned int color, bool filled, int thickness) {
    if (!m_hdcMem) return;
    
    if (filled) {
        // Create a brush for filling
        HBRUSH brush = createBrush(color);
        HBRUSH oldBrush = (HBRUSH)SelectObject(m_hdcMem, brush);
        
        // Create a pen for the outline (null pen if we don't want an outline)
        HPEN pen = thickness > 0 ? createPen(color, thickness) : (HPEN)GetStockObject(NULL_PEN);
        HPEN oldPen = (HPEN)SelectObject(m_hdcMem, pen);
        
        // Draw the filled ellipse
        Ellipse(m_hdcMem, x, y, x + width, y + height);
        
        // Clean up
        SelectObject(m_hdcMem, oldBrush);
        SelectObject(m_hdcMem, oldPen);
        DeleteObject(brush);
        if (thickness > 0) {
            DeleteObject(pen);
        }
    } else {
        // Create a pen for the outline
        HPEN pen = createPen(color, thickness);
        HPEN oldPen = (HPEN)SelectObject(m_hdcMem, pen);
        
        // Use a null brush for transparent fill
        HBRUSH oldBrush = (HBRUSH)SelectObject(m_hdcMem, GetStockObject(NULL_BRUSH));
        
        // Draw the ellipse outline
        Ellipse(m_hdcMem, x, y, x + width, y + height);
        
        // Clean up
        SelectObject(m_hdcMem, oldPen);
        SelectObject(m_hdcMem, oldBrush);
        DeleteObject(pen);
    }
}

void Win32Canvas::drawText(const std::string& text, int x, int y, unsigned int color, const std::string& fontName, int fontSize) {
    if (!m_hdcMem || text.empty()) return;
    
    // Extract color components
    COLORREF colorref = RGB(
        (color >> 16) & 0xFF,  // R
        (color >> 8) & 0xFF,   // G
        color & 0xFF           // B
    );
    
    // Convert fontName (UTF-8) to wide string
    int fontNameLen = MultiByteToWideChar(CP_UTF8, 0, fontName.c_str(), -1, NULL, 0);
    std::vector<wchar_t> wFontName(fontNameLen);
    MultiByteToWideChar(CP_UTF8, 0, fontName.c_str(), -1, wFontName.data(), fontNameLen);

    // Create font
    HFONT font = CreateFontW(
        fontSize,                // Height
        0,                      // Width (0 = match height)
        0,                      // Escapement
        0,                      // Orientation
        FW_NORMAL,              // Weight
        FALSE,                  // Italic
        FALSE,                  // Underline
        FALSE,                  // StrikeOut
        DEFAULT_CHARSET,        // CharSet
        OUT_DEFAULT_PRECIS,     // OutputPrecision
        CLIP_DEFAULT_PRECIS,    // ClipPrecision
        DEFAULT_QUALITY,        // Quality
        DEFAULT_PITCH | FF_DONTCARE, // PitchAndFamily
        wFontName.data()        // Font name (wide string)
    );
    
    HFONT oldFont = (HFONT)SelectObject(m_hdcMem, font);
    
    // Set text color
    SetTextColor(m_hdcMem, colorref);
    SetBkMode(m_hdcMem, TRANSPARENT);
    
    // Convert text (UTF-8) to wide string
    int textLen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
    std::vector<wchar_t> wText(textLen);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wText.data(), textLen);

    // Draw the text
    TextOutW(m_hdcMem, x, y, wText.data(), textLen - 1); // -1 to exclude null terminator
    
    // Clean up
    SelectObject(m_hdcMem, oldFont);
    DeleteObject(font);
}

HPEN Win32Canvas::createPen(unsigned int color, int thickness) {
    // Extract color components
    COLORREF colorref = RGB(
        (color >> 16) & 0xFF,  // R
        (color >> 8) & 0xFF,   // G
        color & 0xFF           // B
    );
    
    return CreatePen(PS_SOLID, thickness, colorref);
}

HBRUSH Win32Canvas::createBrush(unsigned int color) {
    // Extract color components
    COLORREF colorref = RGB(
        (color >> 16) & 0xFF,  // R
        (color >> 8) & 0xFF,   // G
        color & 0xFF           // B
    );
    
    return CreateSolidBrush(colorref);
}

//-----------------------------------------------------------------------------
// Win32Window Implementation
//-----------------------------------------------------------------------------

Win32Window::Win32Window(const WindowConfig& config)
    : Window(config)
    , m_hwnd(NULL)
    , m_hdc(NULL)
{
    m_canvas = std::make_shared<Win32Canvas>(config.width, config.height);
}

Win32Window::~Win32Window() {
    // Remove from window map
    if (m_hwnd) {
        s_windowMap.erase(m_hwnd);
    }
}

bool Win32Window::create() {
    // Register window class
    static bool classRegistered = false;
    static LPCWSTR className = L"BrowserWindowClass";
    
    if (!classRegistered) {
        WNDCLASSW wc = { 0 };
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = Win32Window::WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = className;
        
        if (!RegisterClassW(&wc)) {
            std::cerr << "Failed to register window class" << std::endl;
            return false;
        }
        
        classRegistered = true;
    }
    
    // Create window
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!m_config.resizable) {
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }
    
    RECT rect = { 0, 0, m_config.width, m_config.height };
    AdjustWindowRect(&rect, style, FALSE);
    
    // Convert string to wide string for CreateWindow
    int titleLength = MultiByteToWideChar(CP_UTF8, 0, m_config.title.c_str(), -1, NULL, 0);
    std::vector<wchar_t> wideTitle(titleLength);
    MultiByteToWideChar(CP_UTF8, 0, m_config.title.c_str(), -1, wideTitle.data(), titleLength);
    
    m_hwnd = CreateWindowW(
        className,
        wideTitle.data(),
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (!m_hwnd) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }
    
    // Add to window map
    s_windowMap[m_hwnd] = this;
    
    // Initialize canvas
    m_hdc = GetDC(m_hwnd);
    m_canvas->initialize(m_hdc);
    ReleaseDC(m_hwnd, m_hdc);
    
    // Maximize if requested
    if (m_config.maximized) {
        ShowWindow(m_hwnd, SW_MAXIMIZE);
    }
    
    return true;
}

void Win32Window::show() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
    }
}

void Win32Window::hide() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_HIDE);
    }
}

void Win32Window::close() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
}

bool Win32Window::processEvents() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        
        if (msg.message == WM_QUIT) {
            return false; // Quit
        }
    }
    
    return true; // Continue
}

void* Win32Window::getNativeHandle() const {
    return (void*)m_hwnd;
}

void Win32Window::setTitle(const std::string& title) {
    Window::setTitle(title);
    
    if (m_hwnd) {
        // Ensure proper conversion to wide string
        int len = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, NULL, 0);
        if (len > 0) {
            std::vector<wchar_t> wideTitle(len);
            MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, wideTitle.data(), len);
            SetWindowTextW(m_hwnd, wideTitle.data());
        }
    }
}

void Win32Window::setSize(int width, int height) {
    Window::setSize(width, height);
    
    if (m_hwnd) {
        RECT rect = { 0, 0, width, height };
        DWORD style = GetWindowLong(m_hwnd, GWL_STYLE);
        AdjustWindowRect(&rect, style, FALSE);
        
        SetWindowPos(m_hwnd, NULL, 0, 0, 
                     rect.right - rect.left, 
                     rect.bottom - rect.top,
                     SWP_NOMOVE | SWP_NOZORDER);
    }
}

void Win32Window::setPosition(int x, int y) {
    if (m_hwnd) {
        SetWindowPos(m_hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
}

void Win32Window::getPosition(int& x, int& y) const {
    if (m_hwnd) {
        RECT rect;
        GetWindowRect(m_hwnd, &rect);
        x = rect.left;
        y = rect.top;
    } else {
        x = 0;
        y = 0;
    }
}

void Win32Window::beginPaint() {
    // Don't use BeginPaint here - just get the DC
    if (m_hwnd && !m_hdc) {
        m_hdc = GetDC(m_hwnd);
        // Drawing will happen to the memory DC in the canvas
    }
}

void Win32Window::endPaint() {
    if (m_hwnd && m_hdc) {
        // Release the DC
        ReleaseDC(m_hwnd, m_hdc);
        m_hdc = NULL;
        
        // Force Windows to send a WM_PAINT message
        InvalidateRect(m_hwnd, NULL, FALSE);
    }
}

Canvas* Win32Window::getCanvas() {
    return m_canvas.get();
}

LRESULT CALLBACK Win32Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Get the window instance
    Win32Window* window = NULL;
    auto it = s_windowMap.find(hwnd);
    if (it != s_windowMap.end()) {
        window = it->second;
    }
    
    switch (uMsg) {
        case WM_CREATE:
            return 0;
            
        case WM_CLOSE:
            if (window) {
                window->notifyCloseEvent();
            }
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_SIZE:
            if (window) {
                int width = LOWORD(lParam);
                int height = HIWORD(lParam);
                window->notifyResizeEvent(width, height);
                window->m_canvas->setSize(width, height);
                
                // Re-initialize canvas with new size
                HDC hdc = GetDC(hwnd);
                window->m_canvas->initialize(hdc);
                ReleaseDC(hwnd, hdc);
                
                // Force a repaint
                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;
            
        case WM_PAINT: {
            if (window) {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                
                // Get the canvas
                Win32Canvas* winCanvas = static_cast<Win32Canvas*>(window->m_canvas.get());
                HDC hdcMem = winCanvas->getGC();
                
                if (hdcMem) {
                    // Copy the memory DC content to the window
                    BitBlt(hdc, 
                           ps.rcPaint.left, 
                           ps.rcPaint.top,
                           ps.rcPaint.right - ps.rcPaint.left,
                           ps.rcPaint.bottom - ps.rcPaint.top,
                           hdcMem,
                           ps.rcPaint.left,
                           ps.rcPaint.top,
                           SRCCOPY);
                } else {
                    // No memory DC, fill with background
                    HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
                    FillRect(hdc, &ps.rcPaint, brush);
                    DeleteObject(brush);
                }
                
                EndPaint(hwnd, &ps);
            }
            return 0;
        }
            
        case WM_ERASEBKGND:
            // Prevent flickering by not erasing the background
            return 1;
            
        case WM_KEYDOWN:
        case WM_KEYUP:
            if (window) {
                Key key = mapVirtualKey(wParam);
                KeyAction action = (uMsg == WM_KEYDOWN) ? KeyAction::Press : KeyAction::Release;
                window->notifyKeyEvent(key, action);
            }
            return 0;
            
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            if (window) {
                MouseButton button = mapMouseButton(uMsg);
                MouseAction action = 
                    (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_MBUTTONDOWN) 
                    ? MouseAction::Press : MouseAction::Release;
                
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);
                
                window->notifyMouseButtonEvent(button, action, x, y);
            }
            return 0;
            
        case WM_MOUSEMOVE:
            if (window) {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);
                
                window->notifyMouseMoveEvent(x, y);
            }
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

Key Win32Window::mapVirtualKey(WPARAM wParam) {
    switch (wParam) {
        case VK_BACK:       return Key::Backspace;
        case VK_TAB:        return Key::Tab;
        case VK_RETURN:     return Key::Enter;
        case VK_SHIFT:      return Key::Shift;
        case VK_CONTROL:    return Key::Control;
        case VK_MENU:       return Key::Alt;
        case VK_ESCAPE:     return Key::Escape;
        case VK_SPACE:      return Key::Space;
        case VK_PRIOR:      return Key::PageUp;
        case VK_NEXT:       return Key::PageDown;
        case VK_END:        return Key::End;
        case VK_HOME:       return Key::Home;
        case VK_LEFT:       return Key::Left;
        case VK_UP:         return Key::Up;
        case VK_RIGHT:      return Key::Right;
        case VK_DOWN:       return Key::Down;
        case VK_INSERT:     return Key::Insert;
        case VK_DELETE:     return Key::Delete;
        
        case VK_F1:         return Key::F1;
        case VK_F2:         return Key::F2;
        case VK_F3:         return Key::F3;
        case VK_F4:         return Key::F4;
        case VK_F5:         return Key::F5;
        case VK_F6:         return Key::F6;
        case VK_F7:         return Key::F7;
        case VK_F8:         return Key::F8;
        case VK_F9:         return Key::F9;
        case VK_F10:        return Key::F10;
        case VK_F11:        return Key::F11;
        case VK_F12:        return Key::F12;
        
        default:
            // For letter and number keys, convert to uppercase ASCII
            if (wParam >= 'A' && wParam <= 'Z') {
                return static_cast<Key>(wParam);
            } else if (wParam >= '0' && wParam <= '9') {
                return static_cast<Key>(wParam);
            }
            
            // Default for unknown keys
            return static_cast<Key>(0);
    }
}

MouseButton Win32Window::mapMouseButton(UINT uMsg) {
    switch (uMsg) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            return MouseButton::Left;
            
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            return MouseButton::Right;
            
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            return MouseButton::Middle;
            
        default:
            return MouseButton::Left; // Default
    }
}

} // namespace ui
} // namespace browser