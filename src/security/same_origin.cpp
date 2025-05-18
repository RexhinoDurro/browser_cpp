#include "same_origin.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace browser {
namespace security {

//-----------------------------------------------------------------------------
// Origin Implementation
//-----------------------------------------------------------------------------

Origin::Origin(const std::string& scheme, const std::string& host, int port)
    : m_scheme(scheme)
    , m_host(host)
    , m_port(port)
{
    normalize();
}

Origin::Origin(const std::string& url) 
    : m_scheme("")
    , m_host("")
    , m_port(0)
{
    // Parse URL to extract origin components
    if (url.empty()) {
        *this = null();
        return;
    }
    
    // Extract scheme
    size_t schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos) {
        *this = null();
        return;
    }
    
    m_scheme = url.substr(0, schemeEnd);
    
    // Extract host and port
    size_t hostStart = schemeEnd + 3;
    size_t hostEnd = url.find('/', hostStart);
    if (hostEnd == std::string::npos) {
        hostEnd = url.length();
    }
    
    std::string hostAndPort = url.substr(hostStart, hostEnd - hostStart);
    
    // Check for port specification
    size_t portPos = hostAndPort.find(':');
    if (portPos != std::string::npos) {
        m_host = hostAndPort.substr(0, portPos);
        
        try {
            m_port = std::stoi(hostAndPort.substr(portPos + 1));
        }
        catch (const std::exception&) {
            // Default to standard ports if parsing fails
            if (m_scheme == "http") {
                m_port = 80;
            }
            else if (m_scheme == "https") {
                m_port = 443;
            }
        }
    }
    else {
        m_host = hostAndPort;
        
        // Set default ports
        if (m_scheme == "http") {
            m_port = 80;
        }
        else if (m_scheme == "https") {
            m_port = 443;
        }
    }
    
    normalize();
}

std::string Origin::toString() const {
    if (isNull()) {
        return "null";
    }
    
    std::ostringstream oss;
    oss << m_scheme << "://" << m_host;
    
    // Only include port if it's non-standard
    if ((m_scheme == "http" && m_port != 80) || 
        (m_scheme == "https" && m_port != 443)) {
        oss << ":" << m_port;
    }
    
    return oss.str();
}

bool Origin::equals(const Origin& other) const {
    return m_scheme == other.m_scheme && 
           m_host == other.m_host && 
           m_port == other.m_port;
}

bool Origin::operator==(const Origin& other) const {
    return equals(other);
}

bool Origin::operator!=(const Origin& other) const {
    return !equals(other);
}

bool Origin::isNull() const {
    return m_scheme.empty() && m_host.empty() && m_port == 0;
}

Origin Origin::null() {
    return Origin("", "", 0);
}

Origin Origin::opaque() {
    // Create a unique opaque origin
    static int uniqueId = 0;
    return Origin("opaque", "unique-origin-" + std::to_string(++uniqueId), 0);
}

void Origin::normalize() {
    // Convert scheme to lowercase
    std::transform(m_scheme.begin(), m_scheme.end(), m_scheme.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Convert host to lowercase
    std::transform(m_host.begin(), m_host.end(), m_host.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Handle default ports
    if (m_scheme == "http" && m_port == 0) {
        m_port = 80;
    }
    else if (m_scheme == "https" && m_port == 0) {
        m_port = 443;
    }
}

//-----------------------------------------------------------------------------
// SameOriginPolicy Implementation
//-----------------------------------------------------------------------------

SameOriginPolicy::SameOriginPolicy() {
}

SameOriginPolicy::~SameOriginPolicy() {
}

bool SameOriginPolicy::canAccess(const Origin& source, const Origin& target) const {
    // Check for trusted origins first
    if (isTrustedOrigin(source) || isTrustedOrigin(target)) {
        return true;
    }
    
    // Same-origin check
    return source == target;
}

bool SameOriginPolicy::canSendRequest(const Origin& source, const Origin& target) const {
    // Web browsers generally allow sending cross-origin requests, but restrict reading the response
    return true;
}

bool SameOriginPolicy::canReceiveResponse(const Origin& source, const Origin& target) const {
    // Check if source can access target
    return canAccess(source, target);
}

bool SameOriginPolicy::canReadCookies(const Origin& source, const Origin& target) const {
    // Cookies are strictly same-origin by default
    return canAccess(source, target);
}

bool SameOriginPolicy::canExecuteScript(const Origin& source, const Origin& target) const {
    // Script execution is restricted to same origin
    return canAccess(source, target);
}

void SameOriginPolicy::addTrustedOrigin(const Origin& origin) {
    m_trustedOrigins[origin.toString()] = true;
}

bool SameOriginPolicy::isTrustedOrigin(const Origin& origin) const {
    return m_trustedOrigins.find(origin.toString()) != m_trustedOrigins.end();
}

} // namespace security
} // namespace browser