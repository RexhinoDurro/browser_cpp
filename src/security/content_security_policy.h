#ifndef BROWSER_CONTENT_SECURITY_POLICY_H
#define BROWSER_CONTENT_SECURITY_POLICY_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include "same_origin.h"

namespace browser {
namespace security {

// Source enumeration for CSP directives
enum class CspSource {
    NONE,           // 'none'
    SELF,           // 'self'
    UNSAFE_INLINE,  // 'unsafe-inline'
    UNSAFE_EVAL,    // 'unsafe-eval'
    STRICT_DYNAMIC, // 'strict-dynamic'
    CUSTOM          // Custom source (e.g., domain name)
};

// Resource type enumeration for CSP directives
enum class CspResourceType {
    DEFAULT,    // default-src
    SCRIPT,     // script-src
    STYLE,      // style-src
    IMG,        // img-src
    CONNECT,    // connect-src
    FONT,       // font-src
    MEDIA,      // media-src
    OBJECT,     // object-src
    MANIFEST,   // manifest-src
    FRAME,      // frame-src
    WORKER      // worker-src
};

// Content Security Policy directive
class CspDirective {
public:
    CspDirective(CspResourceType type);
    
    // Add a source to this directive
    void addSource(CspSource source, const std::string& value = "");
    
    // Check if a source is allowed by this directive
    bool allowsSource(const std::string& source, const Origin& pageOrigin) const;
    
    // Check if inline scripts/styles are allowed
    bool allowsInline() const;
    
    // Check if eval() is allowed
    bool allowsEval() const;
    
    // Get the resource type
    CspResourceType type() const { return m_type; }
    
    // Get the sources for this directive
    const std::vector<std::pair<CspSource, std::string>>& sources() const { return m_sources; }
    
private:
    CspResourceType m_type;
    std::vector<std::pair<CspSource, std::string>> m_sources;
    
    // Helper methods
    bool sourceMatches(const std::string& source, const Origin& pageOrigin, CspSource directiveSource, const std::string& directiveValue) const;
};

// Content Security Policy class
class ContentSecurityPolicy {
public:
    ContentSecurityPolicy();
    ~ContentSecurityPolicy();
    
    // Parse a CSP header value
    bool parse(const std::string& policyHeader);
    
    // Check if a resource is allowed by the policy
    bool allowsResource(CspResourceType type, const std::string& source, const Origin& pageOrigin) const;
    
    // Check if inline scripts are allowed
    bool allowsInlineScript() const;
    
    // Check if inline styles are allowed
    bool allowsInlineStyle() const;
    
    // Check if eval() is allowed
    bool allowsEval() const;
    
    // Get a string representation of the policy
    std::string toString() const;
    
private:
    // Map of resource types to directives
    std::map<CspResourceType, std::shared_ptr<CspDirective>> m_directives;
    
    // Default directive (default-src)
    std::shared_ptr<CspDirective> m_defaultDirective;
    
    // Helper methods
    void parseDirective(const std::string& directive);
    CspResourceType resourceTypeFromString(const std::string& typeStr) const;
    std::pair<CspSource, std::string> parseSource(const std::string& source) const;
};

} // namespace security
} // namespace browser

#endif // BROWSER_CONTENT_SECURITY_POLICY_H