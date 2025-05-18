#ifndef BROWSER_XSS_PROTECTION_H
#define BROWSER_XSS_PROTECTION_H

#include <string>
#include <vector>
#include <map>
#include <set>

namespace browser {
namespace security {

// XSS Protection mode
enum class XssProtectionMode {
    DISABLED,       // XSS protection disabled
    ENABLED,        // XSS protection enabled
    BLOCK,          // XSS protection enabled and blocks rendering of page
    REPORT          // XSS protection enabled and reports violations (CSP-like)
};

// Content sanitization level
enum class SanitizationLevel {
    NONE,           // No sanitization
    BASIC,          // Basic sanitization (remove dangerous tags/attributes)
    STRICT,         // Strict sanitization (whitelist approach)
    ESCAPE_ONLY     // Only escape HTML special characters
};

// XSS Protection class
class XssProtection {
public:
    XssProtection();
    ~XssProtection();
    
    // Initialize XSS protection
    bool initialize();
    
    // Set the XSS protection mode
    void setMode(XssProtectionMode mode) { m_mode = mode; }
    
    // Get the current protection mode
    XssProtectionMode mode() const { return m_mode; }
    
    // Set the sanitization level
    void setSanitizationLevel(SanitizationLevel level) { m_sanitizationLevel = level; }
    
    // Get the current sanitization level
    SanitizationLevel sanitizationLevel() const { return m_sanitizationLevel; }
    
    // Set the report URL for reporting XSS attempts
    void setReportUrl(const std::string& url) { m_reportUrl = url; }
    
    // Parse X-XSS-Protection header
    bool parseHeader(const std::string& headerValue);
    
    // Check if a URL might contain XSS
    bool isUrlPotentialXss(const std::string& url) const;
    
    // Check if HTML content might contain XSS
    bool isHtmlPotentialXss(const std::string& html) const;
    
    // Sanitize HTML content to remove potential XSS
    std::string sanitizeHtml(const std::string& html, SanitizationLevel level = SanitizationLevel::BASIC) const;
    
    // Sanitize a URL to remove potential XSS
    std::string sanitizeUrl(const std::string& url) const;
    
    // Escape HTML special characters
    std::string escapeHtml(const std::string& input) const;
    
private:
    XssProtectionMode m_mode;
    SanitizationLevel m_sanitizationLevel;
    std::string m_reportUrl;
    
    // Whitelists for HTML sanitization
    std::set<std::string> m_allowedTags;
    std::map<std::string, std::set<std::string>> m_allowedAttributes;
    std::set<std::string> m_allowedProtocols;
    
    // Initialize whitelists
    void initializeWhitelists();
    
    // Helper methods for HTML sanitization
    bool isAllowedTag(const std::string& tag) const;
    bool isAllowedAttribute(const std::string& tag, const std::string& attribute) const;
    bool isAllowedProtocol(const std::string& protocol) const;
    
    // Detect XSS attack patterns
    bool detectXssPatterns(const std::string& content) const;
};

} // namespace security
} // namespace browser

#endif // BROWSER_XSS_PROTECTION_H