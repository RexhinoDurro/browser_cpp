#include "javascript/js_engine.h"
#include "html/html_parser.h"
#include "css/style_resolver.h"
#include "layout/layout_engine.h"
#include "rendering/renderer.h"
#include <iostream>
#include <string>

int main() {
    // Initialize browser components
    browser::html::HTMLParser parser;
    parser.initialize();
    
    browser::css::StyleResolver styleResolver;
    
    browser::layout::LayoutEngine layoutEngine;
    layoutEngine.initialize();
    
    browser::rendering::Renderer renderer;
    renderer.initialize();
    
    // Parse some HTML
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <title>Layout Test Page</title>
            <style>
                body {
                    margin: 5px;
                    font-family: Arial;
                    font-size: 14px;
                }
                .container {
                    width: 400px;
                    border: 1px solid black;
                    padding: 10px;
                    background-color: #f0f0f0;
                }
                .header {
                    background-color: #007bff;
                    color: white;
                    padding: 5px;
                    margin-bottom: 10px;
                }
                .content {
                    background-color: white;
                    padding: 10px;
                    border: 1px solid #ddd;
                }
                .footer {
                    margin-top: 10px;
                    padding: 5px;
                    background-color: #f8f9fa;
                    border-top: 1px solid #ddd;
                    text-align: center;
                    font-size: 12px;
                }
            </style>
        </head>
        <body>
            <div class="container">
                <div class="header">
                    <h1>Layout Demo</h1>
                </div>
                <div class="content">
                    <p>This is a test of the browser layout engine.</p>
                    <p>We're demonstrating block layout with CSS styling.</p>
                </div>
                <div class="footer">
                    Browser Layout Engine Demo
                </div>
            </div>
        </body>
        </html>
    )";
    
    // Parse HTML into DOM tree
    browser::html::DOMTree domTree = parser.parse(html);
    
    // Extract and parse CSS
    std::vector<std::string> inlineStyles = domTree.findInlineStyles();
    
    // Add styles to the style resolver
    for (const auto& cssText : inlineStyles) {
        browser::css::StyleSheet styleSheet;
        styleSheet.parse(cssText);
        styleResolver.addStyleSheet(styleSheet);
    }
    
    // Set the document for style resolver
    styleResolver.setDocument(domTree.document());
    
    // Perform layout calculations
    layoutEngine.layoutDocument(domTree.document(), &styleResolver, 800, 600);
    
    // Get the layout tree root
    std::shared_ptr<browser::layout::Box> layoutRoot = layoutEngine.layoutRoot();
    
    if (layoutRoot) {
        // Print the layout tree (for debugging)
        std::cout << "Layout tree structure:" << std::endl;
        layoutEngine.printLayoutTree(std::cout);
        
        // Render the layout tree to ASCII art
        std::string asciiRender = renderer.renderToASCII(layoutRoot.get(), 80, 30);
        std::cout << "\nASCII Rendering:" << std::endl;
        std::cout << asciiRender << std::endl;
    } else {
        std::cout << "Failed to create layout tree" << std::endl;
    }
    
    // Initialize JavaScript engine
    browser::javascript::JSEngine jsEngine;
    if (!jsEngine.initialize()) {
        std::cerr << "Failed to initialize JavaScript engine" << std::endl;
        return 1;
    }
    
    // Execute a test script
    std::string script = "document.querySelector('.content').innerHTML = '<p>Content modified by JavaScript!</p>';";
    std::string result, error;
    
    if (!jsEngine.executeScript(domTree.document(), script, result, error)) {
        std::cerr << "JavaScript error: " << error << std::endl;
    } else {
        std::cout << "Script executed successfully!" << std::endl;
        
        // Re-layout and render after JavaScript modification
        styleResolver.resolveStyles();
        layoutEngine.layoutDocument(domTree.document(), &styleResolver, 800, 600);
        layoutRoot = layoutEngine.layoutRoot();
        
        if (layoutRoot) {
            std::string asciiRender = renderer.renderToASCII(layoutRoot.get(), 80, 30);
            std::cout << "\nUpdated ASCII Rendering after JavaScript:" << std::endl;
            std::cout << asciiRender << std::endl;
        }
    }
    
    return 0;
}