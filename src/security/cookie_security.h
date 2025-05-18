#ifndef BROWSER_COOKIE_SECURITY_H
#define BROWSER_COOKIE_SECURITY_H

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include "same_origin.h"

namespace browser {
namespace security {

// Cookie security flags
struct CookieSecurityFlags {
    bool secure;       // Secure flag (HTTPS only)
    bool httpOnly;     // HttpOnly flag (not accessible via JavaScript)
    bool sameSite;     // SameSite flag (restricts cross-site requests)
    std::string sameSiteMode; // SameSite mode: "Strict", "Lax", or "None"
};

// Cookie class
class Cookie {
public:
    Cookie();
    Cookie(const std::string& name, const std::string& value);
    ~Cookie();
    
    // Getters and setters
    std::string name() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    std::string value() const { return m_value; }
    void setValue(const std::string& value) { m_value = value; }
    
    std::string domain() const { return m_domain; }
    void setDomain(const std::string& domain) { m_domain = domain; }
    
    std::string path() const { return m_path; }
    void setPath(const std::string& path) { m_path = path; }
    
    bool isSecure() const { return m_secure; }
    void setSecure(bool secure) { m_secure = secure; }
    
    bool isHttpOnly() const { return m_httpOnly; }
    void setHttpOnly(bool httpOnly) { m_httpOnly = httpOnly; }
    
    std::string sameSite() const { return m_sameSite; }
    void setSameSite(const std::string& sameSite) { m_sameSite = sameSite; }
    
    std::chrono::system_clock::time_point expiresAt() const { return m_expiresAt; }
    void setExpiresAt(const std::chrono::system_clock::time_point& expiresAt) { m_expiresAt = expiresAt; }
    
    // Check if cookie is expired
    bool isExpired() const;
    
    // Check if cookie applies to a domain
    bool appliesTo(const std::string& domain) const;
    
    // Check if cookie applies to a path
    bool appliesToPath(const std::string& path) const;
    
    // Parse a cookie string
    static Cookie parse(const std::string& cookieStr);
    
    // Serialize cookie to string
    std::string toString() const;
    
private:
    std::string m_name;
    std::string m_value;
    std::string m_domain;
    std::string m_path;
    bool m_secure;
    bool m_httpOnly;
    std::string m_sameSite;
    std::chrono::system_clock::time_point m_expiresAt;
};

// Cookie Jar class
class CookieJar {
public:
    CookieJar();
    ~CookieJar();
    
    // Add a cookie
    void addCookie(const Cookie& cookie);
    
    // Get all cookies for a specific domain and path
    std::vector<Cookie> getCookies(const std::string& domain, const std::string& path, bool secure = false) const;
    
    // Get a specific cookie by name
    Cookie getCookie(const std::string& domain, const std::string& path, const std::string& name, bool secure = false) const;
    
    // Remove a cookie
    void removeCookie(const std::string& domain, const std::string& path, const std::string& name);
    
    // Clear all cookies
    void clear();
    
    // Clear expired cookies
    void clearExpired();
    
private:
    // Map of domain -> path -> name -> cookie
    std::map<std::string, std::map<std::string, std::map<std::string, Cookie>>> m_cookies;
};

// Cookie Security Manager
class CookieSecurityManager {
public:
    CookieSecurityManager();
    ~CookieSecurityManager();
    
    // Initialize the cookie security manager
    bool initialize();
    
    // Check if a cookie is allowed to be set
    bool canSetCookie(const Cookie& cookie, const Origin& origin, bool isSecureContext) const;
    
    // Check if a cookie can be read
    bool canReadCookie(const Cookie& cookie, const Origin& requestOrigin, const Origin& cookieOrigin) const;
    
    // Apply security recommendations to a cookie
    Cookie applySecurity(const Cookie& cookie, const Origin& origin) const;
    
    // Parse Set-Cookie header and apply security
    Cookie parseAndSecure(const std::string& setCookieHeader, const Origin& origin) const;
    
private:
    // Helper methods
    bool isSecureDomain(const std::string& domain) const;
    bool isSameSiteAllowed(const std::string& sameSiteMode, const Origin& requestOrigin, const Origin& cookieOrigin) const;
};

} // namespace security
} // namespace browser

#endif // BROWSER_COOKIE_SECURITY_H