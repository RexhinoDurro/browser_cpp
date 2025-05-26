// src/ui/browser_window.cpp
#include "browser_window.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>

namespace browser {
namespace ui {

//-----------------------------------------------------------------------------
// BrowserWindow Implementation
//-----------------------------------------------------------------------------

BrowserWindow::BrowserWindow(const WindowConfig& config)
    : m_historyIndex(0)
    , m_isLoading(false)
    , m_initialized(false)
{
    // Create platform window
    m_window = createPlatformWindow(config);
    
    // Create renderer
    m_renderer = std::make_shared<rendering::Renderer>();
    
    // Create browser engine instance
    m_browser = std::make_shared<browser::Browser>();
    
    // Create custom render context
    m_customContext = std::make_shared<rendering::CustomRenderContext>();
    
    // Create browser controls
    m_browserControls = std::make_shared<BrowserControls>(this);
}

BrowserWindow::~BrowserWindow() {
    // Close window
    if (m_window) {
        m_window->close();
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
    
    // Create window
    if (!m_window->create()) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }
    
    // Get window size
    int width, height;
    m_window->getSize(width, height);
    
    // Create render target using CustomRenderTarget
    m_renderTarget = std::make_shared<rendering::CustomRenderTarget>(width, height);
    if (!m_renderTarget) {
        std::cerr << "Failed to create render target" << std::endl;
        return false;
    }
    
    // Set up window callbacks
    m_window->setKeyCallback([this](Key key, KeyAction action) {
        handleKeyEvent(key, action);
    });
    
    m_window->setMouseButtonCallback([this](MouseButton button, MouseAction action, int x, int y) {
        handleMouseEvent(button, action, x, y);
    });
    
    m_window->setResizeCallback([this](int width, int height) {
        handleResizeEvent(width, height);
    });
    
    m_window->setCloseCallback([this]() {
        handleCloseEvent();
    });
    
    // Initialize browser controls
    if (!m_browserControls->initialize()) {
        std::cerr << "Failed to initialize browser controls" << std::endl;
        return false;
    }
    
    // Load default page
    showDefaultPage();
    
    m_initialized = true;
    return true;
}

void BrowserWindow::show() {
    if (m_window) {
        m_window->show();
    }
}

void BrowserWindow::hide() {
    if (m_window) {
        m_window->hide();
    }
}

void BrowserWindow::close() {
    if (m_window) {
        m_window->close();
    }
}

bool BrowserWindow::isOpen() const {
    return m_initialized && m_window != nullptr;
}

void BrowserWindow::setSize(int width, int height) {
    if (m_window) {
        m_window->setSize(width, height);
    }
}

void BrowserWindow::getSize(int& width, int& height) const {
    if (m_window) {
        m_window->getSize(width, height);
    } else {
        width = 0;
        height = 0;
    }
}

void BrowserWindow::setPosition(int x, int y) {
    if (m_window) {
        m_window->setPosition(x, y);
    }
}

void BrowserWindow::getPosition(int& x, int& y) const {
    if (m_window) {
        m_window->getPosition(x, y);
    } else {
        x = 0;
        y = 0;
    }
}

void BrowserWindow::setTitle(const std::string& title) {
    if (m_window) {
        m_window->setTitle(title);
    }
}

std::string BrowserWindow::getTitle() const {
    if (m_window) {
        return m_window->getTitle();
    }
    return "";
}

void BrowserWindow::processEvents() {
    if (m_window) {
        m_window->processEvents();
    }
}

void BrowserWindow::runEventLoop() {
    while (isOpen()) {
        processEvents();
        
        // Render page if needed
        if (m_needsRender) {
            renderPage();
            m_needsRender = false;
        }
        
        // Sleep a bit to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void BrowserWindow::renderPage() {
    if (!m_browser || !m_window || !m_renderer || !m_renderTarget) {
        return;
    }
    
    // Get the layout root from the browser
    auto layoutRoot = m_browser->layoutRoot();
    if (!layoutRoot) {
        return;
    }
    
    // Get the window size
    int width, height;
    m_window->getSize(width, height);
    
    // Begin painting
    m_window->beginPaint();
    
    // Get canvas (if using platform canvas)
    auto canvas = m_window->getCanvas();
    
    // Clear the window
    if (canvas) {
        canvas->clear(Canvas::rgb(255, 255, 255));
    }
    
    // Draw browser controls
    if (m_browserControls && m_customContext) {
        m_browserControls->draw(m_customContext.get());
    }
    
    // Calculate content area (below toolbar)
    int toolbarHeight = 40; // Height of the toolbar
    int contentWidth = width;
    int contentHeight = height - toolbarHeight;
    
    // Render the page content
    m_renderer->render(layoutRoot.get(), m_renderTarget.get());
    
    // For now, just draw a simple placeholder for the content
    if (canvas) {
        // Draw content area background
        canvas->drawRect(0, toolbarHeight, contentWidth, contentHeight, 
                         Canvas::rgb(255, 255, 255), true);
        
        // Draw some placeholder text
        canvas->drawText("Page content here", 10, toolbarHeight + 20, 
                        Canvas::rgb(0, 0, 0), "Arial", 14);
    }
    
    // End painting
    m_window->endPaint();
}

bool BrowserWindow::loadUrl(const std::string& input) {
    std::string url = input;
    
    // If no scheme is specified, default to http://
    if (url.find("://") == std::string::npos) {
        // Check if it's a file path
        if (url.find('/') == 0 || url.find('\\') == 0 || 
            (url.length() > 1 && url[1] == ':')) {
            url = "file://" + url;
        } else {
            // If it contains a space, assume it's a search query
            if (url.find(' ') != std::string::npos) {
                // URL encode the search query
                std::string encoded;
                for (char c : url) {
                    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                        encoded += c;
                    } else if (c == ' ') {
                        encoded += '+';
                    } else {
                        char hex[4];
                        snprintf(hex, sizeof(hex), "%%%02X", (unsigned char)c);
                        encoded += hex;
                    }
                }
                url = "https://www.google.com/search?q=" + encoded;
            } else {
                url = "http://" + url;
            }
        }
    }
    
    // Now load the URL
    return loadUrlInternal(url);
}

bool BrowserWindow::loadUrlInternal(const std::string& url) {
    if (!m_browser) {
        return false;
    }
    
    // Set loading state
    m_isLoading = true;
    updateLoadingState(true);
    
    // Load URL with browser engine
    std::string error;
    bool success = m_browser->loadUrl(url, error);
    
    if (success) {
        // Update address bar through browser controls
        if (m_browserControls) {
            m_browserControls->setAddressBarText(url);
        }
        
        // Add to history if it's a new URL
        if (m_currentUrl != url) {
            // If we're not at the end of history, truncate forward history
            if (m_historyIndex < m_history.size() - 1) {
                m_history.resize(m_historyIndex + 1);
            }
            
            m_history.push_back(url);
            m_historyIndex = m_history.size() - 1;
            m_currentUrl = url;
        }
        
        // Update UI state
        updateNavigationButtons();
        
        // Render the new page
        m_needsRender = true;
    } else {
        // Show error
        showErrorPage(url, error);
    }
    
    // Update loading state
    m_isLoading = false;
    updateLoadingState(false);
    
    return success;
}

bool BrowserWindow::goBack() {
    if (m_historyIndex > 0) {
        m_historyIndex--;
        std::string url = m_history[m_historyIndex];
        
        // Don't add to history when navigating back
        bool oldIsLoading = m_isLoading;
        m_isLoading = true;
        updateLoadingState(true);
        
        std::string error;
        bool success = m_browser->loadUrl(url, error);
        
        if (success) {
            m_currentUrl = url;
            if (m_browserControls) {
                m_browserControls->setAddressBarText(url);
            }
            
            updateNavigationButtons();
            m_needsRender = true;
        } else {
            // Show error
            showErrorPage(url, error);
        }
        
        m_isLoading = oldIsLoading;
        updateLoadingState(oldIsLoading);
        
        return success;
    }
    
    return false;
}

bool BrowserWindow::goForward() {
    if (m_historyIndex < m_history.size() - 1) {
        m_historyIndex++;
        std::string url = m_history[m_historyIndex];
        
        // Don't add to history when navigating forward
        bool oldIsLoading = m_isLoading;
        m_isLoading = true;
        updateLoadingState(true);
        
        std::string error;
        bool success = m_browser->loadUrl(url, error);
        
        if (success) {
            m_currentUrl = url;
            if (m_browserControls) {
                m_browserControls->setAddressBarText(url);
            }
            
            updateNavigationButtons();
            m_needsRender = true;
        } else {
            // Show error
            showErrorPage(url, error);
        }
        
        m_isLoading = oldIsLoading;
        updateLoadingState(oldIsLoading);
        
        return success;
    }
    
    return false;
}

bool BrowserWindow::reload() {
    if (!m_currentUrl.empty()) {
        return loadUrlInternal(m_currentUrl);
    }
    return false;
}

void BrowserWindow::stopLoading() {
    // Cancel any ongoing loads
    if (m_isLoading && m_browser) {
        // In a real browser, you would have a method like:
        // m_browser->cancelLoad();
        
        m_isLoading = false;
        updateLoadingState(false);
    }
}

void BrowserWindow::showDefaultPage() {
    // Create a simple HTML default page
    std::string defaultHtml = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Simple Browser</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 40px;
            background-color: #f0f0f0;
            color: #333;
        }
        h1 {
            color: #2c3e50;
            text-align: center;
        }
        .search-box {
            width: 100%;
            max-width: 600px;
            margin: 20px auto;
            padding: 10px;
            font-size: 16px;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
        .bookmarks {
            display: flex;
            flex-wrap: wrap;
            justify-content: center;
            margin-top: 40px;
        }
        .bookmark {
            display: flex;
            flex-direction: column;
            align-items: center;
            width: 120px;
            height: 120px;
            margin: 10px;
            padding: 15px;
            text-align: center;
            text-decoration: none;
            color: #333;
            background-color: white;
            border-radius: 8px;
            box-shadow: 0 2px 5px rgba(0,0,0,0.1);
        }
        .bookmark:hover {
            box-shadow: 0 4px 8px rgba(0,0,0,0.2);
        }
        .bookmark-icon {
            font-size: 36px;
            margin-bottom: 10px;
        }
    </style>
</head>
<body>
    <h1>Simple Browser</h1>
    
    <div class="bookmarks">
        <a href="https://www.google.com" class="bookmark">
            <div class="bookmark-icon">G</div>
            <div>Google</div>
        </a>
        <a href="https://www.github.com" class="bookmark">
            <div class="bookmark-icon">GH</div>
            <div>GitHub</div>
        </a>
        <a href="https://www.wikipedia.org" class="bookmark">
            <div class="bookmark-icon">W</div>
            <div>Wikipedia</div>
        </a>
        <a href="https://www.youtube.com" class="bookmark">
            <div class="bookmark-icon">YT</div>
            <div>YouTube</div>
        </a>
    </div>
</body>
</html>
)";

    // Load the default page
    if (m_browser) {
        m_browser->htmlParser()->parse(defaultHtml);
        
        // Set a default URL for the about page
        std::string aboutUrl = "about:home";
        m_currentUrl = aboutUrl;
        m_history.push_back(aboutUrl);
        m_historyIndex = 0;
        
        if (m_browserControls) {
            m_browserControls->setAddressBarText(aboutUrl);
        }
        
        // Update UI state
        updateNavigationButtons();
        
        // Render the page
        m_needsRender = true;
    }
}

void BrowserWindow::showErrorPage(const std::string& url, const std::string& error) {
    // Create an error page
    std::string errorHtml = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Error Loading Page</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 40px;
            background-color: #f8f8f8;
            color: #333;
        }
        .error-container {
            max-width: 600px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 5px;
            box-shadow: 0 2px 5px rgba(0,0,0,0.1);
        }
        h1 {
            color: #e74c3c;
        }
        .url {
            word-break: break-all;
            color: #3498db;
            margin: 10px 0;
            padding: 5px;
            background-color: #f0f0f0;
        }
        .error-message {
            margin-top: 20px;
            padding: 10px;
            background-color: #f0f0f0;
        }
    </style>
</head>
<body>
    <div class="error-container">
        <h1>Error Loading Page</h1>
        <p>The browser could not load the following URL:</p>
        <div class="url">)" + url + R"(</div>
        <p>Error message:</p>
        <div class="error-message">)" + error + R"(</div>
    </div>
</body>
</html>
)";

    // Load the error page
    if (m_browser) {
        m_browser->htmlParser()->parse(errorHtml);
        
        // Don't update history or URL for error pages
        
        // Render the page
        m_needsRender = true;
    }
}

void BrowserWindow::setBrowser(std::shared_ptr<browser::Browser> browser) {
    m_browser = browser;
}

std::shared_ptr<browser::Browser> BrowserWindow::getBrowser() const {
    return m_browser;
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

void BrowserWindow::updateNavigationButtons() {
    // Navigation buttons are now managed by BrowserControls
    // We need to notify it about navigation state changes
    // For now, this is a placeholder - you might want to add methods to BrowserControls
    // to update button states based on history
}

void BrowserWindow::updateLoadingState(bool isLoading) {
    // Update loading state in browser controls
    if (m_browserControls) {
        m_browserControls->setLoading(isLoading);
    }
    
    // Call loading state callback
    if (m_loadingStateCallback) {
        m_loadingStateCallback(isLoading);
    }
}

void BrowserWindow::handleKeyEvent(Key key, KeyAction action) {
    // First, let browser controls handle the event
    if (m_browserControls) {
        // Convert Key and KeyAction to int for compatibility with Control interface
        if (m_browserControls->handleKeyInput(static_cast<int>(key), 0, 
                                             static_cast<int>(action), 0)) {
            return; // Event was handled by controls
        }
    }
    
    // Handle browser keyboard shortcuts
    if (action == KeyAction::Press) {
        // Check for Ctrl key combinations
        bool ctrlPressed = false; // TODO: Need to track modifier keys
        
        if (ctrlPressed) {
            switch (key) {
                case Key::R:
                    // Ctrl+R: Reload
                    reload();
                    break;
                    
                case Key::L:
                    // Ctrl+L: Focus address bar
                    // This would be handled by BrowserControls
                    break;
                    
                case Key::T:
                    // Ctrl+T: New tab (not implemented in this simple browser)
                    break;
                    
                case Key::W:
                    // Ctrl+W: Close window
                    close();
                    break;
            }
        } else {
            // Regular key presses
            switch (key) {
                case Key::F5:
                    // F5: Reload
                    reload();
                    break;
                    
                case Key::Escape:
                    // Escape: Stop loading
                    if (m_isLoading) {
                        stopLoading();
                    }
                    break;
            }
        }
    }
}

void BrowserWindow::handleMouseEvent(MouseButton button, MouseAction action, int x, int y) {
    // First, let browser controls handle the event
    if (m_browserControls) {
        // Convert MouseButton and MouseAction to int for compatibility
        if (m_browserControls->handleMouseButton(static_cast<int>(button), 
                                               static_cast<int>(action), 0, x, y)) {
            return; // Event was handled by controls
        }
    }
    
    // Check if the click is in the content area
    int toolbarHeight = 40;
    if (y > toolbarHeight && m_browser && m_browser->currentDocument()) {
        // Convert coordinates to content area
        int contentX = x;
        int contentY = y - toolbarHeight;
        
        // Forward event to the browser engine
        // In a real browser, you would have something like:
        // m_browser->handleMouseEvent(button, action, contentX, contentY);
        
        // For now, just handle basic link clicking
        if (button == MouseButton::Left && action == MouseAction::Press) {
            // Find the element under the cursor
            html::Element* clickedElement = findElementAtPosition(contentX, contentY);
            
            if (clickedElement) {
                // Check if it's a link
                if (clickedElement->tagName() == "a" && 
                    clickedElement->hasAttribute("href")) {
                    std::string href = clickedElement->getAttribute("href");
                    
                    // Handle relative URLs
                    if (href[0] == '/') {
                        // Absolute path relative to domain
                        size_t domainEnd = m_currentUrl.find("/", m_currentUrl.find("//") + 2);
                        if (domainEnd != std::string::npos) {
                            href = m_currentUrl.substr(0, domainEnd) + href;
                        } else {
                            href = m_currentUrl + href;
                        }
                    } else if (href.find("://") == std::string::npos) {
                        // Relative path
                        size_t lastSlash = m_currentUrl.find_last_of('/');
                        if (lastSlash != std::string::npos) {
                            href = m_currentUrl.substr(0, lastSlash + 1) + href;
                        } else {
                            href = m_currentUrl + "/" + href;
                        }
                    }
                    
                    loadUrl(href);
                }
            }
        }
    }
}

void BrowserWindow::handleResizeEvent(int width, int height) {
    // Notify browser controls about resize
    if (m_browserControls) {
        m_browserControls->handleResize(width, height);
    }
    
    // Resize the render target
    if (m_renderer) {
        m_renderTarget = std::make_shared<rendering::CustomRenderTarget>(width, height);
    }
    
    // Re-layout the page
    if (m_browser && m_browser->currentDocument()) {
        css::StyleResolver* styleResolver = m_browser->styleResolver();
        if (styleResolver) {
            m_browser->layoutEngine()->layoutDocument(
                m_browser->currentDocument(), styleResolver, width, height - 40);
        }
    }
    
    // Redraw the page
    m_needsRender = true;
}

void BrowserWindow::handleCloseEvent() {
    // Handle window close
    m_initialized = false;
}

// Helper function to find element at position (simplified)
html::Element* BrowserWindow::findElementAtPosition(int x, int y) {
    if (!m_browser || !m_browser->layoutEngine()) {
        return nullptr;
    }
    
    // This is a very simplified approach
    // A real browser would traverse the layout tree and do proper hit testing
    
    // Get all links in the document
    html::Document* doc = m_browser->currentDocument();
    if (!doc) return nullptr;
    
    std::vector<html::Element*> links = doc->getElementsByTagName("a");
    
    for (html::Element* link : links) {
        // Get the box for this element
        layout::Box* box = m_browser->layoutEngine()->getBoxForNode(link);
        if (box) {
            // Check if point is inside box
            layout::Rect rect = box->borderBox();
            if (x >= rect.x && x < rect.x + rect.width &&
                y >= rect.y && y < rect.y + rect.height) {
                return link;
            }
        }
    }
    
    return nullptr;
}

// Initialize controls is no longer needed as BrowserControls handles this
void BrowserWindow::initializeControls() {
    // This method is kept for compatibility but does nothing
    // All control initialization is now handled by BrowserControls
}

} // namespace ui
} // namespace browser