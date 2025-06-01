// simple_browser_test.cpp - Minimal test to debug browser window issues
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "browser/browser.h"
#include "ui/browser_window.h"
#include "rendering/renderer.h"
#include "rendering/paint_system.h"

using namespace browser;
using namespace browser::ui;
using namespace browser::rendering;

// Simple test that creates a browser window and loads a basic page
int main(int argc, char* argv[]) {
    std::cout << "Simple Browser Window Test" << std::endl;
    std::cout << "=========================" << std::endl;
    
    try {
        // Step 1: Create browser instance
        std::cout << "\n1. Creating browser instance..." << std::endl;
        auto browser = std::make_shared<browser::Browser>();
        
        // Step 2: Initialize browser
        std::cout << "2. Initializing browser engine..." << std::endl;
        if (!browser->initialize()) {
            std::cerr << "ERROR: Failed to initialize browser engine" << std::endl;
            return 1;
        }
        std::cout << "   Browser engine initialized successfully" << std::endl;
        
        // Step 3: Create browser window
        std::cout << "\n3. Creating browser window..." << std::endl;
        WindowConfig config;
        config.title = "Simple Browser Test";
        config.width = 1024;
        config.height = 768;
        config.resizable = true;
        
        auto browserWindow = std::make_shared<BrowserWindow>(config);
        
        // Step 4: Set browser
        std::cout << "4. Setting browser instance..." << std::endl;
        browserWindow->setBrowser(browser);
        
        // Step 5: Initialize window
        std::cout << "5. Initializing browser window..." << std::endl;
        if (!browserWindow->initialize()) {
            std::cerr << "ERROR: Failed to initialize browser window" << std::endl;
            return 1;
        }
        std::cout << "   Browser window initialized successfully" << std::endl;
        
        // Step 6: Show window
        std::cout << "\n6. Showing browser window..." << std::endl;
        browserWindow->show();
        
        // Step 7: Load a simple test page
        std::cout << "7. Loading test page..." << std::endl;
        std::string testHtml = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Test Page</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            background-color: #f0f0f0;
        }
        h1 {
            color: #333;
            background-color: #fff;
            padding: 10px;
            border: 2px solid #333;
        }
        .box {
            width: 200px;
            height: 100px;
            background-color: #4CAF50;
            margin: 20px;
            padding: 20px;
            color: white;
            text-align: center;
        }
    </style>
</head>
<body>
    <h1>Browser Test Page</h1>
    <p>If you can see this, the browser is working!</p>
    <div class="box">
        This is a test box
    </div>
</body>
</html>
)";
        
        // Try loading the test HTML directly
        std::string error;
        
        // First try to parse the HTML
        std::cout << "   Parsing HTML..." << std::endl;
        html::DOMTree domTree = browser->htmlParser()->parse(testHtml);
        if (!domTree.document()) {
            std::cerr << "ERROR: Failed to parse HTML" << std::endl;
            return 1;
        }
        std::cout << "   HTML parsed successfully" << std::endl;
        
        // Now try to load about:home to see if that works
        std::cout << "   Loading about:home..." << std::endl;
        if (!browser->loadUrl("about:home", error)) {
            std::cerr << "ERROR: Failed to load about:home - " << error << std::endl;
            // Continue anyway to see if window shows
        } else {
            std::cout << "   about:home loaded successfully" << std::endl;
        }
        
        // Step 8: Run event loop for a short time
        std::cout << "\n8. Running event loop (window should be visible)..." << std::endl;
        std::cout << "   Window will close automatically in 5 seconds" << std::endl;
        
        auto start = std::chrono::steady_clock::now();
        std::cout << "\n8. Running event loop (close window to exit)..." << std::endl;

        int frameCount = 0;

        // Run until window is closed by user
        while (browserWindow->isOpen()) {
            // Process events
            browserWindow->processEvents();
            
            // Print frame count every second
            frameCount++;
            if (frameCount % 60 == 0) {
                std::cout << "   Frame " << frameCount << " - Window is open" << std::endl;
            }
            
            // Sleep to avoid consuming too much CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }

        std::cout << "   Window closed by user" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\nEXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\nUNKNOWN EXCEPTION" << std::endl;
        return 1;
    }
    
    return 0;
}

// Additional test to check if we can create a simple window without the browser
void testSimpleWindow() {
    std::cout << "\n\nTesting Simple Window (no browser)..." << std::endl;
    std::cout << "====================================" << std::endl;
    
    WindowConfig config;
    config.title = "Simple Window Test";
    config.width = 800;
    config.height = 600;
    
    auto window = createPlatformWindow(config);
    if (!window) {
        std::cerr << "Failed to create platform window" << std::endl;
        return;
    }
    
    if (!window->create()) {
        std::cerr << "Failed to initialize window" << std::endl;
        return;
    }
    
    window->show();
    
    // Set up paint callback
    bool needsPaint = true;
    
    // Process events for 3 seconds
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(3)) {
        window->processEvents();
        
        if (needsPaint) {
            window->beginPaint();
            Canvas* canvas = window->getCanvas();
            
            if (canvas) {
                // Clear background
                canvas->clear(Canvas::rgb(240, 240, 240));
                
                // Draw test content
                canvas->drawText("Simple Window Test", 50, 50, Canvas::rgb(0, 0, 0), "Arial", 24);
                canvas->drawRect(50, 100, 200, 100, Canvas::rgb(255, 0, 0), true);
                canvas->drawRect(300, 100, 200, 100, Canvas::rgb(0, 255, 0), false, 3);
            }
            
            window->endPaint();
            needsPaint = false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    window->close();
    std::cout << "Simple window test completed" << std::endl;
}