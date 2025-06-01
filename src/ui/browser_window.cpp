// src/ui/browser_window.cpp

// Fix for Windows min/max macros
#ifdef _WIN32
#define NOMINMAX
#endif

#include "browser_window.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include "rendering/paint_system.h"

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
    // Initialize browser engine first
    if (!m_browser) {
        std::cerr << "Browser instance not set" << std::endl;
        return false;
    }
    
    if (!m_browser->initialize()) {
        std::cerr << "Failed to initialize browser engine" << std::endl;
        return false;
    }
    
    // Initialize renderer with paint system
    if (!m_renderer->initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }
    
    // Create and set paint system
    auto paintSystem = std::make_shared<rendering::PaintSystem>();
    if (!paintSystem->initialize()) {
        std::cerr << "Failed to initialize paint system" << std::endl;
        return false;
    }
    m_renderer->setPaintSystem(paintSystem);
    
    // Create window
    if (!m_window->create()) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }
    
    // Set initial window title
    m_window->setTitle("Simple Browser");
    
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
        
        // Always render if we need to
        if (m_needsRender) {
            renderPage();
            m_needsRender = false;
        }
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
    if (!m_browser || !m_window || !m_renderer) {
        std::cerr << "renderPage: Missing required components" << std::endl;
        return;
    }
    
    // Get the window size
    int width, height;
    m_window->getSize(width, height);
    
    // Begin painting
    m_window->beginPaint();
    
    // Get canvas
    auto canvas = m_window->getCanvas();
    
    if (!canvas) {
        std::cerr << "renderPage: Canvas is null!" << std::endl;
        m_window->endPaint();
        return;
    }
    
    // Clear the window with white background
    canvas->clear(Canvas::rgb(255, 255, 255));
    
    // Draw browser controls first
    if (m_browserControls) {
        // Draw toolbar background
        canvas->drawRect(0, 0, width, 40, Canvas::rgb(240, 240, 240), true);
        canvas->drawRect(0, 40, width, 1, Canvas::rgb(200, 200, 200), true); // Border
        
        // Draw navigation buttons
        // Back button
        canvas->drawRect(5, 5, 30, 30, Canvas::rgb(220, 220, 220), true);
        canvas->drawText("←", 12, 22, Canvas::rgb(0, 0, 0), "Arial", 20);
        
        // Forward button
        canvas->drawRect(40, 5, 30, 30, Canvas::rgb(220, 220, 220), true);
        canvas->drawText("→", 47, 22, Canvas::rgb(0, 0, 0), "Arial", 20);
        
        // Reload button
        canvas->drawRect(75, 5, 30, 30, Canvas::rgb(220, 220, 220), true);
        canvas->drawText("↻", 82, 22, Canvas::rgb(0, 0, 0), "Arial", 20);
        
        // Address bar
        canvas->drawRect(110, 5, width - 120, 30, Canvas::rgb(255, 255, 255), true);
        canvas->drawRect(110, 5, width - 120, 30, Canvas::rgb(180, 180, 180), false);
        
        // Address text
        if (!m_currentUrl.empty()) {
            canvas->drawText(m_currentUrl, 115, 22, Canvas::rgb(0, 0, 0), "Arial", 14);
        } else {
            canvas->drawText("Enter URL...", 115, 22, Canvas::rgb(180, 180, 180), "Arial", 14);
        }
    }
    
    // Calculate content area (below toolbar)
    int toolbarHeight = 40;
    int contentY = toolbarHeight + 1; // +1 for border
    int contentHeight = height - contentY;
    
    // Get the layout root from the browser
    auto layoutRoot = m_browser->layoutRoot();
    
    if (layoutRoot) {
        // Create a paint system if we don't have one
        auto paintSystem = m_renderer->getPaintSystem();
        if (!paintSystem) {
            paintSystem = std::make_shared<rendering::PaintSystem>();
            paintSystem->initialize();
            m_renderer->setPaintSystem(paintSystem);
        }
        
        // Create a paint context
        rendering::PaintContext context = paintSystem->createContext(layoutRoot.get());
        
        // Paint the layout tree to the context
        paintSystem->paintBox(layoutRoot.get(), context);
        
        // Now render the display list directly to our canvas
        const rendering::DisplayList& displayList = context.displayList();
        
        // Clear content area
        canvas->drawRect(0, contentY, width, contentHeight, Canvas::rgb(255, 255, 255), true);
        
        // Render each display item
        for (const auto& item : displayList.items()) {
            if (!item) continue;
            
            switch (item->type()) {
                case rendering::DisplayItemType::BACKGROUND: {
                    auto bgItem = static_cast<rendering::BackgroundDisplayItem*>(item.get());
                    canvas->drawRect(
                        static_cast<int>(bgItem->rect().x), 
                        static_cast<int>(bgItem->rect().y) + contentY,
                        static_cast<int>(bgItem->rect().width), 
                        static_cast<int>(bgItem->rect().height),
                        Canvas::rgb(bgItem->color().r, bgItem->color().g, bgItem->color().b),
                        true
                    );
                    break;
                }
                
                case rendering::DisplayItemType::BORDER: {
                    auto borderItem = static_cast<rendering::BorderDisplayItem*>(item.get());
                    
                    // Calculate max border width
                    float maxWidth = borderItem->topWidth();
                    if (borderItem->rightWidth() > maxWidth) maxWidth = borderItem->rightWidth();
                    if (borderItem->bottomWidth() > maxWidth) maxWidth = borderItem->bottomWidth();
                    if (borderItem->leftWidth() > maxWidth) maxWidth = borderItem->leftWidth();
                    
                    canvas->drawRect(
                        static_cast<int>(borderItem->rect().x), 
                        static_cast<int>(borderItem->rect().y) + contentY,
                        static_cast<int>(borderItem->rect().width), 
                        static_cast<int>(borderItem->rect().height),
                        Canvas::rgb(borderItem->color().r, borderItem->color().g, borderItem->color().b),
                        false,
                        static_cast<int>(maxWidth)
                    );
                    break;
                }
                
                case rendering::DisplayItemType::TEXT: {
                    auto textItem = static_cast<rendering::TextDisplayItem*>(item.get());
                    canvas->drawText(
                        textItem->text(),
                        static_cast<int>(textItem->x()),
                        static_cast<int>(textItem->y()) + contentY,
                        Canvas::rgb(textItem->color().r, textItem->color().g, textItem->color().b),
                        textItem->fontFamily(),
                        static_cast<int>(textItem->fontSize())
                    );
                    break;
                }
                
                case rendering::DisplayItemType::RECT: {
                    auto rectItem = static_cast<rendering::RectDisplayItem*>(item.get());
                    canvas->drawRect(
                        static_cast<int>(rectItem->rect().x),
                        static_cast<int>(rectItem->rect().y) + contentY,
                        static_cast<int>(rectItem->rect().width),
                        static_cast<int>(rectItem->rect().height),
                        Canvas::rgb(rectItem->color().r, rectItem->color().g, rectItem->color().b),
                        rectItem->filled()
                    );
                    break;
                }
                
                default:
                    // Other display item types not implemented yet
                    break;
            }
        }
        
        // Debug: Also show ASCII rendering
        std::string ascii = m_renderer->renderToASCII(layoutRoot.get(), 80, 24);
        std::cout << "ASCII render of page:\n" << ascii << std::endl;
        
    } else {
        // No layout root - draw placeholder
        canvas->drawRect(0, contentY, width, contentHeight, Canvas::rgb(250, 250, 250), true);
        canvas->drawText("No page loaded", width/2 - 50, height/2, Canvas::rgb(150, 150, 150), "Arial", 16);
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
    <title>Simple Browser - Home</title>
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
    <p style="text-align: center;">Welcome to Simple Browser</p>
    
    <div class="bookmarks">
        <a href="https://www.google.com" class="bookmark">
            <div class="bookmark-icon">🔍</div>
            <div>Google</div>
        </a>
        <a href="https://www.github.com" class="bookmark">
            <div class="bookmark-icon">💻</div>
            <div>GitHub</div>
        </a>
        <a href="https://www.wikipedia.org" class="bookmark">
            <div class="bookmark-icon">📚</div>
            <div>Wikipedia</div>
        </a>
        <a href="https://www.youtube.com" class="bookmark">
            <div class="bookmark-icon">📺</div>
            <div>YouTube</div>
        </a>
    </div>
</body>
</html>
)";

    // Load the default page properly
    if (m_browser) {
        std::string error;
        
        // Parse the HTML directly
        html::DOMTree domTree = m_browser->htmlParser()->parse(defaultHtml);
        
        // Set the DOM tree in the browser (you may need to add this method)
        // For now, let's use loadUrl with a data URL
        std::string dataUrl = "data:text/html;charset=utf-8," + defaultHtml;
        
        // Actually, let's load it as about:home
        m_browser->loadUrl("about:home", error);
        
        if (!error.empty()) {
            std::cerr << "Error loading default page: " << error << std::endl;
        }
        
        // Set a default URL for the about page
        m_currentUrl = "about:home";
        m_history.clear();
        m_history.push_back(m_currentUrl);
        m_historyIndex = 0;
        
        if (m_browserControls) {
            m_browserControls->setAddressBarText(m_currentUrl);
        }
        
        // Update window title explicitly
        setTitle("Simple Browser - Home");
        
        // Update UI state
        updateNavigationButtons();
        
        // Force a render
        m_needsRender = true;
        renderPage();
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
                m_browser->currentDocument(), styleResolver, 
                static_cast<float>(width), static_cast<float>(height - 40));
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