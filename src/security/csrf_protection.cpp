#include "csrf_protection.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>

namespace browser {
namespace security {

//-----------------------------------------------------------------------------
// CsrfToken Implementation
//-----------------------------------------------------------------------------

CsrfToken::CsrfToken() {
    // Initialize random number generator with a random device
    std::random_device rd;
    m_generator.seed(rd());
}

CsrfToken::~CsrfToken() {
}

std::string CsrfToken::generate() {
    // Generate a random token
    
    // Use a cryptographically secure random generator in a real implementation
    // For demonstration, we'll use a simple approach
    
    std::stringstream ss;
    
    // Generate a 32-byte random token
    for (int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (m_generator() & 0xFF);
    }
    
    m_token = ss.str();
    return m_token;
}

bool CsrfToken::validate(const std::string& token, const std::string& expectedToken) const {
    // In a real implementation, this would use constant-time comparison
    // to avoid timing attacks
    
    if (token.empty() || expectedToken.empty()) {
        return false;
    }
    
    // Simple comparison for demonstration
    return token == expectedToken;
}

//-----------------------------------------------------------------------------
// CsrfProtection Implementation
//-----------------------------------------------------------------------------

CsrfProtection::CsrfProtection()
    : m_mode(CsrfProtectionMode::SAME_ORIGIN)
{
}

CsrfProtection::~CsrfProtection() {
}

bool CsrfProtection::initialize() {
    // No initialization needed for now
    return true;
}

bool CsrfProtection::isPotentialCsrf(const std::string& method, const std::string& targetUrl, 
                                  const std::map<std::string, std::string>& headers,
                                  const std::map<std::string, std::string>& cookies,
                                  const std::map<std::string, std::string>& formParams) const {
    // CSRF primarily affects state-changing requests (POST, PUT, DELETE)
    // GET requests should be safe if properly implemented (idempotent and side-effect free)
    
    // Convert method to uppercase
    std::string upperMethod = method;
    std::transform(upperMethod.begin(), upperMethod.end(), upperMethod.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    
    // Safe methods do not need CSRF protection
    if (upperMethod == "GET" || upperMethod == "HEAD" || upperMethod == "OPTIONS") {
        return false;
    }
    
    // Check based on the protection mode
    switch (m_mode) {
        case CsrfProtectionMode::DISABLED:
            return false;
            
        case CsrfProtectionMode::SAME_ORIGIN:
            return !checkSameOrigin(targetUrl, headers);
            
        case CsrfProtectionMode::ORIGIN_HEADER:
            return !checkOriginHeader(targetUrl, headers);
            
        case CsrfProtectionMode::REFERER_HEADER:
            return !checkRefererHeader(targetUrl, headers);
            
        case CsrfProtectionMode::CSRF_TOKEN:
            return !checkCsrfToken(formParams);
            
        case CsrfProtectionMode::DOUBLE_SUBMIT_COOKIE:
            return !checkDoubleSubmitCookie(cookies, formParams);
            
        default:
            return true; // Unknown mode, assume potential CSRF
    }
}

std::string CsrfProtection::generateToken() {
    return m_tokenGenerator.generate();
}

bool CsrfProtection::validateToken(const std::string& token, const std::string& expectedToken) const {
    return m_tokenGenerator.validate(token, expectedToken);
}

void CsrfProtection::addTrustedOrigin(const Origin& origin) {
    m_trustedOrigins.insert(origin.toString());
}

bool CsrfProtection::isTrustedOrigin(const Origin& origin) const {
    return m_trustedOrigins.find(origin.toString()) != m_trustedOrigins.end();
}

bool CsrfProtection::checkSameOrigin(const std::string& targetUrl, const std::map<std::string, std::string>& headers) const {
    // Same-origin policy check
    // This requires the Origin header to be present and match the target URL
    
    // Find Origin header
    auto originIt = headers.find("Origin");
    if (originIt == headers.end()) {
        // No Origin header, check Referer as fallback
        return checkRefererHeader(targetUrl, headers);
    }
    
    // Parse origins
    Origin origin(originIt->second);
    Origin target(targetUrl);
    
    // Check if origin is trusted
    if (isTrustedOrigin(origin)) {
        return true;
    }
    
    // Check if same origin
    return origin == target;
}

bool CsrfProtection::checkOriginHeader(const std::string& targetUrl, const std::map<std::string, std::string>& headers) const {
    // Origin header check
    // This requires the Origin header to be present and match the target URL
    
    // Find Origin header
    auto originIt = headers.find("Origin");
    if (originIt == headers.end()) {
        // No Origin header, this is suspicious
        return false;
    }
    
    // Parse origins
    Origin origin(originIt->second);
    Origin target(targetUrl);
    
    // Check if origin is trusted
    if (isTrustedOrigin(origin)) {
        return true;
    }
    
    // Check if same origin
    return origin == target;
}

bool CsrfProtection::checkRefererHeader(const std::string& targetUrl, const std::map<std::string, std::string>& headers) const {
    // Referer header check
    // This requires the Referer header to be present and match the target URL
    
    // Find Referer header
    auto refererIt = headers.find("Referer");
    if (refererIt == headers.end()) {
        // No Referer header, this is suspicious
        return false;
    }
    
    // Parse origins
    Origin referer(refererIt->second);
    Origin target(targetUrl);
    
    // Check if referer is trusted
    if (isTrustedOrigin(referer)) {
        return true;
    }
    
    // Check if same origin
    return referer == target;
}

bool CsrfProtection::checkCsrfToken(const std::map<std::string, std::string>& formParams) const {
    // CSRF token check
    // This requires a CSRF token to be present in the form parameters
    
    // Find CSRF token
    auto tokenIt = formParams.find("csrf_token");
    if (tokenIt == formParams.end()) {
        // No CSRF token, this is suspicious
        return false;
    }
    
    // In a real implementation, we would validate the token against a stored value
    // For demonstration, we'll just check if it's non-empty
    return !tokenIt->second.empty();
}

bool CsrfProtection::checkDoubleSubmitCookie(const std::map<std::string, std::string>& cookies, 
                                          const std::map<std::string, std::string>& formParams) const {
    // Double submit cookie check
    // This requires a CSRF token to be present in both cookies and form parameters, and they must match
    
    // Find CSRF tokens
    auto cookieTokenIt = cookies.find("csrf_token");
    auto formTokenIt = formParams.find("csrf_token");
    
    if (cookieTokenIt == cookies.end() || formTokenIt == formParams.end()) {
        // Missing token in either cookies or form parameters
        return false;
    }
    
    // Tokens must match
    return cookieTokenIt->second == formTokenIt->second;
}

} // namespace security
} // namespace browser