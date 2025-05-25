#include "browser.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

namespace browser {

Browser::Browser() {
}

Browser::~Browser() {
}

bool Browser::initialize() {
    // Initialize HTML parser
    if (!m_htmlParser.initialize()) {
        std::cerr << "Failed to initialize HTML parser" << std::endl;
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
    
#ifndef JS_ENGINE_DISABLED
    // Initialize JavaScript engine
    if (!m_jsEngine.initialize()) {
        std::cerr << "Failed to initialize JavaScript engine" << std::endl;
        return false;
    }
#else
    std::cout << "JavaScript support is disabled in this build" << std::endl;
#endif
    
    // Create and initialize resource loader
    m_resourceLoader = std::make_unique<networking::ResourceLoader>();
    if (!m_resourceLoader->initialize("./browser_cache")) {
        std::cerr << "Failed to initialize resource loader" << std::endl;
        return false;
    }
    
    // Create and initialize security manager
    m_securityManager = std::make_unique<security::SecurityManager>();
    if (!m_securityManager->initialize()) {
        std::cerr << "Failed to initialize security manager" << std::endl;
        return false;
    }
    
    return true;
}

bool Browser::loadUrl(const std::string& url, std::string& error) {
    std::cout << "Loading URL: " << url << std::endl;
    
    // Create Origin object for security checks
    security::Origin origin(url);
    
    // Load HTML content
    std::vector<uint8_t> htmlData;
    std::map<std::string, std::string> headers;
    
    if (!m_resourceLoader->loadResource(url, htmlData, headers, error)) {
        std::cerr << "Failed to load URL: " << error << std::endl;
        return false;
    }
    
    // Process security headers
    processSecurityHeaders(headers, url);
    
    // Convert HTML data to string
    std::string html(htmlData.begin(), htmlData.end());
    
    // Check for XSS in the response
    if (m_securityManager->isPotentialXSS(html)) {
        std::cout << "Warning: Potential XSS detected in the response" << std::endl;
        
        // Apply XSS protection based on the configured mode
        auto xssMode = m_securityManager->xssProtection()->mode();
        if (xssMode == security::XssProtectionMode::BLOCK) {
            error = "Potential XSS attack blocked";
            return false;
        }
        else if (xssMode == security::XssProtectionMode::ENABLED) {
            // Sanitize the HTML
            html = m_securityManager->sanitizeHtml(html);
        }
    }
    
    // Parse HTML into DOM tree
    std::cout << "Parsing HTML..." << std::endl;
    m_domTree = m_htmlParser.parse(html);
    
    // Load and process stylesheets
    if (!loadStylesheets(url)) {
        std::cerr << "Warning: Failed to load some stylesheets" << std::endl;
    }
    
    // Set the document for style resolver
    m_styleResolver.setDocument(m_domTree.document());
    
    // Perform layout calculations
    std::cout << "Calculating layout..." << std::endl;
    m_layoutEngine.layoutDocument(m_domTree.document(), &m_styleResolver, 800, 600);
    
    // Load and execute scripts (after layout, so they can access computed styles)
    if (!loadScripts(url)) {
        std::cerr << "Warning: Failed to load some scripts" << std::endl;
    }
    
    // Recalculate layout after scripts may have modified the DOM
    m_styleResolver.resolveStyles();
    m_layoutEngine.layoutDocument(m_domTree.document(), &m_styleResolver, 800, 600);
    
    // Load images (can happen in parallel with script execution in a real browser)
    if (!loadImages(url)) {
        std::cerr << "Warning: Failed to load some images" << std::endl;
    }
    
    return true;
}

std::string Browser::renderToASCII(int width, int height) {
    // Get the layout tree root
    std::shared_ptr<layout::Box> layoutRoot = m_layoutEngine.layoutRoot();
    
    if (!layoutRoot) {
        return "No content to render";
    }
    
    // Render to ASCII
    return m_renderer.renderToASCII(layoutRoot.get(), width, height);
}

bool Browser::loadStylesheets(const std::string& baseUrl) {
    // Get stylesheet links
    std::vector<std::string> stylesheetLinks = m_domTree.findStylesheetLinks();
    
    // Get inline styles
    std::vector<std::string> inlineStyles = m_domTree.findInlineStyles();
    
    // Create Origin object for baseUrl
    security::Origin origin(baseUrl);
    
    // Load external stylesheets
    for (const auto& cssUrl : stylesheetLinks) {
        // Resolve relative URL
        std::string fullUrl = cssUrl;
        if (cssUrl.find("://") == std::string::npos) {
            // Simple relative URL handling
            if (cssUrl[0] == '/') {
                // Absolute path
                size_t pos = baseUrl.find("://");
                if (pos != std::string::npos) {
                    pos = baseUrl.find('/', pos + 3);
                    if (pos != std::string::npos) {
                        fullUrl = baseUrl.substr(0, pos) + cssUrl;
                    } else {
                        fullUrl = baseUrl + cssUrl;
                    }
                }
            } else {
                // Relative path
                size_t pos = baseUrl.find_last_of('/');
                if (pos != std::string::npos) {
                    fullUrl = baseUrl.substr(0, pos + 1) + cssUrl;
                } else {
                    fullUrl = baseUrl + "/" + cssUrl;
                }
            }
        }
        
        // Create Origin object for the stylesheet
        security::Origin cssOrigin(fullUrl);
        
        // Check if allowed by CSP
        if (!m_securityManager->isAllowedByCSP(fullUrl, security::CspResourceType::STYLE, origin)) {
            std::cerr << "Stylesheet blocked by CSP: " << fullUrl << std::endl;
            continue;
        }
        
        // Check same-origin policy
        if (!m_securityManager->sameOriginPolicy()->canReceiveResponse(origin, cssOrigin)) {
            std::cerr << "Stylesheet blocked by same-origin policy: " << fullUrl << std::endl;
            continue;
        }
        
        // Load the stylesheet
        std::cout << "Loading CSS: " << fullUrl << std::endl;
        
        std::vector<uint8_t> cssData;
        std::map<std::string, std::string> cssHeaders;
        std::string error;
        
        if (m_resourceLoader->loadResource(fullUrl, cssData, cssHeaders, error)) {
            std::string cssText(cssData.begin(), cssData.end());
            
            // Parse and add CSS
            css::StyleSheet styleSheet;
            styleSheet.parse(cssText);
            m_styleResolver.addStyleSheet(styleSheet);
        } else {
            std::cerr << "Failed to load CSS: " << error << std::endl;
        }
    }
    
    // Add inline styles
    for (const auto& cssText : inlineStyles) {
        // Check if allowed by CSP
        if (!m_securityManager->contentSecurityPolicy()->allowsInlineStyle()) {
            std::cerr << "Inline style blocked by CSP" << std::endl;
            continue;
        }
        
        css::StyleSheet styleSheet;
        styleSheet.parse(cssText);
        m_styleResolver.addStyleSheet(styleSheet);
    }
    
    return true;
}

#ifdef JS_ENGINE_DISABLED
// Stub implementation when JavaScript is disabled
bool Browser::loadScripts(const std::string& baseUrl) {
    std::cout << "JavaScript support is disabled in this build" << std::endl;
    return true;
}
#else
bool Browser::loadScripts(const std::string& baseUrl) {
    // Get script sources
    std::vector<std::string> scriptSources = m_domTree.findScriptSources();
    
    // Create Origin object for baseUrl
    security::Origin origin(baseUrl);
    
    // Load scripts
    for (const auto& scriptUrl : scriptSources) {
        // Resolve relative URL
        std::string fullUrl = scriptUrl;
        if (scriptUrl.find("://") == std::string::npos) {
            // Simple relative URL handling
            if (scriptUrl[0] == '/') {
                // Absolute path
                size_t pos = baseUrl.find("://");
                if (pos != std::string::npos) {
                    pos = baseUrl.find('/', pos + 3);
                    if (pos != std::string::npos) {
                        fullUrl = baseUrl.substr(0, pos) + scriptUrl;
                    } else {
                        fullUrl = baseUrl + scriptUrl;
                    }
                }
            } else {
                // Relative path
                size_t pos = baseUrl.find_last_of('/');
                if (pos != std::string::npos) {
                    fullUrl = baseUrl.substr(0, pos + 1) + scriptUrl;
                } else {
                    fullUrl = baseUrl + "/" + scriptUrl;
                }
            }
        }
        
        // Create Origin object for the script
        security::Origin scriptOrigin(fullUrl);
        
        // Check if allowed by CSP
        if (!m_securityManager->isAllowedByCSP(fullUrl, security::CspResourceType::SCRIPT, origin)) {
            std::cerr << "Script blocked by CSP: " << fullUrl << std::endl;
            continue;
        }
        
        // Check same-origin policy
        if (!m_securityManager->sameOriginPolicy()->canExecuteScript(origin, scriptOrigin)) {
            std::cerr << "Script blocked by same-origin policy: " << fullUrl << std::endl;
            continue;
        }
        
        // Load the script
        std::cout << "Loading JavaScript: " << fullUrl << std::endl;
        
        std::vector<uint8_t> jsData;
        std::map<std::string, std::string> jsHeaders;
        std::string error;
        
        if (m_resourceLoader->loadResource(fullUrl, jsData, jsHeaders, error)) {
            std::string jsText(jsData.begin(), jsData.end());
            
            // Execute JavaScript
            std::string result, jsError;
            if (!m_jsEngine.executeScript(jsText, result, jsError)) {
                std::cerr << "JavaScript error: " << jsError << std::endl;
            }
        } else {
            std::cerr << "Failed to load JavaScript: " << error << std::endl;
        }
    }
    
    return true;
}
#endif

bool Browser::loadImages(const std::string& baseUrl) {
    // Get image sources
    std::vector<std::string> imageSources = m_domTree.findImageSources();
    
    // Create Origin object for baseUrl
    security::Origin origin(baseUrl);
    
    // Load images
    for (const auto& imageUrl : imageSources) {
        // Resolve relative URL
        std::string fullUrl = imageUrl;
        if (imageUrl.find("://") == std::string::npos) {
            // Simple relative URL handling
            if (imageUrl[0] == '/') {
                // Absolute path
                size_t pos = baseUrl.find("://");
                if (pos != std::string::npos) {
                    pos = baseUrl.find('/', pos + 3);
                    if (pos != std::string::npos) {
                        fullUrl = baseUrl.substr(0, pos) + imageUrl;
                    } else {
                        fullUrl = baseUrl + imageUrl;
                    }
                }
            } else {
                // Relative path
                size_t pos = baseUrl.find_last_of('/');
                if (pos != std::string::npos) {
                    fullUrl = baseUrl.substr(0, pos + 1) + imageUrl;
                } else {
                    fullUrl = baseUrl + "/" + imageUrl;
                }
            }
        }
        
        // Create Origin object for the image
        security::Origin imageOrigin(fullUrl);
        
        // Check if allowed by CSP
        if (!m_securityManager->isAllowedByCSP(fullUrl, security::CspResourceType::IMG, origin)) {
            std::cerr << "Image blocked by CSP: " << fullUrl << std::endl;
            continue;
        }
        
        // Load the image
        std::cout << "Loading image: " << fullUrl << std::endl;
        
        // In a real browser, this would load the image data
        // and create image resources for rendering
        // For the text-based renderer, we don't need to actually load the images
    }
    
    return true;
}

void Browser::processSecurityHeaders(const std::map<std::string, std::string>& headers, const std::string& url) {
    // Process Content-Security-Policy header
    auto cspIt = headers.find("Content-Security-Policy");
    if (cspIt != headers.end()) {
        m_securityManager->contentSecurityPolicy()->parse(cspIt->second);
    }
    
    // Process X-XSS-Protection header
    auto xssIt = headers.find("X-XSS-Protection");
    if (xssIt != headers.end()) {
        m_securityManager->xssProtection()->parseHeader(xssIt->second);
    }
    
    // Process X-Frame-Options header
    auto frameOptionsIt = headers.find("X-Frame-Options");
    if (frameOptionsIt != headers.end()) {
        // In a real browser, this would restrict iframe embedding
        // We don't have iframe support in this simple browser
    }
    
    // Process Strict-Transport-Security header
    auto hstsIt = headers.find("Strict-Transport-Security");
    if (hstsIt != headers.end()) {
        // In a real browser, this would enforce HTTPS for future visits
        // We don't persist HSTS in this simple browser
    }
    
    // Process X-Content-Type-Options header
    auto contentTypeOptionsIt = headers.find("X-Content-Type-Options");
    if (contentTypeOptionsIt != headers.end() && contentTypeOptionsIt->second == "nosniff") {
        // In a real browser, this would prevent MIME type sniffing
        // We don't do MIME type sniffing in this simple browser
    }
    
    // Process Referrer-Policy header
    auto referrerPolicyIt = headers.find("Referrer-Policy");
    if (referrerPolicyIt != headers.end()) {
        // In a real browser, this would control the Referer header
        // We don't send Referer headers in this simple browser
    }
    
    // Process Feature-Policy header
    auto featurePolicyIt = headers.find("Feature-Policy");
    if (featurePolicyIt != headers.end()) {
        // In a real browser, this would control browser features
        // We don't implement Feature Policy in this simple browser
    }
}

// Note: These functions are already declared in the header file
// so they are commented out here to avoid duplicate definition errors
/*
html::Document* Browser::currentDocument() const {
    return m_domTree.document();
}

std::shared_ptr<layout::Box> Browser::layoutRoot() const {
    return m_layoutEngine.layoutRoot();
}
*/

} // namespace browser