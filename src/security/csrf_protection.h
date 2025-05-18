
#ifndef BROWSER_CSRF_PROTECTION_H
#define BROWSER_CSRF_PROTECTION_H

#include <string>
#include <map>
#include <set>
#include <chrono>
#include <random>
#include "same_origin.h"

namespace browser {
namespace security {

// CSRF protection modes
enum class CsrfProtectionMode {
    DISABLED,               // CSRF protection disabled
    SAME_ORIGIN,            // Same-origin policy check only
    ORIGIN_HEADER,          // Check Origin header
    REFERER_HEADER,         // Check Referer header
    CSRF_TOKEN,             // Use CSRF tokens
    DOUBLE_SUBMIT_COOKIE    // Use double submit cookie pattern
};

// CSRF Token class for generating and validating tokens
class CsrfToken {
public:
    CsrfToken();
    ~CsrfToken();
    
    // Generate a new token
    std::string generate();
    
    // Validate a token against a stored value
    bool validate(const std::string& token, const std::string& expectedToken) const;
    
    // Get a previously generated token
    std::string getToken() const { return m_token; }
    
private:
    std::string m_token;
    std::mt19937 m_generator;
};

// CSRF Protection class
class CsrfProtection {
public:
    CsrfProtection();
    ~CsrfProtection();
    
    // Initialize CSRF protection
    bool initialize();
    
    // Set the protection mode
    void setMode(CsrfProtectionMode mode) { m_mode = mode; }
    
    // Get the current protection mode
    CsrfProtectionMode mode() const { return m_mode; }
    
    // Check if a request is potentially CSRF
    bool isPotentialCsrf(const std::string& method, const std::string& targetUrl, 
                         const std::map<std::string, std::string>& headers,
                         const std::map<std::string, std::string>& cookies,
                         const std::map<std::string, std::string>& formParams) const;
    
    // Generate a CSRF token
    std::string generateToken();
    
    // Validate a CSRF token
    bool validateToken(const std::string& token, const std::string& expectedToken) const;
    
    // Add a trusted origin that can send cross-origin requests
    void addTrustedOrigin(const Origin& origin);
    
    // Check if an origin is trusted
    bool isTrustedOrigin(const Origin& origin) const;
    
private:
    CsrfProtectionMode m_mode;
    CsrfToken m_tokenGenerator;
    std::set<std::string> m_trustedOrigins;
    
    // Helper methods for CSRF checks
    bool checkSameOrigin(const std::string& targetUrl, const std::map<std::string, std::string>& headers) const;
    bool checkOriginHeader(const std::string& targetUrl, const std::map<std::string, std::string>& headers) const;
    bool checkRefererHeader(const std::string& targetUrl, const std::map<std::string, std::string>& headers) const;
    bool checkCsrfToken(const std::map<std::string, std::string>& formParams) const;
    bool checkDoubleSubmitCookie(const std::map<std::string, std::string>& cookies, 
                                const std::map<std::string, std::string>& formParams) const;
};

} // namespace security
} // namespace browser

#endif // BROWSER_CSRF_PROTECTION_H