#ifndef BROWSER_SECURITY_MANAGER_H
#define BROWSER_SECURITY_MANAGER_H

#include "same_origin.h"
#include "xss_protection.h"
#include "csrf_protection.h"
#include "certificate_validator.h"
#include "content_security_policy.h"
#include "cookie_security.h"
#include <string>
#include <map>

namespace browser {
namespace security {

// Security manager that coordinates all security components
class SecurityManager {
public:
    SecurityManager();
    ~SecurityManager();
    
    // Initialize the security manager
    bool initialize();
    
    // Get security components
    SameOriginPolicy* sameOriginPolicy() { return &m_sameOriginPolicy; }
    XssProtection* xssProtection() { return &m_xssProtection; }
    CsrfProtection* csrfProtection() { return &m_csrfProtection; }
    ContentSecurityPolicy* contentSecurityPolicy() { return &m_contentSecurityPolicy; }
    CertificateValidator* certificateValidator() { return &m_certificateValidator; }
    CookieSecurityManager* cookieSecurityManager() { return &m_cookieSecurityManager; }
    
    // Security checks
    
    // Check if a request can be made based on security policies
    bool canMakeRequest(const Origin& source, const Origin& target, 
                       const std::string& method, const std::string& contentType) const;
    
    // Check if a resource is allowed by Content Security Policy
    bool isAllowedByCSP(const std::string& url, CspResourceType type, const Origin& pageOrigin) const;
    
    // Verify TLS certificate
    CertificateVerificationResult verifyCertificate(const CertificateChain& chain, 
                                                  const std::string& hostname) const;
    
    // Check for potential XSS
    bool isPotentialXSS(const std::string& html) const;
    
    // Sanitize HTML
    std::string sanitizeHtml(const std::string& html, SanitizationLevel level = SanitizationLevel::BASIC) const;
    
    // Check for potential CSRF
    bool isPotentialCSRF(const std::string& method, const std::string& targetUrl, 
                        const std::map<std::string, std::string>& headers,
                        const std::map<std::string, std::string>& cookies,
                        const std::map<std::string, std::string>& formParams) const;
    
    // Cookie security
    bool canSetCookie(const Cookie& cookie, const Origin& origin, bool isSecureContext) const;
    Cookie secureCookie(const Cookie& cookie, const Origin& origin) const;
    
private:
    // Security components
    SameOriginPolicy m_sameOriginPolicy;
    XssProtection m_xssProtection;
    CsrfProtection m_csrfProtection;
    ContentSecurityPolicy m_contentSecurityPolicy;
    CertificateValidator m_certificateValidator;
    CookieSecurityManager m_cookieSecurityManager;
};

} // namespace security
} // namespace browser

#endif // BROWSER_SECURITY_MANAGER_H