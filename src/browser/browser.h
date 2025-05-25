#ifndef BROWSER_BROWSER_H
#define BROWSER_BROWSER_H

#include "../html/html_parser.h"
#include "../css/style_resolver.h"
#include "../layout/layout_engine.h"
#include "../rendering/renderer.h"
#include "../custom_js/js_engine.h"
#include "../networking/resource_loader.h"
#include "../security/security_manager.h"
#include <string>
#include <memory>

namespace browser {

// Browser class - main entry point
class Browser {
public:
    Browser();
    ~Browser();
    
    // Initialize browser components
    bool initialize();
    
    // Load a URL
    bool loadUrl(const std::string& url, std::string& error);
    
    // Get browser components
    html::HTMLParser* htmlParser() { return &m_htmlParser; }
    css::StyleResolver* styleResolver() { return &m_styleResolver; }
    layout::LayoutEngine* layoutEngine() { return &m_layoutEngine; }
    rendering::Renderer* renderer() { return &m_renderer; }
    custom_js::JSEngine* jsEngine() { return &m_jsEngine; }
    networking::ResourceLoader* resourceLoader() { return m_resourceLoader.get(); }
    security::SecurityManager* securityManager() { return m_securityManager.get(); }
    
    // Get current document
    html::Document* currentDocument() const { return m_domTree.document(); }
    
    // Get layout tree root
    std::shared_ptr<layout::Box> layoutRoot() const { return m_layoutEngine.layoutRoot(); }
    
    // Render current page to ASCII art (for terminal viewing)
    std::string renderToASCII(int width, int height);
    
private:
    // Browser components
    html::HTMLParser m_htmlParser;
    css::StyleResolver m_styleResolver;
    layout::LayoutEngine m_layoutEngine;
    rendering::Renderer m_renderer;
    custom_js::JSEngine m_jsEngine;
    std::unique_ptr<networking::ResourceLoader> m_resourceLoader;
    std::unique_ptr<security::SecurityManager> m_securityManager;
    
    // Current document
    html::DOMTree m_domTree;
    
    // Load and process resources
    bool loadStylesheets(const std::string& baseUrl);
    bool loadScripts(const std::string& baseUrl);
    bool loadImages(const std::string& baseUrl);
    
    // Process security headers
    void processSecurityHeaders(const std::map<std::string, std::string>& headers, const std::string& url);
};

} // namespace browser

#endif // BROWSER_BROWSER_H