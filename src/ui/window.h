#ifndef BROWSER_UI_WINDOW_H
#define BROWSER_UI_WINDOW_H

#include <string>
#include <memory>
#include <functional>
#include "../browser/browser.h"
#include "../rendering/renderer.h"

// Forward declarations for UI toolkit (in this case, we'll use a cross-platform approach)
struct GLFWwindow;

namespace browser {
namespace ui {

// Window configuration
struct WindowConfig {
    std::string title = "Browser";
    int width = 1024;
    int height = 768;
    bool resizable = true;
    bool maximized = false;
};

// Browser window class
class BrowserWindow {
public:
    BrowserWindow(const WindowConfig& config = WindowConfig());
    ~BrowserWindow();
    
    // Initialize the window
    bool initialize();
    
    // Show the window
    void show();
    
    // Hide the window
    void hide();
    
    // Close the window
    void close();
    
    // Check if window is open
    bool isOpen() const;
    
    // Get window size
    void getSize(int& width, int& height) const;
    
    // Set window size
    void setSize(int width, int height);
    
    // Get window position
    void getPosition(int& x, int& y) const;
    
    // Set window position
    void setPosition(int x, int y);
    
    // Set window title
    void setTitle(const std::string& title);
    
    // Get window title
    std::string getTitle() const;
    
    // Load URL
    bool loadUrl(const std::string& url);
    
    // Navigate back
    bool goBack();
    
    // Navigate forward
    bool goForward();
    
    // Reload current page
    bool reload();
    
    // Stop loading
    void stopLoading();
    
    // Set browser instance
    void setBrowser(std::shared_ptr<browser::Browser> browser);
    
    // Get browser instance
    std::shared_ptr<browser::Browser> getBrowser() const;
    
    // Process window events (non-blocking)
    void processEvents();
    
    // Run event loop (blocking)
    void runEventLoop();
    
    // Set URL change callback
    void setUrlChangeCallback(std::function<void(const std::string&)> callback);
    
    // Set title change callback
    void setTitleChangeCallback(std::function<void(const std::string&)> callback);
    
    // Set loading state change callback
    void setLoadingStateCallback(std::function<void(bool)> callback);
    
private:
    // Window configuration
    WindowConfig m_config;
    
    // Browser engine instance
    std::shared_ptr<browser::Browser> m_browser;
    
    // Window handle
    GLFWwindow* m_window;
    
    // Renderer for browser output
    std::shared_ptr<rendering::Renderer> m_renderer;
    std::shared_ptr<rendering::RenderTarget> m_renderTarget;
    
    // Current URL and navigation state
    std::string m_currentUrl;
    std::vector<std::string> m_history;
    size_t m_historyIndex;
    bool m_isLoading;
    
    // UI state
    bool m_initialized;
    
    // Callbacks
    std::function<void(const std::string&)> m_urlChangeCallback;
    std::function<void(const std::string&)> m_titleChangeCallback;
    std::function<void(bool)> m_loadingStateCallback;
    
    // Initialize UI toolkit
    bool initializeUI();
    
    // Render page content
    void renderPage();
    
    // Internal event handling
    void handleKeyEvent(int key, int scancode, int action, int mods);
    void handleMouseEvent(int button, int action, int mods);
    void handleResizeEvent(int width, int height);
    
    // Static callback wrappers for GLFW
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void windowSizeCallback(GLFWwindow* window, int width, int height);
    static void windowCloseCallback(GLFWwindow* window);
};

} // namespace ui
} // namespace browser

#endif // BROWSER_UI_WINDOW_H