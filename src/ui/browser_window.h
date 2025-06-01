// src/ui/browser_window.h
#ifndef BROWSER_UI_BROWSER_WINDOW_H
#define BROWSER_UI_BROWSER_WINDOW_H

#include "window.h"
#include "custom_controls.h"
#include "../browser/browser.h"
#include "../rendering/renderer.h"
#include "../rendering/custom_render_target.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace browser {
namespace ui {

// Browser window class - wraps everything together
class BrowserWindow {
public:
    BrowserWindow(const WindowConfig& config = WindowConfig());
    ~BrowserWindow();
    
    // Initialize the window
    bool initialize();
    
    // Window control methods
    void show();
    void hide();
    void close();
    bool isOpen() const;
    
    // Window properties
    void setSize(int width, int height);
    void getSize(int& width, int& height) const;
    
    void setPosition(int x, int y);
    void getPosition(int& x, int& y) const;
    
    void setTitle(const std::string& title);
    std::string getTitle() const;
    
    // Browser navigation
    bool loadUrl(const std::string& url);
    bool goBack();
    bool goForward();
    bool reload();
    void stopLoading();
    
    // Browser access
    void setBrowser(std::shared_ptr<browser::Browser> browser);
    std::shared_ptr<browser::Browser> getBrowser() const;
    
    // Renderer access
    void setRenderer(std::shared_ptr<rendering::Renderer> renderer) {
        m_renderer = renderer;
    }
    
    // Event handling
    void processEvents();
    void runEventLoop();
    
    // Navigation callbacks
    void setUrlChangeCallback(std::function<void(const std::string&)> callback);
    void setTitleChangeCallback(std::function<void(const std::string&)> callback);
    void setLoadingStateCallback(std::function<void(bool)> callback);
    
private:
    // Core components
    std::shared_ptr<Window> m_window;
    std::shared_ptr<browser::Browser> m_browser;
    std::shared_ptr<rendering::Renderer> m_renderer;
    std::shared_ptr<rendering::RenderTarget> m_renderTarget;
    std::shared_ptr<rendering::CustomRenderContext> m_customContext;
    std::shared_ptr<BrowserControls> m_browserControls;
    
    // Navigation state
    std::string m_currentUrl;
    std::vector<std::string> m_history;
    size_t m_historyIndex;
    bool m_isLoading;
    bool m_needsRender = false;
    
    // UI controls (removed individual controls, now managed by BrowserControls)
    
    // UI state
    bool m_initialized;
    
    // Callbacks
    std::function<void(const std::string&)> m_urlChangeCallback;
    std::function<void(const std::string&)> m_titleChangeCallback;
    std::function<void(bool)> m_loadingStateCallback;
    
    // Internal event handlers
    void handleKeyEvent(Key key, KeyAction action);
    void handleMouseEvent(MouseButton button, MouseAction action, int x, int y);
    void handleResizeEvent(int width, int height);
    void handleCloseEvent();
    
    // UI initialization and management
    void initializeControls();
    void updateNavigationButtons();
    void updateLoadingState(bool isLoading);
    
    // Page rendering
    void renderPage();
    
    // Page loading
    bool loadUrlInternal(const std::string& url);
    void showDefaultPage();
    void showErrorPage(const std::string& url, const std::string& error);
    
    // Helper methods
    html::Element* findElementAtPosition(int x, int y);

    friend class Win32Window;
    friend class MacOSWindow;
    friend class X11Window;
};

} // namespace ui
} // namespace browser

#endif // BROWSER_UI_BROWSER_WINDOW_H