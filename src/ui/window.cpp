#include "window.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

// Include GLFW for window management
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Simple OpenGL headers for rendering
#ifdef _WIN32
#include <Windows.h>
#endif
#include <GL/gl.h>

namespace browser {
namespace ui {

// Map to store window instances for callbacks
static std::map<GLFWwindow*, BrowserWindow*> g_windowMap;

//-----------------------------------------------------------------------------
// BrowserWindow Implementation
//-----------------------------------------------------------------------------

BrowserWindow::BrowserWindow(const WindowConfig& config)
    : m_config(config)
    , m_window(nullptr)
    , m_historyIndex(0)
    , m_isLoading(false)
    , m_initialized(false)
{
    // Create renderer
    m_renderer = std::make_shared<rendering::Renderer>();
    
    // Create browser engine instance
    m_browser = std::make_shared<browser::Browser>();
}

BrowserWindow::~BrowserWindow() {
    // Clean up
    if (m_window) {
        // Remove from window map
        g_windowMap.erase(m_window);
        
        // Destroy window
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    
    // Terminate GLFW if this is the last window
    if (g_windowMap.empty()) {
        glfwTerminate();
    }
}

bool BrowserWindow::initialize() {
    // Initialize browser engine
    if (!m_browser->initialize()) {
        std::cerr << "Failed to initialize browser engine" << std::endl;
        return false;
    }
    
    // Initialize renderer
    if (!m_renderer->initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }
    
    // Initialize UI toolkit
    if (!initializeUI()) {
        std::cerr << "Failed to initialize UI" << std::endl;
        return false;
    }
    
    // Create render target
    m_renderTarget = m_renderer->createTarget(rendering::RenderTargetType::BITMAP, m_config.width, m_config.height);
    if (!m_renderTarget) {
        std::cerr << "Failed to create render target" << std::endl;
        return false;
    }
    
    m_initialized = true;
    return true;
}

bool BrowserWindow::initializeUI() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // Create window
    glfwWindowHint(GLFW_RESIZABLE, m_config.resizable ? GLFW_TRUE : GLFW_FALSE);
    
    m_window = glfwCreateWindow(m_config.width, m_config.height, m_config.title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    // Store window instance for callbacks
    g_windowMap[m_window] = this;
    
    // Set window callbacks
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetWindowSizeCallback(m_window, windowSizeCallback);
    glfwSetWindowCloseCallback(m_window, windowCloseCallback);
    
    // Make the window's context current
    glfwMakeContextCurrent(m_window);
    
    // Initialize OpenGL
    glViewport(0, 0, m_config.width, m_config.height);
    
    // Maximize if requested
    if (m_config.maximized) {
        glfwMaximizeWindow(m_window);
    }
    
    return true;
}

void BrowserWindow::show() {
    if (m_window) {
        glfwShowWindow(m_window);
    }
}

void BrowserWindow::hide() {
    if (m_window) {
        glfwHideWindow(m_window);
    }
}

void BrowserWindow::close() {
    if (m_window) {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
}

bool BrowserWindow::isOpen() const {
    return m_window && !glfwWindowShouldClose(m_window);
}

void BrowserWindow::getSize(int& width, int& height) const {
    if (m_window) {
        glfwGetWindowSize(m_window, &width, &height);
    } else {
        width = m_config.width;
        height = m_config.height;
    }
}

void BrowserWindow::setSize(int width, int height) {
    if (m_window) {
        glfwSetWindowSize(m_window, width, height);
    }
    
    m_config.width = width;
    m_config.height = height;
}

void BrowserWindow::getPosition(int& x, int& y) const {
    if (m_window) {
        glfwGetWindowPos(m_window, &x, &y);
    } else {
        x = 0;
        y = 0;
    }
}

void BrowserWindow::setPosition(int x, int y) {
    if (m_window) {
        glfwSetWindowPos(m_window, x, y);
    }
}

void BrowserWindow::setTitle(const std::string& title) {
    if (m_window) {
        glfwSetWindowTitle(m_window, title.c_str());
    }
    
    m_config.title = title;
}

std::string BrowserWindow::getTitle() const {
    return m_config.title;
}

bool BrowserWindow::loadUrl(const std::string& url) {
    if (!m_initialized || !m_browser) {
        return false;
    }
    
    // Update loading state
    m_isLoading = true;
    if (m_loadingStateCallback) {
        m_loadingStateCallback(true);
    }
    
    // Load URL
    std::string error;
    bool result = m_browser->loadUrl(url, error);
    
    if (result) {
        // Update URL and history
        m_currentUrl = url;
        
        // If we're not at the end of the history, truncate it
        if (m_historyIndex < m_history.size() - 1) {
            m_history.resize(m_historyIndex + 1);
        }
        
        // Add to history
        m_history.push_back(url);
        m_historyIndex = m_history.size() - 1;
        
        // Call URL change callback
        if (m_urlChangeCallback) {
            m_urlChangeCallback(url);
        }
        
        // Update title
        std::string title = "Browser";
        if (m_browser->currentDocument()) {
            title = m_browser->currentDocument()->title();
            if (title.empty()) {
                title = url;
            }
        }
        
        setTitle(title);
        
        // Call title change callback
        if (m_titleChangeCallback) {
            m_titleChangeCallback(title);
        }
        
        // Render page
        renderPage();
    } else {
        std::cerr << "Failed to load URL: " << error << std::endl;
    }
    
    // Update loading state
    m_isLoading = false;
    if (m_loadingStateCallback) {
        m_loadingStateCallback(false);
    }
    
    return result;
}

bool BrowserWindow::goBack() {
    if (m_historyIndex > 0) {
        m_historyIndex--;
        return loadUrl(m_history[m_historyIndex]);
    }
    
    return false;
}

bool BrowserWindow::goForward() {
    if (m_historyIndex < m_history.size() - 1) {
        m_historyIndex++;
        return loadUrl(m_history[m_historyIndex]);
    }
    
    return false;
}

bool BrowserWindow::reload() {
    if (!m_currentUrl.empty()) {
        return loadUrl(m_currentUrl);
    }
    
    return false;
}

void BrowserWindow::stopLoading() {
    // Not implemented yet
}

void BrowserWindow::setBrowser(std::shared_ptr<browser::Browser> browser) {
    m_browser = browser;
}

std::shared_ptr<browser::Browser> BrowserWindow::getBrowser() const {
    return m_browser;
}

void BrowserWindow::processEvents() {
    if (m_window) {
        glfwPollEvents();
    }
}

void BrowserWindow::runEventLoop() {
    if (!m_window) {
        return;
    }
    
    // Main event loop
    while (!glfwWindowShouldClose(m_window)) {
        // Process events
        glfwPollEvents();
        
        // Render
        renderPage();
        
        // Limit frame rate
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
}

void BrowserWindow::setUrlChangeCallback(std::function<void(const std::string&)> callback) {
    m_urlChangeCallback = callback;
}

void BrowserWindow::setTitleChangeCallback(std::function<void(const std::string&)> callback) {
    m_titleChangeCallback = callback;
}

void BrowserWindow::setLoadingStateCallback(std::function<void(bool)> callback) {
    m_loadingStateCallback = callback;
}

void BrowserWindow::renderPage() {
    if (!m_window || !m_browser || !m_renderer || !m_renderTarget) {
        return;
    }
    
    // Make sure OpenGL context is current
    glfwMakeContextCurrent(m_window);
    
    // Clear the window
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Get the root box
    std::shared_ptr<layout::Box> rootBox = m_browser->layoutRoot();
    
    if (rootBox) {
        // Render the page to the render target
        m_renderer->render(rootBox.get(), m_renderTarget.get());
        
        // For simplicity, we'll just draw a colored rectangle where each element would be
        // In a real implementation, we'd use the render target's bitmap
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, m_config.width, m_config.height, 0, -1, 1);
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // Draw a border around the viewport
        glColor3f(0.8f, 0.8f, 0.8f);
        glBegin(GL_LINE_LOOP);
        glVertex2i(0, 0);
        glVertex2i(m_config.width, 0);
        glVertex2i(m_config.width, m_config.height);
        glVertex2i(0, m_config.height);
        glEnd();
        
        // Simple visualization of the layout tree
        renderLayoutBox(rootBox.get(), 0);
    }
    
    // Swap buffers
    glfwSwapBuffers(m_window);
}

void BrowserWindow::renderLayoutBox(layout::Box* box, int depth) {
    if (!box) {
        return;
    }
    
    // Draw the box
    layout::Rect rect = box->borderBox();
    
    // Choose color based on tag name
    float r = 0.9f, g = 0.9f, b = 0.9f;
    
    if (box->element()) {
        std::string tagName = box->element()->tagName();
        
        if (tagName == "div") {
            r = 0.8f; g = 0.8f; b = 0.8f;
        } else if (tagName == "p") {
            r = 0.8f; g = 0.9f; b = 0.8f;
        } else if (tagName == "a") {
            r = 0.0f; g = 0.0f; b = 0.8f;
        } else if (tagName == "img") {
            r = 0.8f; g = 0.8f; b = 0.9f;
        } else if (tagName == "h1" || tagName == "h2" || tagName == "h3") {
            r = 0.9f; g = 0.7f; b = 0.7f;
        } else if (tagName == "body") {
            r = 1.0f; g = 1.0f; b = 1.0f;
        } else if (tagName == "html") {
            r = 0.95f; g = 0.95f; b = 0.95f;
        }
    }
    
    // Fill the box
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(rect.x, rect.y);
    glVertex2f(rect.x + rect.width, rect.y);
    glVertex2f(rect.x + rect.width, rect.y + rect.height);
    glVertex2f(rect.x, rect.y + rect.height);
    glEnd();
    
    // Draw the border
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(rect.x, rect.y);
    glVertex2f(rect.x + rect.width, rect.y);
    glVertex2f(rect.x + rect.width, rect.y + rect.height);
    glVertex2f(rect.x, rect.y + rect.height);
    glEnd();
    
    // Recursively render children
    for (const auto& child : box->children()) {
        renderLayoutBox(child.get(), depth + 1);
    }
}

void BrowserWindow::handleKeyEvent(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        // Handle key press
        switch (key) {
            case GLFW_KEY_F5:
                // Reload
                reload();
                break;
                
            case GLFW_KEY_BACKSPACE:
                // Go back
                if (mods & GLFW_MOD_ALT) {
                    goBack();
                }
                break;
                
            case GLFW_KEY_RIGHT:
                // Go forward
                if (mods & GLFW_MOD_ALT) {
                    goForward();
                }
                break;
        }
    }
}

void BrowserWindow::handleMouseEvent(int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        // Handle mouse press
        double xpos, ypos;
        glfwGetCursorPos(m_window, &xpos, &ypos);
        
        // Simple click handling
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            // Find element at click position
            // This would require hit testing, not implemented here
        }
    }
}

void BrowserWindow::handleResizeEvent(int width, int height) {
    // Update viewport
    glViewport(0, 0, width, height);
    
    // Update config
    m_config.width = width;
    m_config.height = height;
    
    // Update render target
    m_renderTarget = m_renderer->createTarget(rendering::RenderTargetType::BITMAP, width, height);
    
    // Re-render page
    renderPage();
}

// Static callback handlers
void BrowserWindow::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto it = g_windowMap.find(window);
    if (it != g_windowMap.end()) {
        it->second->handleKeyEvent(key, scancode, action, mods);
    }
}

void BrowserWindow::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto it = g_windowMap.find(window);
    if (it != g_windowMap.end()) {
        it->second->handleMouseEvent(button, action, mods);
    }
}

void BrowserWindow::windowSizeCallback(GLFWwindow* window, int width, int height) {
    auto it = g_windowMap.find(window);
    if (it != g_windowMap.end()) {
        it->second->handleResizeEvent(width, height);
    }
}

void BrowserWindow::windowCloseCallback(GLFWwindow* window) {
    // Default behavior
}

} // namespace ui
} // namespace browser