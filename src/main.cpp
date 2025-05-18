#include "browser/browser.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]) {
    // Create and initialize browser
    browser::Browser browser;
    
    if (!browser.initialize()) {
        std::cerr << "Failed to initialize browser" << std::endl;
        return 1;
    }
    
    // URL to load
    std::string url = "https://example.com";
    
    // Override with command line argument if provided
    if (argc > 1) {
        url = argv[1];
    }
    
    // Additional security settings if needed
    browser.securityManager()->xssProtection()->setMode(browser::security::XssProtectionMode::ENABLED);
    browser.securityManager()->csrfProtection()->setMode(browser::security::CsrfProtectionMode::SAME_ORIGIN);
    
    // Load the URL
    std::string error;
    if (!browser.loadUrl(url, error)) {
        std::cerr << "Failed to load URL: " << error << std::endl;
        return 1;
    }
    
    // Render the page to ASCII
    std::cout << "\nASCII Rendering:" << std::endl;
    std::cout << browser.renderToASCII(80, 30) << std::endl;
    
    // In a real browser, we would enter an event loop here
    std::cout << "Page loaded successfully. Press Enter to exit." << std::endl;
    std::cin.get();
    
    return 0;
}