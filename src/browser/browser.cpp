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
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: #fff;
            min-height: 100vh;
            overflow-x: hidden;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 40px 20px;
        }
        
        .header {
            text-align: center;
            margin-bottom: 50px;
            animation: fadeInDown 0.8s ease-out;
        }
        
        h1 {
            font-size: 48px;
            font-weight: 300;
            margin-bottom: 10px;
            text-shadow: 0 2px 4px rgba(0,0,0,0.3);
        }
        
        .subtitle {
            font-size: 18px;
            opacity: 0.9;
        }
        
        .time-display {
            text-align: center;
            margin-bottom: 40px;
            animation: fadeIn 1s ease-out 0.2s both;
        }
        
        #time {
            font-size: 72px;
            font-weight: 100;
            text-shadow: 0 4px 8px rgba(0,0,0,0.3);
        }
        
        #date {
            font-size: 24px;
            opacity: 0.8;
            margin-top: 10px;
        }
        
        .search-container {
            margin: 40px auto;
            max-width: 600px;
            animation: fadeInUp 0.8s ease-out 0.4s both;
        }
        
        .search-box {
            width: 100%;
            padding: 20px 25px;
            font-size: 18px;
            border: none;
            border-radius: 50px;
            background: rgba(255, 255, 255, 0.95);
            color: #333;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
            transition: all 0.3s ease;
        }
        
        .search-box:focus {
            outline: none;
            transform: translateY(-2px);
            box-shadow: 0 12px 40px rgba(0, 0, 0, 0.4);
        }
        
        .search-box::placeholder {
            color: #999;
        }
        
        .bookmarks {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 20px;
            margin-top: 60px;
            animation: fadeInUp 0.8s ease-out 0.6s both;
        }
        
        .bookmark {
            background: rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.2);
            border-radius: 20px;
            padding: 30px 20px;
            text-align: center;
            text-decoration: none;
            color: #fff;
            transition: all 0.3s ease;
            cursor: pointer;
            position: relative;
            overflow: hidden;
        }
        
        .bookmark::before {
            content: '';
            position: absolute;
            top: -50%;
            left: -50%;
            width: 200%;
            height: 200%;
            background: radial-gradient(circle, rgba(255,255,255,0.1) 0%, transparent 70%);
            transform: scale(0);
            transition: transform 0.5s ease;
        }
        
        .bookmark:hover {
            transform: translateY(-5px) scale(1.05);
            background: rgba(255, 255, 255, 0.2);
            box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);
        }
        
        .bookmark:hover::before {
            transform: scale(1);
        }
        
        .bookmark-icon {
            font-size: 48px;
            margin-bottom: 15px;
            display: block;
            filter: drop-shadow(0 2px 4px rgba(0,0,0,0.3));
        }
        
        .bookmark-name {
            font-size: 16px;
            font-weight: 500;
        }
        
        .widgets {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-top: 60px;
            animation: fadeInUp 0.8s ease-out 0.8s both;
        }
        
        .widget {
            background: rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 20px;
            padding: 25px;
            transition: all 0.3s ease;
        }
        
        .widget:hover {
            background: rgba(255, 255, 255, 0.1);
            transform: translateY(-2px);
        }
        
        .widget h3 {
            margin-bottom: 15px;
            font-size: 20px;
            font-weight: 400;
        }
        
        .quick-links {
            list-style: none;
        }
        
        .quick-links li {
            margin: 8px 0;
        }
        
        .quick-links a {
            color: #fff;
            text-decoration: none;
            opacity: 0.8;
            transition: opacity 0.2s ease;
        }
        
        .quick-links a:hover {
            opacity: 1;
            text-decoration: underline;
        }
        
        @keyframes fadeInDown {
            from {
                opacity: 0;
                transform: translateY(-30px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }
        
        @keyframes fadeInUp {
            from {
                opacity: 0;
                transform: translateY(30px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }
        
        @keyframes fadeIn {
            from {
                opacity: 0;
            }
            to {
                opacity: 1;
            }
        }
        
        .floating-shapes {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            overflow: hidden;
            z-index: -1;
        }
        
        .shape {
            position: absolute;
            background: rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(5px);
            border-radius: 50%;
            animation: float 20s infinite ease-in-out;
        }
        
        .shape:nth-child(1) {
            width: 80px;
            height: 80px;
            left: 10%;
            animation-delay: 0s;
        }
        
        .shape:nth-child(2) {
            width: 120px;
            height: 120px;
            right: 15%;
            animation-delay: 5s;
        }
        
        .shape:nth-child(3) {
            width: 60px;
            height: 60px;
            left: 70%;
            animation-delay: 10s;
        }
        
        @keyframes float {
            0%, 100% {
                transform: translateY(100vh) rotate(0deg);
                opacity: 0;
            }
            10% {
                opacity: 0.4;
            }
            90% {
                opacity: 0.4;
            }
            100% {
                transform: translateY(-100vh) rotate(360deg);
                opacity: 0;
            }
        }
    </style>
</head>
<body>
    <div class="floating-shapes">
        <div class="shape"></div>
        <div class="shape"></div>
        <div class="shape"></div>
    </div>
    
    <div class="container">
        <div class="header">
            <h1>Simple Browser</h1>
            <p class="subtitle">Fast, Secure, and Built from Scratch</p>
        </div>
        
        <div class="time-display">
            <div id="time"></div>
            <div id="date"></div>
        </div>
        
        <div class="search-container">
            <input type="text" class="search-box" placeholder="Search the web or enter URL..." id="searchBox" autofocus>
        </div>
        
        <div class="bookmarks">
            <a href="https://www.google.com" class="bookmark">
                <span class="bookmark-icon">🔍</span>
                <span class="bookmark-name">Google</span>
            </a>
            <a href="https://www.github.com" class="bookmark">
                <span class="bookmark-icon">💻</span>
                <span class="bookmark-name">GitHub</span>
            </a>
            <a href="https://www.wikipedia.org" class="bookmark">
                <span class="bookmark-icon">📚</span>
                <span class="bookmark-name">Wikipedia</span>
            </a>
            <a href="https://www.youtube.com" class="bookmark">
                <span class="bookmark-icon">📺</span>
                <span class="bookmark-name">YouTube</span>
            </a>
            <a href="https://www.reddit.com" class="bookmark">
                <span class="bookmark-icon">🌐</span>
                <span class="bookmark-name">Reddit</span>
            </a>
            <a href="https://www.stackoverflow.com" class="bookmark">
                <span class="bookmark-icon">💡</span>
                <span class="bookmark-name">Stack Overflow</span>
            </a>
        </div>
        
        <div class="widgets">
            <div class="widget">
                <h3>🚀 Quick Links</h3>
                <ul class="quick-links">
                    <li><a href="https://news.ycombinator.com">Hacker News</a></li>
                    <li><a href="https://www.producthunt.com">Product Hunt</a></li>
                    <li><a href="https://dev.to">Dev.to</a></li>
                    <li><a href="https://medium.com">Medium</a></li>
                </ul>
            </div>
            
            <div class="widget">
                <h3>ℹ️ Browser Info</h3>
                <ul class="quick-links">
                    <li><a href="about:version">About Simple Browser</a></li>
                    <li><a href="https://www.w3.org">W3C Standards</a></li>
                    <li><a href="https://developer.mozilla.org">MDN Web Docs</a></li>
                </ul>
            </div>
        </div>
    </div>
    
    <script>
        // Update time and date
        function updateTime() {
            const now = new Date();
            const time = now.toLocaleTimeString('en-US', { 
                hour: '2-digit', 
                minute: '2-digit',
                hour12: false 
            });
            const date = now.toLocaleDateString('en-US', { 
                weekday: 'long', 
                year: 'numeric', 
                month: 'long', 
                day: 'numeric' 
            });
            
            document.getElementById('time').textContent = time;
            document.getElementById('date').textContent = date;
        }
        
        updateTime();
        setInterval(updateTime, 1000);
        
        // Search functionality
        const searchBox = document.getElementById('searchBox');
        searchBox.addEventListener('keypress', function(e) {
            if (e.key === 'Enter') {
                const value = this.value.trim();
                if (!value) return;
                
                // Check if it's a URL
                if (value.includes('.') || value.startsWith('http://') || value.startsWith('https://')) {
                    // Ensure it has a protocol
                    if (!value.startsWith('http://') && !value.startsWith('https://')) {
                        window.location.href = 'http://' + value;
                    } else {
                        window.location.href = value;
                    }
                } else {
                    // Search on Google
                    window.location.href = 'https://www.google.com/search?q=' + encodeURIComponent(value);
                }
            }
        });
        
        // Add hover sound effect (visual feedback only in this case)
        const bookmarks = document.querySelectorAll('.bookmark');
        bookmarks.forEach(bookmark => {
            bookmark.addEventListener('mouseenter', function() {
                this.style.transform = 'translateY(-5px) scale(1.05)';
            });
            
            bookmark.addEventListener('mouseleave', function() {
                this.style.transform = 'translateY(0) scale(1)';
            });
        });
        
        // Add click animation
        bookmarks.forEach(bookmark => {
            bookmark.addEventListener('click', function(e) {
                e.preventDefault();
                const href = this.getAttribute('href');
                
                // Add click animation
                this.style.transform = 'scale(0.95)';
                setTimeout(() => {
                    window.location.href = href;
                }, 150);
            });
        });
    </script>
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