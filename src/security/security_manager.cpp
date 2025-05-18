#include "security_manager.h"
#include <iostream>
#include <ranges>
#include <algorithm>

namespace browser {
namespace security {

//-----------------------------------------------------------------------------
// SecurityManager Implementation
//-----------------------------------------------------------------------------

SecurityManager::SecurityManager() {
}

SecurityManager::~SecurityManager() {
}

bool SecurityManager::initialize() {
    // Initialize all security components
    
    if (!m_certificateValidator.initialize()) {
        std::cerr << "Failed to initialize certificate validator" << std::endl;
        return false;
    }
    
    if (!m_xssProtection.initialize()) {
        std::cerr << "Failed to initialize XSS protection" << std::endl;
        return false;
    }
    
    if (!m_csrfProtection.initialize()) {
        std::cerr << "Failed to initialize CSRF protection" << std::endl;
        return false;
    }
    
    if (!m_cookieSecurityManager.initialize()) {
        std::cerr << "Failed to initialize cookie security manager" << std::endl;
        return false;
    }
    
    return true;
}

bool SecurityManager::canMakeRequest(const Origin& source, const Origin& target, 
                                  const std::string& method, const std::string& contentType) const {
    // Check same-origin policy
    if (!m_sameOriginPolicy.canSendRequest(source, target)) {
        return false;
    }
    
    // Additional checks for cross-origin requests
    if (source != target) {
        // CORS would be checked here in a real implementation
        // For demonstration, we'll allow common methods and content types
        
        // Convert method to uppercase
        std::string upperMethod = method;
        std::transform(upperMethod.begin(), upperMethod.end(), upperMethod.begin(),
                      [](unsigned char c) { return std::toupper(c); });
        
        // Allow simple methods
        if (upperMethod != "GET" && upperMethod != "HEAD" && upperMethod != "POST") {
            return false;
        }
        
        // Allow simple content types
        if (!contentType.empty() && 
            contentType != "application/x-www-form-urlencoded" && 
            contentType != "multipart/form-data" && 
            contentType != "text/plain") {
            return false;
        }
    }
    
    return true;
}

bool SecurityManager::isAllowedByCSP(const std::string& url, CspResourceType type, const Origin& pageOrigin) const {
    return m_contentSecurityPolicy.allowsResource(type, url, pageOrigin);
}

CertificateVerificationResult SecurityManager::verifyCertificate(const CertificateChain& chain, 
                                                             const std::string& hostname) const {
    return m_certificateValidator.verify(chain, hostname);
}

bool SecurityManager::isPotentialXSS(const std::string& html) const {
    return m_xssProtection.isHtmlPotentialXss(html);
}

std::string SecurityManager::sanitizeHtml(const std::string& html, SanitizationLevel level) const {
    return m_xssProtection.sanitizeHtml(html, level);
}

bool SecurityManager::isPotentialCSRF(const std::string& method, const std::string& targetUrl, 
                                   const std::map<std::string, std::string>& headers,
                                   const std::map<std::string, std::string>& cookies,
                                   const std::map<std::string, std::string>& formParams) const {
    return m_csrfProtection.isPotentialCsrf(method, targetUrl, headers, cookies, formParams);
}

bool SecurityManager::canSetCookie(const Cookie& cookie, const Origin& origin, bool isSecureContext) const {
    return m_cookieSecurityManager.canSetCookie(cookie, origin, isSecureContext);
}

Cookie SecurityManager::secureCookie(const Cookie& cookie, const Origin& origin) const {
    return m_cookieSecurityManager.applySecurity(cookie, origin);
}

} // namespace security
} // namespace browser