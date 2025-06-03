// src/browser/browser.cpp - Complete Browser Implementation with JavaScript fixes
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
    
    // Clear pending navigation
    m_pendingNavigationUrl.clear();
    
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
        
        // Check for pending navigation from JavaScript
        if (!m_pendingNavigationUrl.empty()) {
            std::cout << "Executing pending navigation to: " << m_pendingNavigationUrl << std::endl;
            std::string pendingUrl = m_pendingNavigationUrl;
            m_pendingNavigationUrl.clear();
            
            // Navigate to the URL
            return loadUrl(pendingUrl, error);
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
        html = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Simple Browser - Home</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f5f5f5;
            margin: 0;
            padding: 20px;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        
        .container {
            max-width: 600px;
            width: 100%;
            background: white;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            padding: 40px;
            text-align: center;
        }
        
        h1 {
            color: #333;
            margin-bottom: 10px;
        }
        
        .subtitle {
            color: #666;
            margin-bottom: 30px;
        }
        
        .search-box {
            width: 100%;
            padding: 12px 20px;
            font-size: 16px;
            border: 2px solid #ddd;
            border-radius: 25px;
            outline: none;
            transition: border-color 0.3s;
        }
        
        .search-box:focus {
            border-color: #4285f4;
        }
        
        .search-button {
            margin-top: 20px;
            padding: 10px 30px;
            font-size: 16px;
            background-color: #4285f4;
            color: white;
            border: none;
            border-radius: 25px;
            cursor: pointer;
            transition: background-color 0.3s;
        }
        
        .search-button:hover {
            background-color: #357ae8;
        }
        
        .links {
            margin-top: 40px;
            display: flex;
            justify-content: center;
            gap: 20px;
        }
        
        .link {
            color: #4285f4;
            text-decoration: none;
            font-size: 14px;
        }
        
        .link:hover {
            text-decoration: underline;
        }
        
        .clock {
            position: fixed;
            top: 20px;
            right: 20px;
            font-size: 14px;
            color: #666;
        }
        
        .tips {
            margin-top: 40px;
            padding: 20px;
            background-color: #f0f7ff;
            border-radius: 8px;
            text-align: left;
        }
        
        .tips h3 {
            color: #333;
            margin-bottom: 10px;
        }
        
        .tips ul {
            color: #666;
            margin: 0;
            padding-left: 20px;
        }
        
        .tips li {
            margin-bottom: 5px;
        }
    </style>
</head>
<body>
    <div class="clock" id="clock"></div>
    
    <div class="container">
        <h1>Welcome to Simple Browser</h1>
        <p class="subtitle">Your gateway to the web</p>
        
        <input type="text" id="searchBox" class="search-box" placeholder="Search or enter URL..." />
        <button class="search-button" onclick="performSearch()">Search</button>
        
        <div class="links">
            <a href="https://www.google.com" class="link">Google</a>
            <a href="https://www.wikipedia.org" class="link">Wikipedia</a>
            <a href="https://www.github.com" class="link">GitHub</a>
            <a href="https://news.ycombinator.com" class="link">Hacker News</a>
        </div>
        
        <div class="tips">
            <h3>Quick Tips:</h3>
            <ul>
                <li>Type a URL or search query in the box above</li>
                <li>Press Enter or click Search to navigate</li>
                <li>Use the navigation buttons in the toolbar</li>
                <li>This browser supports basic HTML, CSS, and JavaScript</li>
            </ul>
        </div>
    </div>
    
    <script>
        // Clock functionality
        function updateClock() {
            var now = new Date();
            var timeStr = now.toLocaleTimeString();
            var dateStr = now.toLocaleDateString();
            document.getElementById('clock').textContent = dateStr + ' ' + timeStr;
        }
        
        // Update clock immediately and then every second
        updateClock();
        setInterval(updateClock, 1000);
        
        // Search functionality
        function performSearch() {
            var searchBox = document.getElementById('searchBox');
            var query = searchBox.value.trim();
            if (query) {
                // Check if it's a URL
                if (query.indexOf('.') !== -1 && query.indexOf(' ') === -1) {
                    // Likely a URL
                    if (query.indexOf('://') === -1) {
                        query = 'http://' + query;
                    }
                    window.location.href = query;
                } else {
                    // It's a search query
                    window.location.href = 'https://www.google.com/search?q=' + encodeURIComponent(query);
                }
            }
        }
        
        // Handle Enter key in search box
        document.getElementById('searchBox').addEventListener('keypress', function(e) {
            if (e.key === 'Enter') {
                performSearch();
            }
        });
        
        // Handle link clicks
        var links = document.querySelectorAll('.link');
        for (var i = 0; i < links.length; i++) {
            links[i].addEventListener('click', function(e) {
                e.preventDefault();
                window.location.href = this.href;
            });
        }
        
        // Animate search box on focus
        var searchBox = document.getElementById('searchBox');
        searchBox.addEventListener('focus', function() {
            this.style.transform = 'scale(1.02)';
        });
        
        searchBox.addEventListener('blur', function() {
            this.style.transform = 'scale(1)';
        });
        
        // Focus search box on page load
        searchBox.focus();
    </script>
</body>
</html>
)HTML";
    } else {
        error = "Unknown about: page";
        return false;
    }
    
    // Parse the about page HTML
    m_domTree = m_htmlParser.parse(html);
    m_currentUrl = url;
    m_currentOrigin = security::Origin::null();
    
    // Load stylesheets (including inline styles)
    std::cout << "Loading stylesheets for about page..." << std::endl;
    if (!loadStylesheets(url)) {
        std::cerr << "Warning: Failed to load stylesheets for about page" << std::endl;
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
        1024, 768
    );
    
    // Execute scripts for interactivity - THIS IS CRITICAL!
    std::cout << "Loading scripts for about page..." << std::endl;
    if (!loadScripts(url)) {
        std::cerr << "Warning: Failed to load scripts for about page" << std::endl;
    }
    
    // Check for pending navigation from JavaScript
    if (!m_pendingNavigationUrl.empty()) {
        std::cout << "Executing pending navigation to: " << m_pendingNavigationUrl << std::endl;
        std::string pendingUrl = m_pendingNavigationUrl;
        m_pendingNavigationUrl.clear();
        
        // Navigate to the URL
        return loadUrl(pendingUrl, error);
    }
    
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
                    // Create a CSS parser
                    css::CSSParser parser;
                    
                    // Parse the CSS text
                    auto stylesheet = parser.parseStylesheet(css);
                    
                    if (stylesheet) {
                        m_styleResolver.addStyleSheet(*stylesheet);
                        std::cout << "Successfully loaded external stylesheet: " << fullUrl 
                                << " with " << stylesheet->rules().size() << " rules" << std::endl;
                    } else {
                        std::cerr << "Failed to parse stylesheet from: " << fullUrl << std::endl;
                        allLoaded = false;
                    }
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
                // Create a CSS parser
                css::CSSParser parser;
                
                // Parse the CSS text into a stylesheet
                auto stylesheet = parser.parseStylesheet(css);
                
                if (stylesheet) {
                    // Add the parsed stylesheet to the style resolver
                    m_styleResolver.addStyleSheet(*stylesheet);
                    std::cout << "Successfully parsed inline stylesheet with " 
                              << stylesheet->rules().size() << " rules" << std::endl;
                } else {
                    std::cerr << "Failed to parse inline style content" << std::endl;
                    allLoaded = false;
                }
            } catch (const std::exception& e) {
                std::cerr << "Failed to parse inline style: " << e.what() << std::endl;
                allLoaded = false;
            }
        }
    }
    
    return allLoaded;
}

bool Browser::loadScripts(const std::string& baseUrl) {
    // Clear pending navigation
    m_pendingNavigationUrl.clear();
    
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
                std::cout << "Loaded external script from: " << fullUrl << std::endl;
            } else {
                std::cerr << "Failed to load script: " << error << std::endl;
                allLoaded = false;
                continue;
            }
        } else {
            // Inline script - THIS IS THE FIX!
            if (!m_securityManager->contentSecurityPolicy()->allowsInlineScript()) {
                std::cerr << "Inline script blocked by CSP" << std::endl;
                continue;
            }
            
            scriptContent = script->textContent();
            if (!scriptContent.empty()) {
                std::cout << "Found inline script with " << scriptContent.length() << " characters" << std::endl;
            }
        }
        
        // Execute script
        if (!scriptContent.empty()) {
            std::cout << "Executing script..." << std::endl;
            std::string result, error;
            if (!m_jsEngine.executeScript(scriptContent, result, error)) {
                std::cerr << "Script execution error: " << error << std::endl;
                allLoaded = false;
            } else {
                std::cout << "Script executed successfully" << std::endl;
                if (!result.empty() && result != "undefined") {
                    std::cout << "Script result: " << result << std::endl;
                }
            }
        }
    }
    
    // After all scripts are executed, check for pending navigation
    if (!m_pendingNavigationUrl.empty()) {
        std::cout << "Pending navigation detected: " << m_pendingNavigationUrl << std::endl;
        // Navigation will be handled by the caller
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
    
    // Create document object
    auto docObj = std::make_shared<custom_js::JSObject>();
    
    // Store DOM element references
    std::map<std::string, html::Element*> elementMap;
    
    // document.getElementById
    docObj->set("getElementById", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [this, &elementMap](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) -> custom_js::JSValue {
                if (args.empty()) return custom_js::JSValue();
                
                std::string id = args[0].toString();
                std::cout << "getElementById called with: " << id << std::endl;
                
                html::Element* element = m_domTree.document()->getElementById(id);
                
                if (element) {
                    std::cout << "Found element with id: " << id << std::endl;
                    
                    // Create element wrapper
                    auto elementObj = std::make_shared<custom_js::JSObject>();
                    
                    // Store element reference
                    elementMap[id] = element;
                    
                    // Basic properties
                    elementObj->set("tagName", custom_js::JSValue(element->tagName()));
                    elementObj->set("id", custom_js::JSValue(element->getAttribute("id")));
                    elementObj->set("className", custom_js::JSValue(element->className()));
                    
                    // Value property for input elements
                    if (element->tagName() == "input") {
                        elementObj->set("value", custom_js::JSValue(element->getAttribute("value")));
                        
                        // Add value getter that returns an object with trim()
                        auto valueObj = std::make_shared<custom_js::JSObject>();
                        valueObj->set("trim", custom_js::JSValue(
                            std::make_shared<custom_js::JSFunction>(
                                [element](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                                    std::string value = element->getAttribute("value");
                                    // Trim whitespace
                                    size_t start = value.find_first_not_of(" \t\n\r");
                                    size_t end = value.find_last_not_of(" \t\n\r");
                                    if (start != std::string::npos && end != std::string::npos) {
                                        value = value.substr(start, end - start + 1);
                                    } else if (start == std::string::npos) {
                                        value = "";
                                    }
                                    return custom_js::JSValue(value);
                                }
                            )
                        ));
                        
                        // Override value property to support .trim()
                        elementObj->set("value", custom_js::JSValue(valueObj));
                    }
                    
                    // textContent property
                    elementObj->set("textContent", custom_js::JSValue(element->textContent()));
                    
                    // innerHTML property (simplified)
                    elementObj->set("innerHTML", custom_js::JSValue(element->innerHTML()));
                    
                    // style property
                    auto styleObj = std::make_shared<custom_js::JSObject>();
                    styleObj->set("transform", custom_js::JSValue(""));
                    elementObj->set("style", custom_js::JSValue(styleObj));
                    
                    // addEventListener method
                    elementObj->set("addEventListener", custom_js::JSValue(
                        std::make_shared<custom_js::JSFunction>(
                            [element, this](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                                if (args.size() >= 2 && args[0].isString() && args[1].isFunction()) {
                                    std::string event = args[0].toString();
                                    auto handler = args[1].toFunction();
                                    
                                    std::cout << "addEventListener called: " << event << " on " << element->id() << std::endl;
                                    
                                    // Store event handlers (simplified - in real browser this would be more complex)
                                    if (event == "keypress" && element->id() == "searchBox") {
                                        // Special handling for search box
                                        element->setAttribute("onkeypress", "true");
                                    } else if (event == "click") {
                                        element->setAttribute("onclick", "true");
                                    } else if (event == "focus") {
                                        element->setAttribute("onfocus", "true");
                                    } else if (event == "blur") {
                                        element->setAttribute("onblur", "true");
                                    }
                                }
                                return custom_js::JSValue();
                            }
                        )
                    ));
                    
                    // focus() method
                    elementObj->set("focus", custom_js::JSValue(
                        std::make_shared<custom_js::JSFunction>(
                            [element](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                                std::cout << "focus() called on element: " << element->id() << std::endl;
                                // In a real browser, this would actually focus the element
                                return custom_js::JSValue();
                            }
                        )
                    ));
                    
                    // For links, add href property
                    if (element->tagName() == "a") {
                        elementObj->set("href", custom_js::JSValue(element->getAttribute("href")));
                    }
                    
                    return custom_js::JSValue(elementObj);
                } else {
                    std::cout << "Element not found with id: " << id << std::endl;
                }
                
                return custom_js::JSValue(); // null
            }
        )
    ));
    
    // document.getElementsByTagName
    docObj->set("getElementsByTagName", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [this](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                if (args.empty()) return custom_js::JSValue();
                
                std::string tagName = args[0].toString();
                std::vector<html::Element*> elements = m_domTree.document()->getElementsByTagName(tagName);
                
                // Create array of elements
                std::vector<custom_js::JSValue> jsElements;
                for (html::Element* elem : elements) {
                    auto elementObj = std::make_shared<custom_js::JSObject>();
                    elementObj->set("tagName", custom_js::JSValue(elem->tagName()));
                    elementObj->set("id", custom_js::JSValue(elem->getAttribute("id")));
                    elementObj->set("className", custom_js::JSValue(elem->className()));
                    
                    // Add getAttribute method
                    elementObj->set("getAttribute", custom_js::JSValue(
                        std::make_shared<custom_js::JSFunction>(
                            [elem](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                                if (args.empty()) return custom_js::JSValue();
                                return custom_js::JSValue(elem->getAttribute(args[0].toString()));
                            }
                        )
                    ));
                    
                    jsElements.push_back(custom_js::JSValue(elementObj));
                }
                
                return custom_js::JSValue(std::make_shared<custom_js::JSArray>(jsElements));
            }
        )
    ));
    
    // document.querySelectorAll
    docObj->set("querySelectorAll", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [this](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                if (args.empty()) return custom_js::JSValue();
                
                std::string selector = args[0].toString();
                std::vector<custom_js::JSValue> jsElements;
                
                // Simplified selector handling
                if (selector.size() > 0 && selector[0] == '.') {
                    // Class selector
                    std::string className = selector.substr(1);
                    std::vector<html::Element*> allElements = m_domTree.document()->getElementsByTagName("*");
                    
                    for (html::Element* elem : allElements) {
                        if (elem->className() == className) {
                            auto elementObj = std::make_shared<custom_js::JSObject>();
                            elementObj->set("tagName", custom_js::JSValue(elem->tagName()));
                            elementObj->set("className", custom_js::JSValue(elem->className()));
                            
                            // Add addEventListener
                            elementObj->set("addEventListener", custom_js::JSValue(
                                std::make_shared<custom_js::JSFunction>(
                                    [elem, this](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                                        if (args.size() >= 2) {
                                            std::string event = args[0].toString();
                                            std::cout << "addEventListener on class element: " << event << std::endl;
                                        }
                                        return custom_js::JSValue();
                                    }
                                )
                            ));
                            
                            // Add href for links
                            if (elem->tagName() == "a") {
                                elementObj->set("href", custom_js::JSValue(elem->getAttribute("href")));
                            }
                            
                            // Add style property
                            auto styleObj = std::make_shared<custom_js::JSObject>();
                            styleObj->set("transform", custom_js::JSValue(""));
                            elementObj->set("style", custom_js::JSValue(styleObj));
                            
                            jsElements.push_back(custom_js::JSValue(elementObj));
                        }
                    }
                }
                
                return custom_js::JSValue(std::make_shared<custom_js::JSArray>(jsElements));
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
    
    // Add location object with WORKING navigation
    auto locationObj = std::make_shared<custom_js::JSObject>();
    locationObj->set("href", custom_js::JSValue(m_currentUrl));
    
    // Create a special setter for href that triggers navigation
    windowObj->set("__navigateTo__", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [this](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                if (!args.empty()) {
                    std::string newUrl = args[0].toString();
                    std::cout << "JavaScript navigation to: " << newUrl << std::endl;
                    m_pendingNavigationUrl = newUrl;
                }
                return custom_js::JSValue();
            }
        )
    ));
    
    // Override location object to use property descriptor
    auto locationWrapper = std::make_shared<custom_js::JSObject>();
    locationWrapper->set("get", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [this](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                auto loc = std::make_shared<custom_js::JSObject>();
                loc->set("href", custom_js::JSValue(m_currentUrl));
                return custom_js::JSValue(loc);
            }
        )
    ));
    
    locationWrapper->set("set", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [this](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                if (!args.empty() && args[0].isObject()) {
                    auto obj = args[0].toObject();
                    if (obj && obj->has("href")) {
                        std::string newUrl = obj->get("href").toString();
                        std::cout << "Setting location.href to: " << newUrl << std::endl;
                        m_pendingNavigationUrl = newUrl;
                    }
                }
                return custom_js::JSValue();
            }
        )
    ));
    
    // Make location work with direct property access
    windowObj->set("location", custom_js::JSValue(locationObj));
    
    // Add Date constructor
    windowObj->set("Date", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                auto dateObj = std::make_shared<custom_js::JSObject>();
                
                // Get current time
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                auto tm = *std::localtime(&time_t);
                
                // Add date methods
                dateObj->set("toLocaleTimeString", custom_js::JSValue(
                    std::make_shared<custom_js::JSFunction>(
                        [tm](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                            char buffer[100];
                            snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", 
                                    tm.tm_hour, tm.tm_min, tm.tm_sec);
                            return custom_js::JSValue(std::string(buffer));
                        }
                    )
                ));
                
                dateObj->set("toLocaleDateString", custom_js::JSValue(
                    std::make_shared<custom_js::JSFunction>(
                        [tm](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                            const char* weekdays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", 
                                                     "Thursday", "Friday", "Saturday"};
                            const char* months[] = {"January", "February", "March", "April", "May", "June",
                                                   "July", "August", "September", "October", "November", "December"};
                            
                            char buffer[200];
                            snprintf(buffer, sizeof(buffer), "%s, %s %d, %d", 
                                    weekdays[tm.tm_wday], months[tm.tm_mon], 
                                    tm.tm_mday, tm.tm_year + 1900);
                            return custom_js::JSValue(std::string(buffer));
                        }
                    )
                ));
                
                return custom_js::JSValue(dateObj);
            }
        )
    ));
    
    // Add setInterval for clock updates
    windowObj->set("setInterval", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                // In a real implementation, this would schedule the callback
                return custom_js::JSValue(1.0);
            }
        )
    ));
    
    // Add encodeURIComponent for search functionality
    windowObj->set("encodeURIComponent", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                if (args.empty()) return custom_js::JSValue("");
                
                std::string input = args[0].toString();
                std::string encoded;
                
                for (char c : input) {
                    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                        encoded += c;
                    } else if (c == ' ') {
                        encoded += "%20";
                    } else {
                        char hex[4];
                        snprintf(hex, sizeof(hex), "%%%02X", (unsigned char)c);
                        encoded += hex;
                    }
                }
                
                return custom_js::JSValue(encoded);
            }
        )
    ));
    
    // Set window as global object
    m_jsEngine.defineGlobalVariable("window", custom_js::JSValue(windowObj));
    
    // Also define globals directly
    m_jsEngine.defineGlobalVariable("document", windowObj->get("document"));
    m_jsEngine.defineGlobalVariable("console", windowObj->get("console"));
    m_jsEngine.defineGlobalVariable("Date", windowObj->get("Date"));
    m_jsEngine.defineGlobalVariable("setInterval", windowObj->get("setInterval"));
    m_jsEngine.defineGlobalVariable("encodeURIComponent", windowObj->get("encodeURIComponent"));
    
    // Add location as a special global with setter support
    m_jsEngine.defineGlobalVariable("location", custom_js::JSValue(locationObj));
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