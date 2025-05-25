// A simple test in your main.cpp or a test file
#include "browser/browser.h"
#include <iostream>

int main() {
    browser::Browser browser;
    if (!browser.initialize()) {
        std::cerr << "Failed to initialize browser" << std::endl;
        return 1;
    }
    
    std::string script = "var x = 10; var y = 20; x + y;";
    std::string result, error;
    
    if (browser.jsEngine()->executeScript(script, result, error)) {
        std::cout << "Script result: " << result << std::endl;
    } else {
        std::cerr << "Script error: " << error << std::endl;
    }
    
    return 0;
}