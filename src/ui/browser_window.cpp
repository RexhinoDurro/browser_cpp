// src/ui/browser_window.cpp - Fixed version with proper rendering

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
    std::cout << "BrowserWindow::initialize() starting..." << std::endl;
    
    // Initialize browser engine first
    if (!m_browser) {
        std::cerr << "Browser instance not set" << std::endl;
        return false;
    }
    
    if (!m_browser->initialize()) {
        std::cerr << "Failed to initialize browser engine" << std::endl;
        return false;
    }
    std::cout << "Browser engine initialized" << std::endl;
    
    // Initialize renderer with paint system
    if (!m_renderer->initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }
    std::cout << "Renderer initialized" << std::endl;
    
    // Create and set paint system
    auto paintSystem = std::make_shared<rendering::PaintSystem>();
    if (!paintSystem->initialize()) {
        std::cerr << "Failed to initialize paint system" << std::endl;
        return false;
    }
    m_renderer->setPaintSystem(paintSystem);
    std::cout << "Paint system initialized" << std::endl;
    
    // Create window
    if (!m_window) {
        std::cerr << "Window not created!" << std::endl;
        return false;
    }
    
    if (!m_window->create()) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }
    std::cout << "Window created successfully" << std::endl;
    
    // Set initial window title
    m_window->setTitle("Simple Browser");
    
    // Get window size
    int width, height;
    m_window->getSize(width, height);
    std::cout << "Window size: " << width << "x" << height << std::endl;
    
    // Create render target using CustomRenderTarget
    m_renderTarget = std::make_shared<rendering::CustomRenderTarget>(width, height);
    if (!m_renderTarget) {
        std::cerr << "Failed to create render target" << std::endl;
        return false;
    }
    std::cout << "Render target created" << std::endl;
    
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
    std::cout << "Window callbacks set" << std::endl;
    
    // Initialize browser controls
    if (!m_browserControls->initialize()) {
        std::cerr << "Failed to initialize browser controls" << std::endl;
        return false;
    }
    std::cout << "Browser controls initialized" << std::endl;
    
    // Load default page
    std::cout << "Loading default page..." << std::endl;
    showDefaultPage();
    
    m_initialized = true;
    std::cout << "BrowserWindow::initialize() completed successfully" << std::endl;
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
    return m_initialized && m_window && m_window->processEvents();
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
        // Process window events
        m_window->processEvents();
        
        // Check if we need to render
        static int frameCount = 0;
        frameCount++;
        
        // Always render if needed, or periodically to ensure display updates
        if (m_needsRender || (frameCount % 60 == 0)) {
            renderPage();
            m_needsRender = false;
        }
    }
}

void BrowserWindow::runEventLoop() {
    std::cout << "Entering event loop..." << std::endl;
    
    if (m_window && m_initialized) {
        m_window->show();
        std::cout << "Window shown" << std::endl;
    }
    
    int frameCount = 0;
    auto lastRenderTime = std::chrono::steady_clock::now();
    
    while (m_initialized && m_window) {
        // Process window events
        if (!m_window->processEvents()) {
            std::cout << "Window closed or error in processEvents" << std::endl;
            break;
        }
        
        // Calculate time since last render
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRenderTime);
        
        // Render at 60 FPS or when needed
        if (m_needsRender || elapsed.count() >= 16) {
            renderPage();
            m_needsRender = false;
            lastRenderTime = now;
        }
        
        frameCount++;
        
        // Sleep a bit to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::cout << "Exiting event loop" << std::endl;
}
// Helper function to render a box recursively
void renderBox(Canvas* canvas, layout::Box* box, int offsetX, int offsetY) {
    if (!box || box->displayType() == layout::DisplayType::NONE) {
        return;
    }
    
    // Get box rectangles
    layout::Rect contentRect = box->contentRect();
    layout::Rect borderBox = box->borderBox();
    layout::Rect marginBox = box->marginBox();
    
    // Apply offset
    contentRect.x += offsetX;
    contentRect.y += offsetY;
    borderBox.x += offsetX;
    borderBox.y += offsetY;
    marginBox.x += offsetX;
    marginBox.y += offsetY;
    
    // Get style properties
    const css::ComputedStyle& style = box->style();
    
    // Draw background
    css::Value bgColorValue = style.getProperty("background-color");
    std::string bgColorStr = bgColorValue.stringValue();
    
    if (!bgColorStr.empty() && bgColorStr != "transparent") {
        unsigned int bgColor = Canvas::rgb(255, 255, 255); // Default white
        
        // Parse color (simplified)
        if (bgColorStr == "black") bgColor = Canvas::rgb(0, 0, 0);
        else if (bgColorStr == "white") bgColor = Canvas::rgb(255, 255, 255);
        else if (bgColorStr == "red") bgColor = Canvas::rgb(255, 0, 0);
        else if (bgColorStr == "green") bgColor = Canvas::rgb(0, 128, 0);
        else if (bgColorStr == "blue") bgColor = Canvas::rgb(0, 0, 255);
        else if (bgColorStr[0] == '#' && bgColorStr.length() >= 7) {
            // Parse hex color
            int r = std::stoi(bgColorStr.substr(1, 2), nullptr, 16);
            int g = std::stoi(bgColorStr.substr(3, 2), nullptr, 16);
            int b = std::stoi(bgColorStr.substr(5, 2), nullptr, 16);
            bgColor = Canvas::rgb(r, g, b);
        } else if (bgColorStr.substr(0, 4) == "rgba") {
            // Parse rgba (simplified)
            size_t start = bgColorStr.find('(') + 1;
            size_t end = bgColorStr.find(')');
            std::string values = bgColorStr.substr(start, end - start);
            
            // Extract r, g, b values (simplified parsing)
            int r = 255, g = 255, b = 255;
            std::istringstream ss(values);
            char comma;
            ss >> r >> comma >> g >> comma >> b;
            bgColor = Canvas::rgb(r, g, b);
        }
        
        // Fill background
        canvas->drawRect(borderBox.x, borderBox.y, borderBox.width, borderBox.height, bgColor, true);
    }
    
    // Draw border
    float borderTop = box->borderTop();
    float borderRight = box->borderRight();
    float borderBottom = box->borderBottom();
    float borderLeft = box->borderLeft();
    
    if (borderTop > 0 || borderRight > 0 || borderBottom > 0 || borderLeft > 0) {
        css::Value borderColorValue = style.getProperty("border-color");
        std::string borderColorStr = borderColorValue.stringValue();
        unsigned int borderColor = Canvas::rgb(0, 0, 0); // Default black
        
        if (!borderColorStr.empty()) {
            // Parse border color (simplified)
            if (borderColorStr == "black") borderColor = Canvas::rgb(0, 0, 0);
            else if (borderColorStr == "gray" || borderColorStr == "grey") borderColor = Canvas::rgb(128, 128, 128);
            else if (borderColorStr[0] == '#' && borderColorStr.length() >= 7) {
                int r = std::stoi(borderColorStr.substr(1, 2), nullptr, 16);
                int g = std::stoi(borderColorStr.substr(3, 2), nullptr, 16);
                int b = std::stoi(borderColorStr.substr(5, 2), nullptr, 16);
                borderColor = Canvas::rgb(r, g, b);
            }
        }
        
        // Draw border as rectangle outline
        int maxBorder = std::max({borderTop, borderRight, borderBottom, borderLeft});
        canvas->drawRect(borderBox.x, borderBox.y, borderBox.width, borderBox.height, borderColor, false, maxBorder);
    }
    
    // Draw text if this is a text box
    if (auto textBox = dynamic_cast<layout::TextBox*>(box)) {
        if (textBox->textNode()) {
            std::string text = textBox->textNode()->nodeValue();
            
            // Get text color
            css::Value colorValue = style.getProperty("color");
            std::string colorStr = colorValue.stringValue();
            unsigned int textColor = Canvas::rgb(0, 0, 0); // Default black
            
            if (!colorStr.empty()) {
                // Parse text color
                if (colorStr == "black") textColor = Canvas::rgb(0, 0, 0);
                else if (colorStr == "white") textColor = Canvas::rgb(255, 255, 255);
                else if (colorStr == "blue") textColor = Canvas::rgb(0, 0, 255);
                else if (colorStr == "red") textColor = Canvas::rgb(255, 0, 0);
                else if (colorStr[0] == '#' && colorStr.length() >= 7) {
                    int r = std::stoi(colorStr.substr(1, 2), nullptr, 16);
                    int g = std::stoi(colorStr.substr(3, 2), nullptr, 16);
                    int b = std::stoi(colorStr.substr(5, 2), nullptr, 16);
                    textColor = Canvas::rgb(r, g, b);
                }
            }
            
            // Get font properties
            css::Value fontSizeValue = style.getProperty("font-size");
            int fontSize = 16; // Default
            if (fontSizeValue.type() == css::ValueType::LENGTH) {
                fontSize = static_cast<int>(fontSizeValue.numericValue());
            }
            
            css::Value fontFamilyValue = style.getProperty("font-family");
            std::string fontFamily = fontFamilyValue.stringValue();
            if (fontFamily.empty()) {
                fontFamily = "Arial";
            }
            
            // Draw text
            canvas->drawText(text, contentRect.x, contentRect.y + fontSize, textColor, fontFamily, fontSize);
        }
    }
    
    // Render children
    for (const auto& child : box->children()) {
        renderBox(canvas, child.get(), offsetX, offsetY);
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
    
    // Draw browser controls (toolbar)
    int toolbarHeight = 40;
    
    // Toolbar background
    canvas->drawRect(0, 0, width, toolbarHeight, Canvas::rgb(240, 240, 240), true);
    canvas->drawRect(0, toolbarHeight, width, 1, Canvas::rgb(200, 200, 200), true); // Border
    
    // Navigation buttons with better visual feedback
    int buttonSize = 30;
    int buttonMargin = 5;
    int buttonY = 5;
    
    // Back button
    bool canGoBack = m_historyIndex > 0;
    unsigned int backButtonColor = canGoBack ? Canvas::rgb(220, 220, 220) : Canvas::rgb(240, 240, 240);
    unsigned int backButtonBorder = canGoBack ? Canvas::rgb(180, 180, 180) : Canvas::rgb(220, 220, 220);
    unsigned int backButtonText = canGoBack ? Canvas::rgb(0, 0, 0) : Canvas::rgb(180, 180, 180);
    
    canvas->drawRect(buttonMargin, buttonY, buttonSize, buttonSize, backButtonColor, true);
    canvas->drawRect(buttonMargin, buttonY, buttonSize, buttonSize, backButtonBorder, false, 1);
    canvas->drawText("◀", buttonMargin + 8, buttonY + 20, backButtonText, "Arial", 16);
    
    // Forward button
    bool canGoForward = m_historyIndex < m_history.size() - 1;
    unsigned int forwardButtonColor = canGoForward ? Canvas::rgb(220, 220, 220) : Canvas::rgb(240, 240, 240);
    unsigned int forwardButtonBorder = canGoForward ? Canvas::rgb(180, 180, 180) : Canvas::rgb(220, 220, 220);
    unsigned int forwardButtonText = canGoForward ? Canvas::rgb(0, 0, 0) : Canvas::rgb(180, 180, 180);
    
    int forwardX = buttonMargin * 2 + buttonSize;
    canvas->drawRect(forwardX, buttonY, buttonSize, buttonSize, forwardButtonColor, true);
    canvas->drawRect(forwardX, buttonY, buttonSize, buttonSize, forwardButtonBorder, false, 1);
    canvas->drawText("▶", forwardX + 8, buttonY + 20, forwardButtonText, "Arial", 16);
    
    // Reload button
    int reloadX = buttonMargin * 3 + buttonSize * 2;
    canvas->drawRect(reloadX, buttonY, buttonSize, buttonSize, Canvas::rgb(220, 220, 220), true);
    canvas->drawRect(reloadX, buttonY, buttonSize, buttonSize, Canvas::rgb(180, 180, 180), false, 1);
    canvas->drawText("↻", reloadX + 8, buttonY + 20, Canvas::rgb(0, 0, 0), "Arial", 16);
    
    // Address bar
    int addressBarX = buttonMargin * 4 + buttonSize * 3;
    int addressBarWidth = width - addressBarX - buttonMargin;
    canvas->drawRect(addressBarX, buttonY, addressBarWidth, buttonSize, Canvas::rgb(255, 255, 255), true);
    canvas->drawRect(addressBarX, buttonY, addressBarWidth, buttonSize, Canvas::rgb(180, 180, 180), false, 1);
    
    // Address text
    if (!m_currentUrl.empty()) {
        canvas->drawText(m_currentUrl, addressBarX + 5, buttonY + 20, Canvas::rgb(0, 0, 0), "Arial", 14);
    } else {
        canvas->drawText("Enter URL...", addressBarX + 5, buttonY + 20, Canvas::rgb(180, 180, 180), "Arial", 14);
    }
    
    // Calculate content area (below toolbar)
    int contentY = toolbarHeight + 1; // +1 for border
    int contentHeight = height - contentY;
    
    // Get the layout root from the browser
    auto layoutRoot = m_browser->layoutRoot();
    
    if (layoutRoot) {
        // We have content to render - render the actual layout tree!
        std::cout << "Rendering page content from layout tree..." << std::endl;
        
        // Create a clipping region for the content area
        // (In a real implementation, you'd set up proper clipping)
        
        // Render the layout tree starting from the root
        renderBox(canvas, layoutRoot.get(), 0, contentY);
        
        // If this is the home page, make sure JavaScript is executed for interactivity
        if (m_currentUrl == "about:home" && m_browser->jsEngine()) {
            // The JavaScript should already be executed when the page was loaded
            // This ensures the clock updates, etc.
        }
        
    } else {
        // No content - this shouldn't happen if about:home loaded correctly
        canvas->drawRect(0, contentY, width, contentHeight, Canvas::rgb(250, 250, 250), true);
        
        std::string message = "Loading page...";
        int textWidth = message.length() * 8;
        int textX = (width - textWidth) / 2;
        int textY = contentY + (contentHeight / 2);
        
        canvas->drawText(message, textX, textY, Canvas::rgb(150, 150, 150), "Arial", 16);
    }
    
    // End painting
    m_window->endPaint();
}

bool BrowserWindow::loadUrl(const std::string& input) {
    std::string url = input;
    
    // Trim whitespace
    size_t start = url.find_first_not_of(" \t\n\r");
    size_t end = url.find_last_not_of(" \t\n\r");
    if (start != std::string::npos && end != std::string::npos) {
        url = url.substr(start, end - start + 1);
    }
    
    // Handle empty input
    if (url.empty()) {
        return false;
    }
    
    // Check if it's a URL or a search query
    bool isUrl = false;
    
    // Check for common URL patterns
    if (url.find("://") != std::string::npos) {
        isUrl = true;
    } else if (url.substr(0, 6) == "about:") {
        isUrl = true;
    } else if (url.find(".") != std::string::npos) {
        // Check if it looks like a domain
        size_t dotPos = url.find(".");
        size_t spacePos = url.find(" ");
        if (dotPos > 0 && dotPos < url.length() - 1 && 
            (spacePos == std::string::npos || dotPos < spacePos)) {
            isUrl = true;
        }
    }
    
    if (!isUrl) {
        // It's a search query - use Google search
        std::string encoded;
        for (char c : url) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += c;
            } else if (c == ' ') {
                encoded += "+";
            } else {
                char hex[4];
                snprintf(hex, sizeof(hex), "%%%02X", (unsigned char)c);
                encoded += hex;
            }
        }
        url = "https://www.google.com/search?q=" + encoded;
    } else if (url.find("://") == std::string::npos && url.substr(0, 6) != "about:") {
        // Add http:// if no protocol specified
        url = "http://" + url;
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
    // Load the about:home page which is defined in browser.cpp
    if (m_browser) {
        std::string error;
        
        // Load the about:home page
        m_browser->loadUrl("about:home", error);
        
        if (!error.empty()) {
            std::cerr << "Error loading default page: " << error << std::endl;
        } else {
            // Perform layout
            int width, height;
            m_window->getSize(width, height);
            
            if (m_browser->layoutEngine() && m_browser->currentDocument() && m_browser->styleResolver()) {
                m_browser->layoutEngine()->layoutDocument(
                    m_browser->currentDocument(),
                    m_browser->styleResolver(),
                    static_cast<float>(width),
                    static_cast<float>(height - 40) // Account for toolbar
                );
            }
        }
        
        // Set initial state
        m_currentUrl = "about:home";
        m_history.clear();
        m_history.push_back(m_currentUrl);
        m_historyIndex = 0;
        
        if (m_browserControls) {
            m_browserControls->setAddressBarText(m_currentUrl);
        }
        
        // Update window title
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
    // Navigation buttons are now properly enabled/disabled based on history
    // The rendering code above handles the visual state
    m_needsRender = true;
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
        // Pass the Key enum value directly
        if (m_browserControls->handleKeyInput(static_cast<int>(key), 0, 
                                             static_cast<int>(action), 0)) {
            m_needsRender = true;  // Force re-render after input
            return;
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
        if (m_browserControls->handleMouseButton(static_cast<int>(button), 
                                               static_cast<int>(action), 0, x, y)) {
            m_needsRender = true;  // Force re-render after click
            return;
        }
    }
    
    // Check if the click is in the toolbar area
    int toolbarHeight = 40;
    if (y <= toolbarHeight && button == MouseButton::Left && action == MouseAction::Press) {
        // Handle toolbar button clicks
        int buttonSize = 30;
        int buttonMargin = 5;
        int buttonY = 5;
        
        // Check back button
        if (x >= buttonMargin && x < buttonMargin + buttonSize &&
            y >= buttonY && y < buttonY + buttonSize) {
            goBack();
            return;
        }
        
        // Check forward button
        int forwardX = buttonMargin * 2 + buttonSize;
        if (x >= forwardX && x < forwardX + buttonSize &&
            y >= buttonY && y < buttonY + buttonSize) {
            goForward();
            return;
        }
        
        // Check reload button
        int reloadX = buttonMargin * 3 + buttonSize * 2;
        if (x >= reloadX && x < reloadX + buttonSize &&
            y >= buttonY && y < buttonY + buttonSize) {
            reload();
            return;
        }
    }
    
    // Check if the click is in the content area
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
    // Don't immediately set initialized to false
    // First check if we should really close
    if (m_window) {
        // Actually close the window
        m_window->close();
        m_window.reset();
    }
    
    // Now set initialized to false
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