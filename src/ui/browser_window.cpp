// src/ui/browser_window.cpp - Browser window implementation

#include "window.h"
#include <iostream>
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
    
    m_initialized = true;
    return true;
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

// (Removed incomplete function definition)