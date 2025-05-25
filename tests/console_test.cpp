#include "../src/browser/browser.h"
#include <iostream>
#include <string>
int main(int argc, char* argv[]) {
std::cout << "Browser Engine Console Test" << std::endl;
std::cout << "============================" << std::endl;
// Create and initialize browser
browser::Browser browserEngine;

std::cout << "Initializing browser engine..." << std::endl;
if (!browserEngine.initialize()) {
    std::cerr << "Failed to initialize browser engine" << std::endl;
    return 1;
}

std::cout << "Browser engine initialized successfully!" << std::endl;

// Test with a simple HTML page
std::string testUrl = "https://example.com";
if (argc > 1) {
    testUrl = argv[1];
}

std::cout << "Loading URL: " << testUrl << std::endl;

std::string error;
bool result = browserEngine.loadUrl(testUrl, error);

if (result) {
    std::cout << "Page loaded successfully!" << std::endl;
    
    // Render to ASCII for console output
    std::cout << "\nRendered output (80x25):" << std::endl;
    std::cout << std::string(82, '=') << std::endl;
    
    std::string ascii = browserEngine.renderToASCII(80, 25);
    std::cout << ascii << std::endl;
    
    std::cout << std::string(82, '=') << std::endl;
    
    // Print document title if available
    if (browserEngine.currentDocument()) {
        std::string title = browserEngine.currentDocument()->title();
        if (!title.empty()) {
            std::cout << "Page title: " << title << std::endl;
        }
    }
} else {
    std::cerr << "Failed to load URL: " << error << std::endl;
    return 1;
}

return 0;
}
