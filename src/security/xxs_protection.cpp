#include "xss_protection.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>

namespace browser {
namespace security {

//-----------------------------------------------------------------------------
// XssProtection Implementation
//-----------------------------------------------------------------------------

XssProtection::XssProtection()
    : m_mode(XssProtectionMode::ENABLED)
    , m_sanitizationLevel(SanitizationLevel::BASIC)
{
}

XssProtection::~XssProtection() {
}

bool XssProtection::initialize() {
    // Initialize whitelists for HTML sanitization
    initializeWhitelists();
    return true;
}

bool XssProtection::parseHeader(const std::string& headerValue) {
    // Parse X-XSS-Protection header value
    // Format: 0 or 1; [mode=block] [; report=<reporting-uri>]
    
    if (headerValue.empty()) {
        return false;
    }
    
    // Check if protection is disabled
    if (headerValue == "0") {
        m_mode = XssProtectionMode::DISABLED;
        return true;
    }
    
    // Check if protection is enabled
    if (headerValue[0] == '1') {
        m_mode = XssProtectionMode::ENABLED;
        
        // Check for additional directives
        if (headerValue.find("mode=block") != std::string::npos) {
            m_mode = XssProtectionMode::BLOCK;
        }
        
        // Check for report directive
        size_t reportPos = headerValue.find("report=");
        if (reportPos != std::string::npos) {
            // Extract report URL
            size_t urlStart = headerValue.find('=', reportPos) + 1;
            size_t urlEnd = headerValue.find(';', urlStart);
            
            if (urlEnd == std::string::npos) {
                urlEnd = headerValue.length();
            }
            
            m_reportUrl = headerValue.substr(urlStart, urlEnd - urlStart);
            m_mode = XssProtectionMode::REPORT;
        }
        
        return true;
    }
    
    return false;
}

bool XssProtection::isUrlPotentialXss(const std::string& url) const {
    // Check if the URL contains potential XSS payloads
    // This is a simplified check and would be much more comprehensive in a real implementation
    
    // Convert to lowercase for case-insensitive checks
    std::string lowerUrl = url;
    std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Check for common XSS patterns in URLs
    const std::vector<std::string> xssPatterns = {
        "javascript:",
        "data:text/html",
        "data:application/javascript",
        "vbscript:",
        "<script",
        "%3Cscript",
        "alert(",
        "onerror=",
        "onload=",
        "onclick=",
        "eval(",
        "fromcharcode("
    };
    
    for (const auto& pattern : xssPatterns) {
        if (lowerUrl.find(pattern) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool XssProtection::isHtmlPotentialXss(const std::string& html) const {
    return detectXssPatterns(html);
}

std::string XssProtection::sanitizeHtml(const std::string& html, SanitizationLevel level) const {
    // Sanitize HTML content based on sanitization level
    
    if (level == SanitizationLevel::NONE) {
        return html;
    }
    
    if (level == SanitizationLevel::ESCAPE_ONLY) {
        return escapeHtml(html);
    }
    
    // For BASIC and STRICT sanitization, we need to parse the HTML and filter tags/attributes
    
    // In a real implementation, this would use a proper HTML parser
    // For simplicity, we'll use a regex-based approach for demonstration
    
    std::string sanitized = html;
    
    if (level == SanitizationLevel::BASIC) {
        // Remove script tags and event handlers
        std::regex scriptTagRegex("<script[^>]*>.*?</script>", std::regex::icase | std::regex::dotall);
        sanitized = std::regex_replace(sanitized, scriptTagRegex, "");
        
        // Remove javascript: URLs
        std::regex jsUrlRegex("(href|src|action)\\s*=\\s*[\"']\\s*javascript:", std::regex::icase);
        sanitized = std::regex_replace(sanitized, jsUrlRegex, "data-removed=");
        
        // Remove on* event handlers
        std::regex eventHandlerRegex("\\s+on[a-z]+\\s*=", std::regex::icase);
        sanitized = std::regex_replace(sanitized, eventHandlerRegex, " data-removed=");
    }
    else if (level == SanitizationLevel::STRICT) {
        // In strict mode, we'd whitelist tags and attributes
        // This would require a proper HTML parser
        // For demonstration, we'll use a simplified approach
        
        // Replace all tags with stripped versions
        std::regex tagRegex("<([^>]+)>");
        sanitized = std::regex_replace(sanitized, tagRegex, [this](const std::smatch& match) {
            std::string tag = match[1].str();
            
            // Get tag name
            size_t spacePos = tag.find_first_of(" \t\n\r\f\v");
            std::string tagName = spacePos != std::string::npos ? tag.substr(0, spacePos) : tag;
            
            // Check if closing tag
            bool isClosingTag = false;
            if (!tagName.empty() && tagName[0] == '/') {
                isClosingTag = true;
                tagName = tagName.substr(1);
            }
            
            // Convert to lowercase
            std::transform(tagName.begin(), tagName.end(), tagName.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            
            // Check if allowed
            if (!isAllowedTag(tagName)) {
                return "";
            }
            
            // If closing tag, just return it
            if (isClosingTag) {
                return "</" + tagName + ">";
            }
            
            // Parse attributes
            std::string result = "<" + tagName;
            
            if (spacePos != std::string::npos) {
                std::string attrStr = tag.substr(spacePos + 1);
                std::regex attrRegex("([a-zA-Z0-9_-]+)\\s*=\\s*[\"']([^\"']*)[\"']");
                
                std::string::const_iterator searchStart(attrStr.begin());
                std::smatch attrMatch;
                
                while (std::regex_search(searchStart, attrStr.end(), attrMatch, attrRegex)) {
                    std::string attrName = attrMatch[1].str();
                    std::string attrValue = attrMatch[2].str();
                    
                    // Convert to lowercase
                    std::transform(attrName.begin(), attrName.end(), attrName.begin(),
                                   [](unsigned char c) { return std::tolower(c); });
                    
                    // Check if allowed
                    if (isAllowedAttribute(tagName, attrName)) {
                        // Check for URL attributes
                        if (attrName == "href" || attrName == "src" || attrName == "action") {
                            // Check protocol
                            size_t colonPos = attrValue.find(':');
                            if (colonPos != std::string::npos) {
                                std::string protocol = attrValue.substr(0, colonPos);
                                
                                // Convert to lowercase
                                std::transform(protocol.begin(), protocol.end(), protocol.begin(),
                                               [](unsigned char c) { return std::tolower(c); });
                                
                                if (!isAllowedProtocol(protocol)) {
                                    // Skip this attribute
                                    searchStart = attrMatch[0].second;
                                    continue;
                                }
                            }
                        }
                        
                        result += " " + attrName + "=\"" + attrValue + "\"";
                    }
                    
                    searchStart = attrMatch[0].second;
                }
            }
            
            return result + ">";
        });
    }
    
    return sanitized;
}

std::string XssProtection::sanitizeUrl(const std::string& url) const {
    // Sanitize a URL to remove potential XSS
    
    // Check if URL is potentially harmful
    if (isUrlPotentialXss(url)) {
        // Return a safe placeholder URL
        return "#";
    }
    
    return url;
}

std::string XssProtection::escapeHtml(const std::string& input) const {
    std::string escaped;
    escaped.reserve(input.size());
    
    for (char c : input) {
        switch (c) {
            case '&':
                escaped += "&amp;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            case '\'':
                escaped += "&#39;";
                break;
            default:
                escaped += c;
                break;
        }
    }
    
    return escaped;
}

void XssProtection::initializeWhitelists() {
    // Initialize allowed HTML tags
    m_allowedTags = {
        "a", "abbr", "article", "aside", "b", "blockquote", "br", "caption", "code", "div", 
        "em", "h1", "h2", "h3", "h4", "h5", "h6", "hr", "i", "img", "li", "ol", "p", 
        "pre", "span", "strong", "table", "tbody", "td", "tfoot", "th", "thead", "tr", "ul"
    };
    
    // Initialize allowed attributes for each tag
    
    // Global attributes allowed on all tags
    std::set<std::string> globalAttributes = {
        "class", "id", "title", "lang", "dir", "tabindex"
    };
    
    // Default each tag to allow global attributes
    for (const auto& tag : m_allowedTags) {
        m_allowedAttributes[tag] = globalAttributes;
    }
    
    // Add specific attributes for certain tags
    m_allowedAttributes["a"].insert({"href", "target", "rel"});
    m_allowedAttributes["img"].insert({"src", "alt", "width", "height"});
    m_allowedAttributes["table"].insert({"border", "cellpadding", "cellspacing"});
    m_allowedAttributes["td"].insert({"colspan", "rowspan"});
    m_allowedAttributes["th"].insert({"colspan", "rowspan", "scope"});
    
    // Initialize allowed URL protocols
    m_allowedProtocols = {
        "http", "https", "mailto", "tel", "ftp"
    };
}

bool XssProtection::isAllowedTag(const std::string& tag) const {
    return m_allowedTags.find(tag) != m_allowedTags.end();
}

bool XssProtection::isAllowedAttribute(const std::string& tag, const std::string& attribute) const {
    auto it = m_allowedAttributes.find(tag);
    if (it != m_allowedAttributes.end()) {
        return it->second.find(attribute) != it->second.end();
    }
    return false;
}

bool XssProtection::isAllowedProtocol(const std::string& protocol) const {
    return m_allowedProtocols.find(protocol) != m_allowedProtocols.end();
}

bool XssProtection::detectXssPatterns(const std::string& content) const {
    // Convert to lowercase for case-insensitive checks
    std::string lowerContent = content;
    std::transform(lowerContent.begin(), lowerContent.end(), lowerContent.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Check for common XSS patterns
    const std::vector<std::string> xssPatterns = {
        "<script",
        "javascript:",
        "data:text/html",
        "vbscript:",
        "expression(",
        "eval(",
        "document.cookie",
        "document.domain",
        "document.write",
        "window.location",
        "onload=",
        "onerror=",
        "onmouseover=",
        "onclick=",
        "alert(",
        "prompt(",
        "confirm(",
        "fromcharcode("
    };
    
    for (const auto& pattern : xssPatterns) {
        if (lowerContent.find(pattern) != std::string::npos) {
            return true;
        }
    }
    
    // Check for more complex patterns with regex
    std::vector<std::regex> xssRegexPatterns = {
        std::regex("<\\s*script[^>]*>[\\s\\S]*?<\\s*/\\s*script\\s*>", std::regex::icase),
        std::regex("<\\s*iframe[^>]*>[\\s\\S]*?<\\s*/\\s*iframe\\s*>", std::regex::icase),
        std::regex("<[^>]*\\s+on[a-z]+\\s*=", std::regex::icase),
        std::regex("<[^>]*\\s+dynsrc\\s*=", std::regex::icase),
        std::regex("<[^>]*\\s+lowsrc\\s*=", std::regex::icase),
        std::regex("\\+=\\s*\\\\[\"']\\s*\\+\\s*\\\\[\"']", std::regex::icase)
    };
    
    for (const auto& regex : xssRegexPatterns) {
        if (std::regex_search(content, regex)) {
            return true;
        }
    }
    
    return false;
}

} // namespace security
} // namespace browser