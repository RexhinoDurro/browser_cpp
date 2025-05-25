// src/browser/browser.cpp - Complete Browser Implementation
#include "browser.h"
#include "../networking/http_client.h"
#include "../storage/local_storage.h"
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <chrono>

namespace browser {

Browser::Browser() 
    : m_resourceLoader(std::make_unique<networking::ResourceLoader>())
    , m_securityManager(std::make_unique<security::SecurityManager>()) {
}

Browser::~Browser() {
    // Cleanup in reverse order of initialization
}

bool Browser::initialize() {
    std::cout << "Initializing browser components..." << std::endl;
    
    // Initialize HTML parser
    if (!m_htmlParser.initialize()) {
        std::cerr << "Failed to initialize HTML parser" << std::endl;
        return false;
    }
    
    // Initialize CSS style resolver
    if (!m_styleResolver.initialize()) {
        std::cerr << "Failed to initialize style resolver" << std::endl;
        return false;
    }
    
    // Initialize JavaScript engine
    if (!m_jsEngine.initialize()) {
        std::cerr << "Failed to initialize JavaScript engine" << std::endl;
        return false;
    }
    
    // Initialize layout engine
    if (!m_layoutEngine.initialize()) {
        std::cerr << "Failed to initialize layout engine" << std::endl;
        return false;
    }
    
    // Initialize renderer
    if (!m_renderer.initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }
    
    // Initialize resource loader
    if (!m_resourceLoader->initialize()) {
        std::cerr << "Failed to initialize resource loader" << std::endl;
        return false;
    }
    
    // Initialize security manager
    if (!m_securityManager->initialize()) {
        std::cerr << "Failed to initialize security manager" << std::endl;
        return false;
    }
    
    // Start resource loader thread
    m_resourceLoader->start();
    
    // Set up JavaScript bindings
    setupJavaScriptBindings();
    
    std::cout << "Browser components initialized successfully" << std::endl;
    return true;
}

bool Browser::loadUrl(const std::string& url, std::string& error) {
    std::cout << "Loading URL: " << url << std::endl;
    
    try {
        // Special handling for about: URLs
        if (url.substr(0, 6) == "about:") {
            return loadAboutPage(url, error);
        }
        
        // Parse URL to get origin
        security::Origin targetOrigin(url);
        
        // Security check for navigation
        if (m_currentOrigin.toString() != "null") {
            if (!m_securityManager->canMakeRequest(m_currentOrigin, targetOrigin, "GET", "")) {
                error = "Navigation blocked by security policy";
                return false;
            }
        }
        
        // Load the resource
        std::vector<uint8_t> data;
        std::map<std::string, std::string> headers;
        
        if (!m_resourceLoader->loadResource(url, data, headers, error)) {
            return false;
        }
        
        // Process security headers
        processSecurityHeaders(headers, url);
        
        // Convert data to string (assuming UTF-8)
        std::string html(data.begin(), data.end());
        
        // Parse HTML
        std::cout << "Parsing HTML..." << std::endl;
        m_domTree = m_htmlParser.parse(html);
        
        if (!m_domTree.document()) {
            error = "Failed to parse HTML";
            return false;
        }
        
        // Update current URL and origin
        m_currentUrl = url;
        m_currentOrigin = targetOrigin;
        
        // Load stylesheets
        std::cout << "Loading stylesheets..." << std::endl;
        if (!loadStylesheets(url)) {
            std::cerr << "Warning: Some stylesheets failed to load" << std::endl;
        }
        
        // Resolve styles
        std::cout << "Resolving styles..." << std::endl;
        m_styleResolver.setDocument(m_domTree.document());
        m_styleResolver.resolveStyles();
        
        // Calculate layout
        std::cout << "Calculating layout..." << std::endl;
        m_layoutEngine.layoutDocument(
            m_domTree.document(),
            &m_styleResolver,
            1024, // Default viewport width
            768   // Default viewport height
        );
        
        // Execute scripts
        std::cout << "Executing scripts..." << std::endl;
        if (!loadScripts(url)) {
            std::cerr << "Warning: Some scripts failed to load" << std::endl;
        }
        
        // Load images (asynchronously)
        loadImages(url);
        
        std::cout << "Page loaded successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        error = "Exception while loading page: " + std::string(e.what());
        return false;
    }
}

bool Browser::loadAboutPage(const std::string& url, std::string& error) {
    std::string html;
    
    if (url == "about:home" || url == "about:blank") {
        html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Simple Browser - Home</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 40px;
            background-color: #f0f0f0;
            color: #333;
        }
        h1 {
            color: #2c3e50;
            text-align: center;
        }
        .search-box {
            width: 100%;
            max-width: 600px;
            margin: 20px auto;
            display: block;
            padding: 10px;
            font-size: 16px;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
        .links {
            text-align: center;
            margin-top: 40px;
        }
        .links a {
            margin: 0 10px;
            color: #3498db;
            text-decoration: none;
        }
        .links a:hover {
            text-decoration: underline;
        }
    </style>
</head>
<body>
    <h1>Welcome to Simple Browser</h1>
    <input type="text" class="search-box" placeholder="Enter URL or search..." id="searchBox">
    <div class="links">
        <a href="https://www.google.com">Google</a>
        <a href="https://www.github.com">GitHub</a>
        <a href="https://www.wikipedia.org">Wikipedia</a>
        <a href="about:version">About</a>
    </div>
    <script>
        document.getElementById('searchBox').addEventListener('keypress', function(e) {
            if (e.key === 'Enter') {
                var value = this.value.trim();
                if (value.includes('.') || value.startsWith('http')) {
                    window.location.href = value;
                } else {
                    window.location.href = 'https://www.google.com/search?q=' + encodeURIComponent(value);
                }
            }
        });
    </script>
</body>
</html>
)";
    } else if (url == "about:version") {
        html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>About Simple Browser</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 40px;
            background-color: #fff;
        }
        h1 { color: #333; }
        .info { background: #f0f0f0; padding: 20px; border-radius: 5px; }
        .component { margin: 10px 0; }
    </style>
</head>
<body>
    <h1>Simple Browser</h1>
    <div class="info">
        <div class="component"><strong>Version:</strong> 1.0.0</div>
        <div class="component"><strong>Engine:</strong> Custom</div>
        <div class="component"><strong>JavaScript:</strong> Custom ES5 Interpreter</div>
        <div class="component"><strong>CSS:</strong> CSS2.1 (partial CSS3)</div>
        <div class="component"><strong>HTML:</strong> HTML5 Parser</div>
    </div>
    <p>A web browser built from scratch for educational purposes.</p>
</body>
</html>
)";
    } else {
        error = "Unknown about: page";
        return false;
    }
    
    // Parse the about page HTML
    m_domTree = m_htmlParser.parse(html);
    m_currentUrl = url;
    m_currentOrigin = security::Origin::null();
    
    // Resolve styles
    m_styleResolver.setDocument(m_domTree.document());
    m_styleResolver.resolveStyles();
    
    // Calculate layout
    m_layoutEngine.layoutDocument(
        m_domTree.document(),
        &m_styleResolver,
        1024, 768
    );
    
    return true;
}

std::string Browser::resolveUrl(const std::string& baseUrl, const std::string& relativeUrl) {
    // Check if already absolute
    if (relativeUrl.find("://") != std::string::npos) {
        return relativeUrl;
    }
    
    try {
        // Parse base URL
        size_t schemeEnd = baseUrl.find("://");
        if (schemeEnd == std::string::npos) {
            return relativeUrl; // Invalid base URL
        }
        
        std::string scheme = baseUrl.substr(0, schemeEnd + 3);
        std::string baseWithoutScheme = baseUrl.substr(schemeEnd + 3);
        
        // Handle absolute paths
        if (relativeUrl[0] == '/') {
            size_t pathStart = baseWithoutScheme.find('/');
            if (pathStart == std::string::npos) {
                return scheme + baseWithoutScheme + relativeUrl;
            } else {
                return scheme + baseWithoutScheme.substr(0, pathStart) + relativeUrl;
            }
        } 
        
        // Handle relative paths
        size_t lastSlash = baseUrl.find_last_of('/');
        if (lastSlash == std::string::npos || lastSlash < schemeEnd + 3) {
            return baseUrl + "/" + relativeUrl;
        } else {
            return baseUrl.substr(0, lastSlash + 1) + relativeUrl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error resolving URL: " << e.what() << std::endl;
        return relativeUrl;
    }
}

bool Browser::loadStylesheets(const std::string& baseUrl) {
    auto links = m_domTree.document()->getElementsByTagName("link");
    bool allLoaded = true;
    
    for (html::Element* link : links) {
        if (link->getAttribute("rel") == "stylesheet") {
            std::string href = link->getAttribute("href");
            if (href.empty()) continue;
            
            std::string fullUrl = resolveUrl(baseUrl, href);
            
            // Check CSP
            if (!m_securityManager->isAllowedByCSP(fullUrl, security::CspResourceType::STYLE, m_currentOrigin)) {
                std::cerr << "Stylesheet blocked by CSP: " << fullUrl << std::endl;
                continue;
            }
            
            // Load stylesheet
            std::vector<uint8_t> data;
            std::map<std::string, std::string> headers;
            std::string error;
            
            if (m_resourceLoader->loadResource(fullUrl, data, headers, error)) {
                std::string css(data.begin(), data.end());
                
                // Parse and add stylesheet
                try {
                    auto stylesheet = std::make_shared<css::StyleSheet>();
                    // Note: In a real implementation, you'd parse the CSS here
                    m_styleResolver.addStyleSheet(*stylesheet);
                } catch (const std::exception& e) {
                    std::cerr << "Failed to parse stylesheet: " << e.what() << std::endl;
                    allLoaded = false;
                }
            } else {
                std::cerr << "Failed to load stylesheet: " << error << std::endl;
                allLoaded = false;
            }
        }
    }
    
    // Also process inline styles
    auto styles = m_domTree.document()->getElementsByTagName("style");
    for (html::Element* style : styles) {
        std::string css = style->textContent();
        if (!css.empty()) {
            try {
                auto stylesheet = std::make_shared<css::StyleSheet>();
                // Note: In a real implementation, you'd parse the CSS here
                m_styleResolver.addStyleSheet(*stylesheet);
            } catch (const std::exception& e) {
                std::cerr << "Failed to parse inline style: " << e.what() << std::endl;
                allLoaded = false;
            }
        }
    }
    
    return allLoaded;
}

bool Browser::loadScripts(const std::string& baseUrl) {
    auto scripts = m_domTree.document()->getElementsByTagName("script");
    bool allLoaded = true;
    
    for (html::Element* script : scripts) {
        std::string src = script->getAttribute("src");
        std::string scriptContent;
        
        if (!src.empty()) {
            // External script
            std::string fullUrl = resolveUrl(baseUrl, src);
            
            // Check CSP
            if (!m_securityManager->isAllowedByCSP(fullUrl, security::CspResourceType::SCRIPT, m_currentOrigin)) {
                std::cerr << "Script blocked by CSP: " << fullUrl << std::endl;
                continue;
            }
            
            // Load script
            std::vector<uint8_t> data;
            std::map<std::string, std::string> headers;
            std::string error;
            
            if (m_resourceLoader->loadResource(fullUrl, data, headers, error)) {
                scriptContent = std::string(data.begin(), data.end());
            } else {
                std::cerr << "Failed to load script: " << error << std::endl;
                allLoaded = false;
                continue;
            }
        } else {
            // Inline script
            if (!m_securityManager->contentSecurityPolicy()->allowsInlineScript()) {
                std::cerr << "Inline script blocked by CSP" << std::endl;
                continue;
            }
            
            scriptContent = script->textContent();
        }
        
        // Execute script
        if (!scriptContent.empty()) {
            std::string result, error;
            if (!m_jsEngine.executeScript(scriptContent, result, error)) {
                std::cerr << "Script execution error: " << error << std::endl;
                allLoaded = false;
            }
        }
    }
    
    return allLoaded;
}

bool Browser::loadImages(const std::string& baseUrl) {
    auto images = m_domTree.document()->getElementsByTagName("img");
    
    for (html::Element* img : images) {
        std::string src = img->getAttribute("src");
        if (src.empty()) continue;
        
        std::string fullUrl = resolveUrl(baseUrl, src);
        
        // Check CSP
        if (!m_securityManager->isAllowedByCSP(fullUrl, security::CspResourceType::IMG, m_currentOrigin)) {
            std::cerr << "Image blocked by CSP: " << fullUrl << std::endl;
            continue;
        }
        
        // Queue image loading asynchronously
        auto request = std::make_shared<networking::ResourceRequest>(fullUrl, networking::ResourceType::IMAGE);
        request->setCompletionCallback(
            [img, fullUrl](const std::vector<uint8_t>& data, const std::map<std::string, std::string>& headers) {
                // In a real implementation, you would decode the image and store it
                std::cout << "Image loaded: " << fullUrl << " (" << data.size() << " bytes)" << std::endl;
            }
        );
        
        m_resourceLoader->queueRequest(request);
    }
    
    return true;
}

void Browser::processSecurityHeaders(const std::map<std::string, std::string>& headers, const std::string& url) {
    // Process Content-Security-Policy
    auto cspIt = headers.find("Content-Security-Policy");
    if (cspIt != headers.end()) {
        m_securityManager->contentSecurityPolicy()->parse(cspIt->second);
    }
    
    // Process X-XSS-Protection
    auto xssIt = headers.find("X-XSS-Protection");
    if (xssIt != headers.end()) {
        m_securityManager->xssProtection()->parseHeader(xssIt->second);
    }
    
    // Process X-Frame-Options
    auto frameIt = headers.find("X-Frame-Options");
    if (frameIt != headers.end()) {
        // Handle frame options
    }
    
    // Process Strict-Transport-Security
    auto hstsIt = headers.find("Strict-Transport-Security");
    if (hstsIt != headers.end()) {
        // Handle HSTS
    }
}

void Browser::setupJavaScriptBindings() {
    // Create window object
    auto windowObj = std::make_shared<custom_js::JSObject>();
    
    // Add document object
    auto docObj = std::make_shared<custom_js::JSObject>();
    
    // document.getElementById
    docObj->set("getElementById", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [this](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                if (args.empty()) return custom_js::JSValue();
                
                std::string id = args[0].toString();
                html::Element* element = m_domTree.document()->getElementById(id);
                
                if (element) {
                    // Create element wrapper
                    auto elementObj = std::make_shared<custom_js::JSObject>();
                    
                    // Add element properties
                    elementObj->set("tagName", custom_js::JSValue(element->tagName()));
                    elementObj->set("id", custom_js::JSValue(element->getAttribute("id")));
                    
                    // Add textContent property
                    elementObj->set("textContent", custom_js::JSValue(element->textContent()));
                    
                    return custom_js::JSValue(elementObj);
                }
                
                return custom_js::JSValue(); // null
            }
        )
    ));
    
    windowObj->set("document", custom_js::JSValue(docObj));
    
    // Add console object
    auto consoleObj = std::make_shared<custom_js::JSObject>();
    
    // console.log
    consoleObj->set("log", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                std::cout << "[Console] ";
                for (const auto& arg : args) {
                    std::cout << arg.toString() << " ";
                }
                std::cout << std::endl;
                return custom_js::JSValue();
            }
        )
    ));
    
    windowObj->set("console", custom_js::JSValue(consoleObj));
    
    // Add location object
    auto locationObj = std::make_shared<custom_js::JSObject>();
    locationObj->set("href", custom_js::JSValue(m_currentUrl));
    
    windowObj->set("location", custom_js::JSValue(locationObj));
    
    // Set window as global object
    m_jsEngine.defineGlobalVariable("window", custom_js::JSValue(windowObj));
}

std::string Browser::renderToASCII(int width, int height) {
    auto layoutRoot = m_layoutEngine.layoutRoot();
    if (!layoutRoot) {
        return "No page loaded\n";
    }
    
    // Use the renderer to create ASCII output
    return m_renderer.renderToASCII(layoutRoot.get(), width, height);
}

} // namespace browser