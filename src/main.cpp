#include "browser/browser.h"
#include "ui/window.h"
#include "ui/controls.h"
#include <iostream>
#include <string>
#include <memory>

int main(int argc, char* argv[]) {
    // Create and initialize browser window
    browser::ui::WindowConfig config;
    config.title = "Simple Browser";
    config.width = 1024;
    config.height = 768;
    config.maximized = true;
    
    auto window = std::make_shared<browser::ui::BrowserWindow>(config);
    
    if (!window->initialize()) {
        std::cerr << "Failed to initialize browser window" << std::endl;
        return 1;
    }
    
    // Create and initialize browser controls
    auto controls = std::make_shared<browser::ui::BrowserControls>(window.get());
    
    if (!controls->initialize()) {
        std::cerr << "Failed to initialize browser controls" << std::endl;
        return 1;
    }
    
    // URL to load
    std::string url = "https://example.com";
    
    // Override with command line argument if provided
    if (argc > 1) {
        url = argv[1];
    }
    
    // Set URL in address bar
    controls->setAddressBarText(url);
    
    // Show browser window
    window->show();
    
    // Set up window callbacks
    window->setUrlChangeCallback([&controls](const std::string& url) {
        controls->setAddressBarText(url);
    });
    
    window->setLoadingStateCallback([&controls](bool loading) {
        controls->setLoading(loading);
    });
    
    // Load initial URL
    window->loadUrl(url);
    
    // Main event loop
    while (window->isOpen()) {
        // Process window events
        window->processEvents();
        
        // Draw browser controls
        controls->draw();
        
        // Limit frame rate (approx. 60 FPS)
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    return 0;
}