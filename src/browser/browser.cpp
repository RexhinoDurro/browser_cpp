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
    <title>Simple Browser - Test</title>
    <style>
        body {
            background-color: #ff0000;
            margin: 0;
            padding: 20px;
        }
        h1 {
            color: white;
            background-color: blue;
            padding: 10px;
        }
        .test {
            background-color: yellow;
            color: black;
            padding: 10px;
            margin: 10px;
        }
    </style>
</head>
<body>
    <h1>Test Page</h1>
    <div class="test">This should have a yellow background</div>
    <p>This is a test paragraph.</p>
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
    
    // IMPORTANT: This must be called!
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
    
    // Print what elements have what styles
    std::cout << "\n=== Applied Styles ===" << std::endl;
    html::Element* body = m_domTree.document()->getElementsByTagName("body")[0];
    if (body) {
        css::ComputedStyle bodyStyle = m_styleResolver.getComputedStyle(body);
        std::cout << "Body background-color: " << bodyStyle.getProperty("background-color").stringValue() << std::endl;
    }
    std::cout << "==================\n" << std::endl;
    
    return true;

    
    // Execute scripts for interactivity
    if (!loadScripts(url)) {
        std::cerr << "Warning: Failed to load scripts for about page" << std::endl;
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
                    
                    // Add value property (for input elements)
                    elementObj->set("value", custom_js::JSValue(element->getAttribute("value")));
                    
                    // Add addEventListener method
                    elementObj->set("addEventListener", custom_js::JSValue(
                        std::make_shared<custom_js::JSFunction>(
                            [element, this](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                                if (args.size() >= 2 && args[0].isString() && args[1].isFunction()) {
                                    std::string event = args[0].toString();
                                    // Store the event handler (in a real implementation)
                                    // For now, we'll just handle keypress on searchBox
                                    if (event == "keypress" && element->id() == "searchBox") {
                                        // Store the handler for later use
                                        // This is simplified - real implementation would store all handlers
                                    }
                                }
                                return custom_js::JSValue();
                            }
                        )
                    ));
                    
                    // Add trim method to string values
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
                                }
                                return custom_js::JSValue(value);
                            }
                        )
                    ));
                    
                    return custom_js::JSValue(elementObj);
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
                    
                    // Add addEventListener for links
                    elementObj->set("addEventListener", custom_js::JSValue(
                        std::make_shared<custom_js::JSFunction>(
                            [elem, this](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                                // Simplified event handler storage
                                return custom_js::JSValue();
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
                
                // Simplified selector handling - just handle class selectors
                if (selector.size() > 0 && selector[0] == '.') {
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
                                        return custom_js::JSValue();
                                    }
                                )
                            ));
                            
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
    
    // Add location object with navigation support
    auto locationObj = std::make_shared<custom_js::JSObject>();
    locationObj->set("href", custom_js::JSValue(m_currentUrl));
    
    // Make href a settable property that triggers navigation
    locationObj->set("__href_setter__", custom_js::JSValue(
        std::make_shared<custom_js::JSFunction>(
            [this](const std::vector<custom_js::JSValue>& args, custom_js::JSValue thisValue) {
                if (!args.empty()) {
                    std::string newUrl = args[0].toString();
                    // Queue navigation (in a real implementation, this would be async)
                    std::cout << "JavaScript navigation to: " << newUrl << std::endl;
                    // Note: In a real implementation, you'd need to handle this navigation request
                    // properly through the event loop
                }
                return custom_js::JSValue();
            }
        )
    ));
    
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
                            // Simple time format HH:MM
                            snprintf(buffer, sizeof(buffer), "%02d:%02d", tm.tm_hour, tm.tm_min);
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
                // For now, we'll just return a fake timer ID
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
    
    // Also define some globals directly
    m_jsEngine.defineGlobalVariable("document", windowObj->get("document"));
    m_jsEngine.defineGlobalVariable("console", windowObj->get("console"));
    m_jsEngine.defineGlobalVariable("Date", windowObj->get("Date"));
    m_jsEngine.defineGlobalVariable("setInterval", windowObj->get("setInterval"));
    m_jsEngine.defineGlobalVariable("encodeURIComponent", windowObj->get("encodeURIComponent"));
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