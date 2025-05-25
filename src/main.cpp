// src/main.cpp
#include "ui/browser_window.h"
#include <iostream>

int main(int argc, char* argv[]) {
    // Create browser window configuration
    browser::ui::WindowConfig config;
    config.title = "Simple Browser";
    config.width = 1024;
    config.height = 768;
    config.resizable = true;
    
    // Create browser window
    browser::ui::BrowserWindow browserWindow(config);
    
    // Initialize browser
    if (!browserWindow.initialize()) {
        std::cerr << "Failed to initialize browser window" << std::endl;
        return 1;
    }
    
    // Show the browser window
    browserWindow.show();
    
    // Load a URL if provided as command line argument
    if (argc > 1) {
        browserWindow.loadUrl(argv[1]);
    }
    
    // Run the event loop
    browserWindow.runEventLoop();
    
    return 0;
}