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
    
    // Create render target
    m_renderTarget = m_renderer->createTarget(rendering::RenderTargetType::BITMAP, width, height);
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
    
    // Initialize UI controls
    initializeControls();
    
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
    // Simple implementation - could be improved
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

void BrowserWindow::initializeControls() {
    // Get window size
    int width, height;
    m_window->getSize(width, height);
    
    // Create toolbar controls
    int buttonWidth = 32;
    int buttonHeight = 32;
    int buttonPadding = 4;
    int toolbarHeight = 40;
    int addressBarHeight = 28;
    
    // Back button
    m_backButton = std::make_shared<Button>(
        buttonPadding, 
        (toolbarHeight - buttonHeight) / 2, 
        buttonWidth, 
        buttonHeight, 
        "◀"
    );
    m_backButton->setClickHandler([this]() {
        goBack();
    });
    m_backButton->setEnabled(false); // Initially disabled
    m_window->addControl(m_backButton);
    
    // Forward button
    m_forwardButton = std::make_shared<Button>(
        buttonPadding * 2 + buttonWidth, 
        (toolbarHeight - buttonHeight) / 2, 
        buttonWidth, 
        buttonHeight, 
        "▶"
    );
    m_forwardButton->setClickHandler([this]() {
        goForward();
    });
    m_forwardButton->setEnabled(false); // Initially disabled
    m_window->addControl(m_forwardButton);
    
    // Reload button
    m_reloadButton = std::make_shared<Button>(
        buttonPadding * 3 + buttonWidth * 2, 
        (toolbarHeight - buttonHeight) / 2, 
        buttonWidth, 
        buttonHeight, 
        "↻"
    );
    m_reloadButton->setClickHandler([this]() {
        reload();
    });
    m_window->addControl(m_reloadButton);
    
    // Stop button
    m_stopButton = std::make_shared<Button>(
        buttonPadding * 4 + buttonWidth * 3, 
        (toolbarHeight - buttonHeight) / 2, 
        buttonWidth, 
        buttonHeight, 
        "✕"
    );
    m_stopButton->setClickHandler([this]() {
        stopLoading();
    });
    m_stopButton->setVisible(false); // Hide initially
    m_window->addControl(m_stopButton);
    
    // Address bar
    int addressBarX = buttonPadding * 5 + buttonWidth * 4;
    int addressBarWidth = width - addressBarX - buttonPadding;
    m_addressBar = std::make_shared<TextInput>(
        addressBarX,
        (toolbarHeight - addressBarHeight) / 2,
        addressBarWidth,
        addressBarHeight
    );
    m_addressBar->setPlaceholder("Enter URL...");
    m_addressBar->setSubmitHandler([this](const std::string& text) {
        loadUrl(text);
    });
    m_window->addControl(m_addressBar);
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
    
    // Calculate content area (below toolbar)
    int toolbarHeight = 40; // Height of the toolbar
    int contentWidth = width;
    int contentHeight = height - toolbarHeight;
    
    // Create a canvas for the content area
    auto canvas = m_window->getCanvas();
    if (!canvas) {
        return;
    }
    
    // Clear the content area
    canvas->drawRect(0, toolbarHeight, contentWidth, contentHeight, 
                     Canvas::rgb(255, 255, 255), true);
    
    // Render the page
    m_renderer->render(layoutRoot.get(), m_renderTarget.get());
    
    // Copy render target to window canvas
    // This would depend on your renderer implementation
    // For simplicity, we'll just use ASCII rendering for now
    std::string asciiOutput = m_renderer->renderToASCII(layoutRoot.get(), contentWidth, contentHeight);
    
    // Draw the ASCII output to canvas
    int lineHeight = 12;
    int y = toolbarHeight + 5;
    
    std::istringstream iss(asciiOutput);
    std::string line;
    while (std::getline(iss, line) && y < height) {
        canvas->drawText(line, 5, y, Canvas::rgb(0, 0, 0));
        y += lineHeight;
    }
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
        // Update address bar
        if (m_addressBar) {
            m_addressBar->setText(url);
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
            if (m_addressBar) {
                m_addressBar->setText(url);
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
            if (m_addressBar) {
                m_addressBar->setText(url);
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
        
        if (m_addressBar) {
            m_addressBar->setText(aboutUrl);
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
    if (m_backButton) {
        m_backButton->setEnabled(m_historyIndex > 0);
    }
    
    if (m_forwardButton) {
        m_forwardButton->setEnabled(m_historyIndex < m_history.size() - 1);
    }
}

void BrowserWindow::updateLoadingState(bool isLoading) {
    if (m_reloadButton) {
        m_reloadButton->setVisible(!isLoading);
    }
    
    if (m_stopButton) {
        m_stopButton->setVisible(isLoading);
    }
    
    // Call loading state callback
    if (m_loadingStateCallback) {
        m_loadingStateCallback(isLoading);
    }
}

void BrowserWindow::handleKeyEvent(Key key, KeyAction action) {
    // Handle browser keyboard shortcuts
    if (action == KeyAction::Press) {
        // Check for Ctrl key combinations
        bool ctrlPressed = (int)Key::Control & 0x01; // Check if Ctrl is pressed
        
        if (ctrlPressed) {
            switch (key) {
                case Key::R:
                    // Ctrl+R: Reload
                    reload();
                    break;
                    
                case Key::L:
                    // Ctrl+L: Focus address bar
                    if (m_addressBar) {
                        m_addressBar->setFocus(true);
                    }
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
            // This is a simplified approach - a real browser would use hit testing
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
    // Resize address bar
    if (m_addressBar) {
        int buttonWidth = 32;
        int buttonPadding = 4;
        int addressBarX = buttonPadding * 5 + buttonWidth * 4;
        int addressBarWidth = width - addressBarX - buttonPadding;
        int addressBarHeight = 28; // Use the same height as in initializeControls
        
        m_addressBar->setSize(addressBarWidth, addressBarHeight);
    }
    
    // Resize the render target
    if (m_renderer) {
        m_renderTarget = m_renderer->createTarget(
            rendering::RenderTargetType::BITMAP, width, height);
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
    // For now, just set m_initialized to false to stop the event loop
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

} // namespace ui
} // namespace browser