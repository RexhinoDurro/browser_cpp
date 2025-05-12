#include "javascript/js_engine.h"
#include "html/html_parser.h"
#include <iostream>

int main() {
    // Initialize browser components
    browser::html::HTMLParser parser;
    parser.initialize();
    
    // Parse some HTML
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Test Page</title>
        </head>
        <body>
            <div id="output">Initial content</div>
            <script>
                document.getElementById('output').innerHTML = 'Modified by JavaScript!';
                console.log('JavaScript executed successfully');
            </script>
        </body>
        </html>
    )";
    
    browser::html::DOMTree domTree = parser.parse(html);
    
    // Initialize JavaScript engine
    browser::javascript::JSEngine jsEngine;
    if (!jsEngine.initialize()) {
        std::cerr << "Failed to initialize JavaScript engine" << std::endl;
        return 1;
    }
    
    // Execute a test script
    std::string script = "document.getElementById('output').innerHTML = 'Hello from JavaScript!';";
    std::string result, error;
    if (!jsEngine.executeScript(domTree.document(), script, result, error)) {
        std::cerr << "JavaScript error: " << error << std::endl;
    } else {
        std::cout << "Script executed successfully, result: " << result << std::endl;
    }
    
    // Print the HTML
    std::cout << "Final HTML: " << domTree.toHTML() << std::endl;
    
    return 0;
}