#include "cookie_security.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>

namespace browser {
namespace security {

//-----------------------------------------------------------------------------
// Cookie Implementation
//-----------------------------------------------------------------------------

Cookie::Cookie()
    : m_secure(false)
    , m_httpOnly(false)
    , m_sameSite("None")
{
    // Set default expiration to session (no explicit expiration)
    m_expiresAt = std::chrono::system_clock::time_point::max();
    
    // Set default path
    m_path = "/";
}

Cookie::Cookie(const std::string& name, const std::string& value)
    : m_name(name)
    , m_value(value)
    , m_secure(false)
    , m_httpOnly(false)
    , m_sameSite("None")
{
    // Set default expiration to session (no explicit expiration)
    m_expiresAt = std::chrono::system_clock::time_point::max();
    
    // Set default path
    m_path = "/";
}

Cookie::~Cookie() {
}

bool Cookie::isExpired() const {
    return std::chrono::system_clock::now() > m_expiresAt;
}

bool Cookie::appliesTo(const std::string& domain) const {
    // If cookie domain is empty, it's a host-only cookie
    if (m_domain.empty()) {
        return domain == m_domain;
    }
    
    // Domain matching as per RFC 6265
    if (m_domain[0] == '.') {
        // Remove leading dot for comparison
        std::string cookieDomain = m_domain.substr(1);
        
        // Domain suffix match
        if (domain.length() >= cookieDomain.length() &&
            domain.substr(domain.length() - cookieDomain.length()) == cookieDomain) {
            return true;
        }
    }
    else {
        // Exact match
        return domain == m_domain;
    }
    
    return false;
}

bool Cookie::appliesToPath(const std::string& path) const {
    // Path matching as per RFC 6265
    if (m_path.empty() || path.empty()) {
        return false;
    }
    
    if (m_path == path) {
        return true;
    }
    
    // Check if cookie path is a prefix of the requested path
    if (path.substr(0, m_path.length()) == m_path) {
        // The cookie path is a prefix of the request path
        if (m_path[m_path.length() - 1] == '/') {
            // If cookie path ends with '/', it's a direct match
            return true;
        }
        else if (path.length() > m_path.length() && path[m_path.length()] == '/') {
            // If request path continues with '/', it's a subdirectory
            return true;
        }
    }
    
    return false;
}

Cookie Cookie::parse(const std::string& cookieStr) {
    Cookie cookie;
    
    // Split the cookie string into name-value pair and attributes
    size_t firstSemicolon = cookieStr.find(';');
    
    if (firstSemicolon == std::string::npos) {
        // Only name=value, no attributes
        size_t equalsPos = cookieStr.find('=');
        if (equalsPos != std::string::npos) {
            cookie.setName(cookieStr.substr(0, equalsPos));
            cookie.setValue(cookieStr.substr(equalsPos + 1));
        }
    }
    else {
        // Parse name=value
        std::string nameValue = cookieStr.substr(0, firstSemicolon);
        size_t equalsPos = nameValue.find('=');
        if (equalsPos != std::string::npos) {
            cookie.setName(nameValue.substr(0, equalsPos));
            cookie.setValue(nameValue.substr(equalsPos + 1));
        }
        
        // Parse attributes
        std::string attributesStr = cookieStr.substr(firstSemicolon + 1);
        std::istringstream iss(attributesStr);
        std::string attribute;
        
        while (std::getline(iss, attribute, ';')) {
            // Trim whitespace
            attribute.erase(0, attribute.find_first_not_of(" \t"));
            attribute.erase(attribute.find_last_not_of(" \t") + 1);
            
            // Parse attribute
            size_t attrEqualsPos = attribute.find('=');
            if (attrEqualsPos != std::string::npos) {
                std::string name = attribute.substr(0, attrEqualsPos);
                std::string value = attribute.substr(attrEqualsPos + 1);
                
                // Convert name to lowercase
                std::transform(name.begin(), name.end(), name.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                
                if (name == "domain") {
                    cookie.setDomain(value);
                }
                else if (name == "path") {
                    cookie.setPath(value);
                }
                else if (name == "expires") {
                    // Parse expiration date
                    // In a real implementation, this would parse HTTP date format
                    // For demonstration, we'll set it to a year from now
                    cookie.setExpiresAt(std::chrono::system_clock::now() + std::chrono::hours(24 * 365));
                }
                else if (name == "max-age") {
                    // Parse max-age (seconds)
                    try {
                        int seconds = std::stoi(value);
                        cookie.setExpiresAt(std::chrono::system_clock::now() + std::chrono::seconds(seconds));
                    }
                    catch (const std::exception&) {
                        // Ignore invalid max-age
                    }
                }
                else if (name == "samesite") {
                    // Convert value to title case
                    if (!value.empty()) {
                        value[0] = std::toupper(value[0]);
                        for (size_t i = 1; i < value.length(); i++) {
                            value[i] = std::tolower(value[i]);
                        }
                        
                        // Validate SameSite value
                        if (value == "Strict" || value == "Lax" || value == "None") {
                            cookie.setSameSite(value);
                        }
                    }
                }
            }
            else {
                // Boolean attributes (Secure, HttpOnly)
                std::string name = attribute;
                
                // Convert name to lowercase
                std::transform(name.begin(), name.end(), name.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                
                if (name == "secure") {
                    cookie.setSecure(true);
                }
                else if (name == "httponly") {
                    cookie.setHttpOnly(true);
                }
            }
        }
    }
    
    return cookie;
}

std::string Cookie::toString() const {
    std::ostringstream oss;
    
    // Name=Value
    oss << m_name << "=" << m_value;
    
    // Domain
    if (!m_domain.empty()) {
        oss << "; Domain=" << m_domain;
    }
    
    // Path
    if (!m_path.empty()) {
        oss << "; Path=" << m_path;
    }
    
    // Expires
    if (m_expiresAt != std::chrono::system_clock::time_point::max()) {
        // In a real implementation, this would format the date according to HTTP date format
        oss << "; Expires=<date>";
    }
    
    // Secure flag
    if (m_secure) {
        oss << "; Secure";
    }
    
    // HttpOnly flag
    if (m_httpOnly) {
        oss << "; HttpOnly";
    }
    
    // SameSite flag
    if (!m_sameSite.empty() && m_sameSite != "None") {
        oss << "; SameSite=" << m_sameSite;
    }
    
    return oss.str();
}

//-----------------------------------------------------------------------------
// CookieJar Implementation
//-----------------------------------------------------------------------------

CookieJar::CookieJar() {
}

CookieJar::~CookieJar() {
}

void CookieJar::addCookie(const Cookie& cookie) {
    // Don't add expired cookies
    if (cookie.isExpired()) {
        return;
    }
    
    // Add or update cookie
    m_cookies[cookie.domain()][cookie.path()][cookie.name()] = cookie;
    
    // Clean up expired cookies periodically
    clearExpired();
}

std::vector<Cookie> CookieJar::getCookies(const std::string& domain, const std::string& path, bool secure) const {
    std::vector<Cookie> cookies;
    
    // Get all cookies that apply to the domain and path
    for (const auto& domainEntry : m_cookies) {
        Cookie tempCookie;
        if (domainEntry.first.empty() || (tempCookie.setDomain(domainEntry.first), tempCookie.appliesTo(domain))) {
            for (const auto& pathEntry : domainEntry.second) {
                Cookie pathCookie;
                if ((pathCookie.setDomain(domainEntry.first), pathCookie.setPath(pathEntry.first), pathCookie.appliesToPath(path))) {
                    for (const auto& nameEntry : pathEntry.second) {
                        const Cookie& cookie = nameEntry.second;
                        
                        // Check if cookie is expired
                        if (cookie.isExpired()) {
                            continue;
                        }
                        
                        // Check Secure flag
                        if (cookie.isSecure() && !secure) {
                            continue;
                        }
                        
                        cookies.push_back(cookie);
                    }
                }
            }
        }
    }
    
    return cookies;
}

Cookie CookieJar::getCookie(const std::string& domain, const std::string& path, const std::string& name, bool secure) const {
    // Get matching cookies
    std::vector<Cookie> cookies = getCookies(domain, path, secure);
    
    // Find cookie with the specified name
    for (const Cookie& cookie : cookies) {
        if (cookie.name() == name) {
            return cookie;
        }
    }
    
    // Return empty cookie if not found
    return Cookie();
}

void CookieJar::removeCookie(const std::string& domain, const std::string& path, const std::string& name) {
    // Find and remove the cookie
    auto domainIt = m_cookies.find(domain);
    if (domainIt != m_cookies.end()) {
        auto pathIt = domainIt->second.find(path);
        if (pathIt != domainIt->second.end()) {
            pathIt->second.erase(name);
            
            // Clean up empty containers
            if (pathIt->second.empty()) {
                domainIt->second.erase(pathIt);
                
                if (domainIt->second.empty()) {
                    m_cookies.erase(domainIt);
                }
            }
        }
    }
}

void CookieJar::clear() {
    m_cookies.clear();
}

void CookieJar::clearExpired() {
    // Iterate through all cookies and remove expired ones
    for (auto domainIt = m_cookies.begin(); domainIt != m_cookies.end();) {
        for (auto pathIt = domainIt->second.begin(); pathIt != domainIt->second.end();) {
            for (auto nameIt = pathIt->second.begin(); nameIt != pathIt->second.end();) {
                if (nameIt->second.isExpired()) {
                    nameIt = pathIt->second.erase(nameIt);
                }
                else {
                    ++nameIt;
                }
            }
            
            if (pathIt->second.empty()) {
                pathIt = domainIt->second.erase(pathIt);
            }
            else {
                ++pathIt;
            }
        }
        
        if (domainIt->second.empty()) {
            domainIt = m_cookies.erase(domainIt);
        }
        else {
            ++domainIt;
        }
    }
}

//-----------------------------------------------------------------------------
// CookieSecurityManager Implementation
//-----------------------------------------------------------------------------

CookieSecurityManager::CookieSecurityManager() {
}

CookieSecurityManager::~CookieSecurityManager() {
}

bool CookieSecurityManager::initialize() {
    // No initialization needed for now
    return true;
}

bool CookieSecurityManager::canSetCookie(const Cookie& cookie, const Origin& origin, bool isSecureContext) const {
    // Check if cookie is secure and context is not secure
    if (cookie.isSecure() && !isSecureContext) {
        return false;
    }
    
    // Check if cookie domain is allowed for this origin
    if (!cookie.domain().empty() && !cookie.appliesTo(origin.host())) {
        return false;
    }
    
    // Check for SameSite=None without Secure
    if (cookie.sameSite() == "None" && !cookie.isSecure()) {
        return false;
    }
    
    return true;
}

bool CookieSecurityManager::canReadCookie(const Cookie& cookie, const Origin& requestOrigin, const Origin& cookieOrigin) const {
    // Check same-origin policy
    if (requestOrigin != cookieOrigin) {
        // Cross-origin request, check SameSite
        if (!isSameSiteAllowed(cookie.sameSite(), requestOrigin, cookieOrigin)) {
            return false;
        }
    }
    
    // Check if cookie is secure and the request is not over HTTPS
    if (cookie.isSecure() && requestOrigin.scheme() != "https") {
        return false;
    }
    
    return true;
}

Cookie CookieSecurityManager::applySecurity(const Cookie& cookie, const Origin& origin) const {
    Cookie securedCookie = cookie;
    
    // Set Secure flag for cookies on secure domains
    if (isSecureDomain(origin.host())) {
        securedCookie.setSecure(true);
    }
    
    // Set SameSite=Lax by default if not specified
    if (securedCookie.sameSite().empty()) {
        securedCookie.setSameSite("Lax");
    }
    
    // SameSite=None requires Secure
    if (securedCookie.sameSite() == "None") {
        securedCookie.setSecure(true);
    }
    
    // Set HttpOnly flag for sensitive cookies
    // In a real implementation, this would be more sophisticated
    if (securedCookie.name() == "session" || 
        securedCookie.name() == "sessionid" || 
        securedCookie.name() == "auth" || 
        securedCookie.name() == "csrf_token") {
        securedCookie.setHttpOnly(true);
    }
    
    return securedCookie;
}

Cookie CookieSecurityManager::parseAndSecure(const std::string& setCookieHeader, const Origin& origin) const {
    // Parse the Set-Cookie header
    Cookie cookie = Cookie::parse(setCookieHeader);
    
    // Apply security recommendations
    cookie = applySecurity(cookie, origin);
    
    return cookie;
}

bool CookieSecurityManager::isSecureDomain(const std::string& domain) const {
    // List of secure domains that should always use Secure flag
    static const std::vector<std::string> secureDomains = {
        "login", "secure", "accounts", "auth", "banking", "pay", "checkout"
    };
    
    // Check if domain contains any secure keywords
    for (const auto& secureDomain : secureDomains) {
        if (domain.find(secureDomain) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool CookieSecurityManager::isSameSiteAllowed(const std::string& sameSiteMode, const Origin& requestOrigin, const Origin& cookieOrigin) const {
    // Same-site checks as per SameSite cookie specification
    
    if (sameSiteMode == "None") {
        // SameSite=None allows cross-site requests
        return true;
    }
    
    if (sameSiteMode == "Lax") {
        // SameSite=Lax allows some cross-site requests (e.g., navigations)
        // In a real implementation, this would check the request context
        return true;
    }
    
    if (sameSiteMode == "Strict") {
        // SameSite=Strict allows only same-site requests
        return requestOrigin == cookieOrigin;
    }
    
    // Default to Lax behavior
    return true;
}

} // namespace security
} // namespace browser