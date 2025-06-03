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
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            margin: 0;
            padding: 20px;
            display: flex;
            flex-direction: column;
            align-items: center;
            min-height: 100vh;
            color: #333;
        }
        
        .container {
            max-width: 800px;
            width: 100%;
            background: rgba(255, 255, 255, 0.95);
            border-radius: 20px;
            box-shadow: 0 8px 32px rgba(0,0,0,0.2);
            padding: 40px;
            text-align: center;
            backdrop-filter: blur(10px);
            animation: slideIn 0.5s ease-out;
        }
        
        @keyframes slideIn {
            from {
                opacity: 0;
                transform: translateY(30px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }
        
        h1 {
            color: #5a67d8;
            margin-bottom: 10px;
            font-size: 48px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.1);
        }
        
        .subtitle {
            color: #718096;
            margin-bottom: 30px;
            font-size: 20px;
        }
        
        .search-container {
            position: relative;
            margin-bottom: 40px;
        }
        
        .search-box {
            width: 100%;
            padding: 15px 50px 15px 20px;
            font-size: 18px;
            border: 3px solid #e2e8f0;
            border-radius: 50px;
            outline: none;
            transition: all 0.3s;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        
        .search-box:focus {
            border-color: #667eea;
            box-shadow: 0 0 0 4px rgba(102, 126, 234, 0.1), 0 4px 6px rgba(0,0,0,0.1);
            transform: translateY(-2px);
        }
        
        .search-button {
            position: absolute;
            right: 5px;
            top: 50%;
            transform: translateY(-50%);
            padding: 12px 24px;
            font-size: 16px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 50px;
            cursor: pointer;
            transition: all 0.3s;
            font-weight: bold;
        }
        
        .search-button:hover {
            transform: translateY(-50%) scale(1.05);
            box-shadow: 0 4px 12px rgba(102, 126, 234, 0.4);
        }
        
        .search-button:active {
            transform: translateY(-50%) scale(0.95);
        }
        
        .quick-links {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 20px;
            margin-bottom: 40px;
        }
        
        .link-card {
            background: white;
            padding: 20px;
            border-radius: 15px;
            text-decoration: none;
            color: #333;
            transition: all 0.3s;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
            border: 2px solid transparent;
        }
        
        .link-card:hover {
            transform: translateY(-5px);
            box-shadow: 0 8px 16px rgba(0,0,0,0.2);
            border-color: #667eea;
            background: linear-gradient(135deg, #f7fafc 0%, #edf2f7 100%);
        }
        
        .link-card h3 {
            margin: 10px 0 5px 0;
            color: #5a67d8;
            font-size: 18px;
        }
        
        .link-card p {
            margin: 0;
            color: #718096;
            font-size: 14px;
        }
        
        .link-icon {
            font-size: 32px;
            margin-bottom: 10px;
        }
        
        .clock {
            position: fixed;
            top: 20px;
            right: 20px;
            font-size: 18px;
            color: white;
            background: rgba(0, 0, 0, 0.3);
            padding: 10px 20px;
            border-radius: 50px;
            backdrop-filter: blur(10px);
            font-weight: bold;
            text-shadow: 1px 1px 2px rgba(0,0,0,0.5);
        }
        
        .tips {
            margin-top: 40px;
            padding: 25px;
            background: linear-gradient(135deg, #f0f4ff 0%, #e6edff 100%);
            border-radius: 15px;
            text-align: left;
            border-left: 5px solid #667eea;
        }
        
        .tips h3 {
            color: #5a67d8;
            margin-bottom: 15px;
            font-size: 22px;
        }
        
        .tips ul {
            color: #4a5568;
            margin: 0;
            padding-left: 25px;
            line-height: 1.8;
        }
        
        .tips li {
            margin-bottom: 8px;
        }
        
        .feature-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin-top: 40px;
        }
        
        .feature-item {
            background: linear-gradient(135deg, #fbb6ce 0%, #f687b3 100%);
            color: white;
            padding: 30px;
            border-radius: 15px;
            text-align: center;
            cursor: pointer;
            transition: all 0.3s;
        }
        
        .feature-item:hover {
            transform: scale(1.05);
            box-shadow: 0 8px 24px rgba(0,0,0,0.2);
        }
        
        .feature-item h4 {
            margin: 15px 0 10px 0;
            font-size: 20px;
        }
        
        .feature-icon {
            font-size: 48px;
        }
        
        #greeting {
            font-size: 24px;
            color: #5a67d8;
            margin-bottom: 20px;
            font-weight: bold;
        }
        
        .theme-toggle {
            position: fixed;
            bottom: 20px;
            right: 20px;
            background: white;
            border: none;
            width: 60px;
            height: 60px;
            border-radius: 50%;
            cursor: pointer;
            box-shadow: 0 4px 12px rgba(0,0,0,0.2);
            font-size: 24px;
            transition: all 0.3s;
        }
        
        .theme-toggle:hover {
            transform: scale(1.1);
            box-shadow: 0 6px 20px rgba(0,0,0,0.3);
        }
    </style>
</head>
<body>
    <div class="clock" id="clock"></div>
    
    <div class="container">
        <h1>🌐 Welcome to Simple Browser</h1>
        <p class="subtitle">Your gateway to the web</p>
        
        <div id="greeting"></div>
        
        <div class="search-container">
            <input type="text" id="searchBox" class="search-box" placeholder="Search or enter URL..." />
            <button class="search-button" onclick="performSearch()">Go</button>
        </div>
        
        <div class="quick-links">
            <a href="https://www.google.com" class="link-card">
                <div class="link-icon">🔍</div>
                <h3>Google</h3>
                <p>Search the web</p>
            </a>
            <a href="https://www.wikipedia.org" class="link-card">
                <div class="link-icon">📚</div>
                <h3>Wikipedia</h3>
                <p>Free encyclopedia</p>
            </a>
            <a href="https://www.github.com" class="link-card">
                <div class="link-icon">💻</div>
                <h3>GitHub</h3>
                <p>Code repository</p>
            </a>
            <a href="https://news.ycombinator.com" class="link-card">
                <div class="link-icon">📰</div>
                <h3>Hacker News</h3>
                <p>Tech news</p>
            </a>
        </div>
        
        <div class="feature-grid">
            <div class="feature-item" onclick="showAlert('Feature coming soon!')">
                <div class="feature-icon">⚡</div>
                <h4>Fast Browsing</h4>
                <p>Lightning quick page loads</p>
            </div>
            <div class="feature-item" onclick="showAlert('Privacy features enabled!')">
                <div class="feature-icon">🔒</div>
                <h4>Secure</h4>
                <p>Your privacy matters</p>
            </div>
            <div class="feature-item" onclick="showAlert('Customization coming soon!')">
                <div class="feature-icon">🎨</div>
                <h4>Customizable</h4>
                <p>Make it yours</p>
            </div>
        </div>
        
        <div class="tips">
            <h3>💡 Quick Tips:</h3>
            <ul>
                <li>Type a URL or search query in the box above</li>
                <li>Press Enter or click Go to navigate</li>
                <li>Use the navigation buttons in the toolbar</li>
                <li>Click on any quick link card to visit popular sites</li>
                <li>Try clicking the feature cards below!</li>
            </ul>
        </div>
    </div>
    
    <button class="theme-toggle" onclick="toggleTheme()">🌙</button>
    
    <script>
        var isDarkMode = false;
        
        // Greeting based on time of day
        function updateGreeting() {
            var now = new Date();
            var hour = now.getHours();
            var greeting = '';
            
            if (hour < 12) {
                greeting = '☀️ Good Morning!';
            } else if (hour < 18) {
                greeting = '🌤️ Good Afternoon!';
            } else {
                greeting = '🌙 Good Evening!';
            }
            
            document.getElementById('greeting').textContent = greeting;
        }
        
        // Clock functionality
        function updateClock() {
            var now = new Date();
            var timeStr = now.toLocaleTimeString();
            var dateStr = now.toLocaleDateString();
            document.getElementById('clock').textContent = dateStr + ' ' + timeStr;
        }
        
        // Update greeting and clock immediately
        updateGreeting();
        updateClock();
        
        // Update clock every second
        setInterval(updateClock, 1000);
        
        // Search functionality
        function performSearch() {
            var searchBox = document.getElementById('searchBox');
            var query = searchBox.value.trim();
            if (query) {
                // Add search animation
                searchBox.style.transform = 'scale(0.98)';
                setTimeout(function() {
                    searchBox.style.transform = 'scale(1)';
                }, 100);
                
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
        
        // Handle link clicks with animation
        var links = document.querySelectorAll('.link-card');
        for (var i = 0; i < links.length; i++) {
            links[i].addEventListener('click', function(e) {
                e.preventDefault();
                var card = this;
                card.style.transform = 'scale(0.95)';
                setTimeout(function() {
                    window.location.href = card.href;
                }, 150);
            });
        }
        
        // Animate search box on focus
        var searchBox = document.getElementById('searchBox');
        searchBox.addEventListener('focus', function() {
            this.parentElement.style.transform = 'scale(1.02)';
        });
        
        searchBox.addEventListener('blur', function() {
            this.parentElement.style.transform = 'scale(1)';
        });
        
        // Show alert for features
        function showAlert(message) {
            // Create custom alert
            var alertDiv = document.createElement('div');
            alertDiv.style.cssText = 'position: fixed; top: 50%; left: 50%; transform: translate(-50%, -50%); background: white; padding: 30px; border-radius: 15px; box-shadow: 0 8px 32px rgba(0,0,0,0.3); z-index: 1000; animation: popIn 0.3s ease-out;';
            alertDiv.innerHTML = '<h3 style="margin: 0 0 15px 0; color: #5a67d8;">' + message + '</h3><button onclick="this.parentElement.remove()" style="background: #667eea; color: white; border: none; padding: 10px 20px; border-radius: 8px; cursor: pointer;">OK</button>';
            document.body.appendChild(alertDiv);
        }
        
        // Theme toggle
        function toggleTheme() {
            isDarkMode = !isDarkMode;
            var body = document.body;
            var button = document.querySelector('.theme-toggle');
            
            if (isDarkMode) {
                body.style.background = 'linear-gradient(135deg, #1a202c 0%, #2d3748 100%)';
                button.textContent = '☀️';
            } else {
                body.style.background = 'linear-gradient(135deg, #667eea 0%, #764ba2 100%)';
                button.textContent = '🌙';
            }
        }
        
        // Add some particle effects
        function createParticle() {
            var particle = document.createElement('div');
            particle.style.cssText = 'position: fixed; width: 4px; height: 4px; background: white; border-radius: 50%; pointer-events: none; opacity: 0.8;';
            particle.style.left = Math.random() * window.innerWidth + 'px';
            particle.style.top = '-10px';
            document.body.appendChild(particle);
            
            var speed = 1 + Math.random() * 2;
            var drift = (Math.random() - 0.5) * 2;
            
            function animate() {
                var top = parseFloat(particle.style.top);
                var left = parseFloat(particle.style.left);
                
                particle.style.top = (top + speed) + 'px';
                particle.style.left = (left + drift) + 'px';
                particle.style.opacity = parseFloat(particle.style.opacity) - 0.01;
                
                if (top < window.innerHeight && parseFloat(particle.style.opacity) > 0) {
                    requestAnimationFrame(animate);
                } else {
                    particle.remove();
                }
            }
            
            requestAnimationFrame(animate);
        }
        
        // Create particles occasionally
        setInterval(createParticle, 500);
        
        // Focus search box on page load
        searchBox.focus();
        
        // Add CSS animation
        var style = document.createElement('style');
        style.textContent = '@keyframes popIn { from { opacity: 0; transform: translate(-50%, -50%) scale(0.8); } to { opacity: 1; transform: translate(-50%, -50%) scale(1); } }';
        document.head.appendChild(style);
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
                    if (element->tagName() == "input" || element->tagName() == "INPUT") {
                        // Create a proper value property
                        std::string value = element->getAttribute("value");
                        elementObj->set("value", custom_js::JSValue(value));
                    }
                    
                    // textContent property
                    elementObj->set("textContent", custom_js::JSValue(element->textContent()));
                    
                    // innerHTML property (simplified)
                    elementObj->set("innerHTML", custom_js::JSValue(element->innerHTML()));
                    
                    // style property
                    auto styleObj = std::make_shared<custom_js::JSObject>();
                    styleObj->set("transform", custom_js::JSValue(""));
                    styleObj->set("background", custom_js::JSValue(""));
                    styleObj->set("cssText", custom_js::JSValue(""));
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
                    if (element->tagName() == "a" || element->tagName() == "A") {
                        elementObj->set("href", custom_js::JSValue(element->getAttribute("href")));
                    }
                    
                    // parentElement property
                    if (element->parentElement()) {
                        auto parentObj = std::make_shared<custom_js::JSObject>();
                        parentObj->set("style", custom_js::JSValue(styleObj));
                        elementObj->set("parentElement", custom_js::JSValue(parentObj));
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
                            if (elem->tagName() == "a" || elem->tagName() == "A") {
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
                
                // Create array object with length property
                auto arrayObj = std::make_shared<custom_js::JSObject>();
                arrayObj->set("length", custom_js::JSValue(static_cast<double>(jsElements.size())));
                
                // Add array elements
                for (size_t i = 0; i < jsElements.size(); ++i) {
                    arrayObj->set(std::to_string(i), jsElements[i]);
                }
                
                return custom_js::JSValue(arrayObj);
            }
        )
    ));
    
    // document.createElement
    docObj->set("createElement", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [this](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                if (args.empty()) return custom_js::JSValue();
                
                std::string tagName = args[0].toString();
                auto element = m_domTree.document()->createElement(tagName);
                
                auto elementObj = std::make_shared<custom_js::JSObject>();
                elementObj->set("tagName", custom_js::JSValue(tagName));
                
                // Style object
                auto styleObj = std::make_shared<custom_js::JSObject>();
                styleObj->set("cssText", custom_js::JSValue(""));
                elementObj->set("style", custom_js::JSValue(styleObj));
                
                return custom_js::JSValue(elementObj);
            }
        )
    ));
    
    // document.head
    auto headElement = m_domTree.document()->getElementsByTagName("head");
    if (!headElement.empty()) {
        auto headObj = std::make_shared<custom_js::JSObject>();
        headObj->set("appendChild", custom_js::JSValue(
            std::make_shared<custom_js::JSFunction>(
                [](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                    std::cout << "appendChild called on head" << std::endl;
                    return custom_js::JSValue();
                }
            )
        ));
        docObj->set("head", custom_js::JSValue(headObj));
    }
    
    // document.body
    auto bodyElement = m_domTree.document()->getElementsByTagName("body");
    if (!bodyElement.empty()) {
        auto bodyObj = std::make_shared<custom_js::JSObject>();
        bodyObj->set("appendChild", custom_js::JSValue(
            std::make_shared<custom_js::JSFunction>(
                [](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                    std::cout << "appendChild called on body" << std::endl;
                    return custom_js::JSValue();
                }
            )
        ));
        docObj->set("body", custom_js::JSValue(bodyObj));
    }
    
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
    
    // Create href property with getter/setter
    locationObj->set("href", custom_js::JSValue(m_currentUrl));
    
    // Add a special property for navigation
    locationObj->set("__setHref__", custom_js::JSValue(
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
    
    windowObj->set("location", custom_js::JSValue(locationObj));
    
    // Override location.href setter in JavaScript
    m_jsEngine.executeScript(R"(
        Object.defineProperty(window.location, 'href', {
            get: function() { return window.location.__href__ || ''; },
            set: function(value) { 
                console.log('Setting location.href to: ' + value);
                window.location.__setHref__(value);
            }
        });
    )", {}, {});
    
    // Add Date constructor
    windowObj->set("Date", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                auto dateObj = std::make_shared<custom_js::JSObject>();
                
                // Get current time
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                auto tm = *std::localtime(&time_t);
                
                // Add getHours method
                dateObj->set("getHours", custom_js::JSValue(
                    std::make_shared<custom_js::JSFunction>(
                        [tm](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                            return custom_js::JSValue(static_cast<double>(tm.tm_hour));
                        }
                    )
                ));
                
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
    
    // Add setTimeout
    windowObj->set("setTimeout", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                // In a real implementation, this would schedule the callback
                return custom_js::JSValue(1.0);
            }
        )
    ));
    
    // Add requestAnimationFrame
    windowObj->set("requestAnimationFrame", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                // In a real implementation, this would schedule the callback
                return custom_js::JSValue(1.0);
            }
        )
    ));
    
    // Add window.innerWidth and window.innerHeight
    windowObj->set("innerWidth", custom_js::JSValue(1024.0));
    windowObj->set("innerHeight", custom_js::JSValue(768.0));
    
    // Add Math object
    auto mathObj = std::make_shared<custom_js::JSObject>();
    mathObj->set("random", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                return custom_js::JSValue(static_cast<double>(rand()) / RAND_MAX);
            }
        )
    ));
    
    windowObj->set("Math", custom_js::JSValue(mathObj));
    
    // Add parseFloat
    windowObj->set("parseFloat", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                if (args.empty()) return custom_js::JSValue(0.0);
                std::string str = args[0].toString();
                try {
                    return custom_js::JSValue(std::stod(str));
                } catch (...) {
                    return custom_js::JSValue(0.0);
                }
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
    m_jsEngine.defineGlobalVariable("setTimeout", windowObj->get("setTimeout"));
    m_jsEngine.defineGlobalVariable("encodeURIComponent", windowObj->get("encodeURIComponent"));
    m_jsEngine.defineGlobalVariable("requestAnimationFrame", windowObj->get("requestAnimationFrame"));
    m_jsEngine.defineGlobalVariable("Math", windowObj->get("Math"));
    m_jsEngine.defineGlobalVariable("parseFloat", windowObj->get("parseFloat"));
    
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