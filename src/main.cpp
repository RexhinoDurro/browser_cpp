#include "javascript/js_engine.h"
#include "html/html_parser.h"
#include "css/style_resolver.h"
#include "layout/layout_engine.h"
#include "rendering/renderer.h"
#include "networking/resource_loader.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]) {
    // Initialize browser components
    browser::html::HTMLParser parser;
    parser.initialize();
    
    browser::css::StyleResolver styleResolver;
    
    browser::layout::LayoutEngine layoutEngine;
    layoutEngine.initialize();
    
    browser::rendering::Renderer renderer;
    renderer.initialize();
    
    browser::javascript::JSEngine jsEngine;
    jsEngine.initialize();
    
    browser::networking::ResourceLoader resourceLoader;
    resourceLoader.initialize("./browser_cache");
    resourceLoader.start();
    
    // URL to load
    std::string url = "https://example.com";
    
    // Override with command line argument if provided
    if (argc > 1) {
        url = argv[1];
    }
    
    std::cout << "Browser starting - loading: " << url << std::endl;
    
    // Load the page
    std::vector<uint8_t> htmlData;
    std::map<std::string, std::string> headers;
    std::string error;
    
    std::cout << "Fetching HTML..." << std::endl;
    if (!resourceLoader.loadResource(url, htmlData, headers, error)) {
        std::cerr << "Failed to load URL: " << error << std::endl;
        return 1;
    }
    
    // Convert to string
    std::string html(htmlData.begin(), htmlData.end());
    
    // Parse HTML into DOM tree
    std::cout << "Parsing HTML..." << std::endl;
    browser::html::DOMTree domTree = parser.parse(html);
    
    // Get and load CSS resources
    std::cout << "Loading CSS resources..." << std::endl;
    std::vector<std::string> stylesheetLinks = domTree.findStylesheetLinks();
    
    for (const auto& cssUrl : stylesheetLinks) {
        // Resolve relative URL
        std::string fullUrl = url;
        if (cssUrl.find("://") == std::string::npos) {
            // Simple relative URL handling (not comprehensive)
            if (cssUrl[0] == '/') {
                // Absolute path
                size_t pos = url.find("://");
                if (pos != std::string::npos) {
                    pos = url.find('/', pos + 3);
                    if (pos != std::string::npos) {
                        fullUrl = url.substr(0, pos) + cssUrl;
                    } else {
                        fullUrl = url + cssUrl;
                    }
                }
            } else {
                // Relative path
                size_t pos = url.find_last_of('/');
                if (pos != std::string::npos) {
                    fullUrl = url.substr(0, pos + 1) + cssUrl;
                } else {
                    fullUrl = url + "/" + cssUrl;
                }
            }
        } else {
            fullUrl = cssUrl;
        }
        
        std::cout << "Loading CSS: " << fullUrl << std::endl;
        
        std::vector<uint8_t> cssData;
        std::map<std::string, std::string> cssHeaders;
        
        if (resourceLoader.loadResource(fullUrl, cssData, cssHeaders, error)) {
            std::string cssText(cssData.begin(), cssData.end());
            
            // Parse and add CSS
            browser::css::StyleSheet styleSheet;
            styleSheet.parse(cssText);
            styleResolver.addStyleSheet(styleSheet);
        } else {
            std::cerr << "Failed to load CSS: " << error << std::endl;
        }
    }
    
    // Extract and parse inline CSS
    std::vector<std::string> inlineStyles = domTree.findInlineStyles();
    
    for (const auto& cssText : inlineStyles) {
        browser::css::StyleSheet styleSheet;
        styleSheet.parse(cssText);
        styleResolver.addStyleSheet(styleSheet);
    }
    
    // Set the document for style resolver
    styleResolver.setDocument(domTree.document());
    
    // Perform layout calculations
    std::cout << "Calculating layout..." << std::endl;
    layoutEngine.layoutDocument(domTree.document(), &styleResolver, 800, 600);
    
    // Get the layout tree root
    std::shared_ptr<browser::layout::Box> layoutRoot = layoutEngine.layoutRoot();
    
    if (layoutRoot) {
        // Print the layout tree (for debugging)
        std::cout << "\nLayout tree structure:" << std::endl;
        layoutEngine.printLayoutTree(std::cout);
        
        // Render the layout tree to ASCII art
        std::string asciiRender = renderer.renderToASCII(layoutRoot.get(), 80, 30);
        std::cout << "\nASCII Rendering:" << std::endl;
        std::cout << asciiRender << std::endl;
    } else {
        std::cout << "Failed to create layout tree" << std::endl;
    }
    
    // Get and load JavaScript resources
    std::cout << "Loading JavaScript resources..." << std::endl;
    std::vector<std::string> scriptSources = domTree.findScriptSources();
    
    for (const auto& scriptUrl : scriptSources) {
        // Resolve relative URL
        std::string fullUrl = url;
        if (scriptUrl.find("://") == std::string::npos) {
            // Simple relative URL handling (not comprehensive)
            if (scriptUrl[0] == '/') {
                // Absolute path
                size_t pos = url.find("://");
                if (pos != std::string::npos) {
                    pos = url.find('/', pos + 3);
                    if (pos != std::string::npos) {
                        fullUrl = url.substr(0, pos) + scriptUrl;
                    } else {
                        fullUrl = url + scriptUrl;
                    }
                }
            } else {
                // Relative path
                size_t pos = url.find_last_of('/');
                if (pos != std::string::npos) {
                    fullUrl = url.substr(0, pos + 1) + scriptUrl;
                } else {
                    fullUrl = url + "/" + scriptUrl;
                }
            }
        } else {
            fullUrl = scriptUrl;
        }
        
        std::cout << "Loading JavaScript: " << fullUrl << std::endl;
        
        std::vector<uint8_t> jsData;
        std::map<std::string, std::string> jsHeaders;
        
        if (resourceLoader.loadResource(fullUrl, jsData, jsHeaders, error)) {
            std::string jsText(jsData.begin(), jsData.end());
            
            // Execute JavaScript
            std::string result, jsError;
            if (!jsEngine.executeScript(domTree.document(), jsText, result, jsError)) {
                std::cerr << "JavaScript error: " << jsError << std::endl;
            }
        } else {
            std::cerr << "Failed to load JavaScript: " << error << std::endl;
        }
    }
    
    // Re-layout and render after JavaScript modification
    styleResolver.resolveStyles();
    layoutEngine.layoutDocument(domTree.document(), &styleResolver, 800, 600);
    layoutRoot = layoutEngine.layoutRoot();
    
    if (layoutRoot) {
        std::string asciiRender = renderer.renderToASCII(layoutRoot.get(), 80, 30);
        std::cout << "\nUpdated ASCII Rendering after JavaScript:" << std::endl;
        std::cout << asciiRender << std::endl;
    }
    
    // Wait for any pending resource requests to complete
    std::cout << "Waiting for pending resource requests..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Stop the resource loader
    resourceLoader.stop();
    
    return 0;
}