#ifndef BROWSER_SAME_ORIGIN_H
#define BROWSER_SAME_ORIGIN_H

#include <string>
#include <map>
#include <memory>

namespace browser {
namespace security {

// Represents a web origin as defined by the same-origin policy
// An origin is defined by the triplet (scheme, host, port)
class Origin {
public:
    Origin(const std::string& scheme, const std::string& host, int port);
    Origin(const std::string& url); // Construct from URL string
    
    // Get the components of the origin
    std::string scheme() const { return m_scheme; }
    std::string host() const { return m_host; }
    int port() const { return m_port; }
    
    // Get the serialized origin string
    std::string toString() const;
    
    // Compare origins
    bool equals(const Origin& other) const;
    bool operator==(const Origin& other) const;
    bool operator!=(const Origin& other) const;
    
    // Check if this is a null origin (represents an opaque origin)
    bool isNull() const;
    
    // Predefined origins
    static Origin null();  // The "null" origin
    static Origin opaque(); // An opaque unique origin
    
private:
    std::string m_scheme;
    std::string m_host;
    int m_port;
    
    // Normalize the origin components
    void normalize();
};

// Same-origin policy enforcer
class SameOriginPolicy {
public:
    SameOriginPolicy();
    ~SameOriginPolicy();
    
    // Check if a cross-origin request is allowed
    bool canAccess(const Origin& source, const Origin& target) const;
    
    // Check if source origin can send request to target origin
    bool canSendRequest(const Origin& source, const Origin& target) const;
    
    // Check if source origin can receive response from target origin
    bool canReceiveResponse(const Origin& source, const Origin& target) const;
    
    // Check if source can read cookies from target origin
    bool canReadCookies(const Origin& source, const Origin& target) const;
    
    // Check if source can execute script from target origin
    bool canExecuteScript(const Origin& source, const Origin& target) const;
    
    // Add an origin to the trusted list (for development/testing)
    void addTrustedOrigin(const Origin& origin);
    
    // Check if origin is trusted
    bool isTrustedOrigin(const Origin& origin) const;
    
private:
    // List of trusted origins (for development/testing)
    std::map<std::string, bool> m_trustedOrigins;
};

} // namespace security
} // namespace browser

#endif // BROWSER_SAME_ORIGIN_H