#include "content_security_policy.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>

namespace browser {
namespace security {

//-----------------------------------------------------------------------------
// CspDirective Implementation
//-----------------------------------------------------------------------------

CspDirective::CspDirective(CspResourceType type)
    : m_type(type)
{
}

void CspDirective::addSource(CspSource source, const std::string& value) {
    m_sources.push_back(std::make_pair(source, value));
}

bool CspDirective::allowsSource(const std::string& source, const Origin& pageOrigin) const {
    // If no sources specified, nothing is allowed
    if (m_sources.empty()) {
        return false;
    }
    
    // Check if any source in the directive matches
    for (const auto& directiveSource : m_sources) {
        if (sourceMatches(source, pageOrigin, directiveSource.first, directiveSource.second)) {
            return true;
        }
    }
    
    // If 'none' is present, nothing is allowed
    for (const auto& directiveSource : m_sources) {
        if (directiveSource.first == CspSource::NONE) {
            return false;
        }
    }
    
    return false;
}

bool CspDirective::allowsInline() const {
    // Check for 'unsafe-inline'
    for (const auto& source : m_sources) {
        if (source.first == CspSource::UNSAFE_INLINE) {
            return true;
        }
    }
    
    return false;
}

bool CspDirective::allowsEval() const {
    // Check for 'unsafe-eval'
    for (const auto& source : m_sources) {
        if (source.first == CspSource::UNSAFE_EVAL) {
            return true;
        }
    }
    
    return false;
}

bool CspDirective::sourceMatches(const std::string& source, const Origin& pageOrigin, 
                               CspSource directiveSource, const std::string& directiveValue) const {
    // Process special keywords
    if (directiveSource == CspSource::NONE) {
        return false;
    }
    else if (directiveSource == CspSource::SELF) {
        // Same origin check
        Origin sourceOrigin(source);
        return sourceOrigin == pageOrigin;
    }
    else if (directiveSource == CspSource::CUSTOM) {
        // Handle custom source formats
        
        // Exact match
        if (directiveValue == source) {
            return true;
        }
        
        // Domain match (*.example.com)
        if (directiveValue.size() > 2 && directiveValue[0] == '*' && directiveValue[1] == '.') {
            std::string domain = directiveValue.substr(2);
            
            // Check if source ends with domain
            if (source.length() > domain.length() &&
                source.substr(source.length() - domain.length()) == domain) {
                return true;
            }
        }
        
        // Scheme match (https:)
        if (directiveValue.back() == ':') {
            std::string scheme = directiveValue.substr(0, directiveValue.length() - 1);
            
            // Check if source starts with scheme://
            if (source.substr(0, scheme.length() + 3) == scheme + "://") {
                return true;
            }
        }
    }
    
    return false;
}

//-----------------------------------------------------------------------------
// ContentSecurityPolicy Implementation
//-----------------------------------------------------------------------------

ContentSecurityPolicy::ContentSecurityPolicy() {
    // Create default directive
    m_defaultDirective = std::make_shared<CspDirective>(CspResourceType::DEFAULT);
    m_directives[CspResourceType::DEFAULT] = m_defaultDirective;
}

ContentSecurityPolicy::~ContentSecurityPolicy() {
}

bool ContentSecurityPolicy::parse(const std::string& policyHeader) {
    if (policyHeader.empty()) {
        return false;
    }
    
    // Split policy header into directives
    std::istringstream policyStream(policyHeader);
    std::string directive;
    
    while (std::getline(policyStream, directive, ';')) {
        // Remove leading/trailing whitespace
        directive.erase(0, directive.find_first_not_of(" \t\r\n"));
        directive.erase(directive.find_last_not_of(" \t\r\n") + 1);
        
        if (!directive.empty()) {
            parseDirective(directive);
        }
    }
    
    return true;
}

bool ContentSecurityPolicy::allowsResource(CspResourceType type, const std::string& source, const Origin& pageOrigin) const {
    // Check specific directive
    auto it = m_directives.find(type);
    if (it != m_directives.end() && it->second) {
        return it->second->allowsSource(source, pageOrigin);
    }
    
    // Fall back to default directive
    return m_defaultDirective->allowsSource(source, pageOrigin);
}

bool ContentSecurityPolicy::allowsInlineScript() const {
    // Check script-src directive
    auto it = m_directives.find(CspResourceType::SCRIPT);
    if (it != m_directives.end() && it->second) {
        return it->second->allowsInline();
    }
    
    // Fall back to default directive
    return m_defaultDirective->allowsInline();
}

bool ContentSecurityPolicy::allowsInlineStyle() const {
    // Check style-src directive
    auto it = m_directives.find(CspResourceType::STYLE);
    if (it != m_directives.end() && it->second) {
        return it->second->allowsInline();
    }
    
    // Fall back to default directive
    return m_defaultDirective->allowsInline();
}

bool ContentSecurityPolicy::allowsEval() const {
    // Check script-src directive
    auto it = m_directives.find(CspResourceType::SCRIPT);
    if (it != m_directives.end() && it->second) {
        return it->second->allowsEval();
    }
    
    // Fall back to default directive
    return m_defaultDirective->allowsEval();
}

std::string ContentSecurityPolicy::toString() const {
    std::ostringstream oss;
    
    for (const auto& directive : m_directives) {
        // Convert CspResourceType to string
        std::string typeStr;
        switch (directive.first) {
            case CspResourceType::DEFAULT: typeStr = "default-src"; break;
            case CspResourceType::SCRIPT: typeStr = "script-src"; break;
            case CspResourceType::STYLE: typeStr = "style-src"; break;
            case CspResourceType::IMG: typeStr = "img-src"; break;
            case CspResourceType::CONNECT: typeStr = "connect-src"; break;
            case CspResourceType::FONT: typeStr = "font-src"; break;
            case CspResourceType::MEDIA: typeStr = "media-src"; break;
            case CspResourceType::OBJECT: typeStr = "object-src"; break;
            case CspResourceType::MANIFEST: typeStr = "manifest-src"; break;
            case CspResourceType::FRAME: typeStr = "frame-src"; break;
            case CspResourceType::WORKER: typeStr = "worker-src"; break;
        }
        
        oss << typeStr << " ";
        
        // List the sources for this directive
        if (directive.second) {
            for (const auto& source : directive.second->sources()) {
                oss << source.second << " ";
            }
        }
        
        oss << "; ";
    }
    
    return oss.str();
}

void ContentSecurityPolicy::parseDirective(const std::string& directive) {
    // Split directive into type and sources
    size_t spacePos = directive.find(' ');
    if (spacePos == std::string::npos) {
        return; // Invalid directive
    }
    
    std::string typeStr = directive.substr(0, spacePos);
    std::string sourcesStr = directive.substr(spacePos + 1);
    
    // Parse resource type
    CspResourceType type = resourceTypeFromString(typeStr);
    
    // Create or get directive
    std::shared_ptr<CspDirective> cspDirective;
    if (type == CspResourceType::DEFAULT) {
        cspDirective = m_defaultDirective;
    }
    else {
        cspDirective = std::make_shared<CspDirective>(type);
        m_directives[type] = cspDirective;
    }
    
    // Parse sources
    std::istringstream sourcesStream(sourcesStr);
    std::string source;
    while (std::getline(sourcesStream, source, ' ')) {
        if (!source.empty()) {
            auto parsedSource = parseSource(source);
            cspDirective->addSource(parsedSource.first, parsedSource.second);
        }
    }
}

CspResourceType ContentSecurityPolicy::resourceTypeFromString(const std::string& typeStr) const {
    if (typeStr == "default-src") return CspResourceType::DEFAULT;
    if (typeStr == "script-src") return CspResourceType::SCRIPT;
    if (typeStr == "style-src") return CspResourceType::STYLE;
    if (typeStr == "img-src") return CspResourceType::IMG;
    if (typeStr == "connect-src") return CspResourceType::CONNECT;
    if (typeStr == "font-src") return CspResourceType::FONT;
    if (typeStr == "media-src") return CspResourceType::MEDIA;
    if (typeStr == "object-src") return CspResourceType::OBJECT;
    if (typeStr == "manifest-src") return CspResourceType::MANIFEST;
    if (typeStr == "frame-src") return CspResourceType::FRAME;
    if (typeStr == "worker-src") return CspResourceType::WORKER;
    
    return CspResourceType::DEFAULT;
}

std::pair<CspSource, std::string> ContentSecurityPolicy::parseSource(const std::string& source) const {
    // Handle special keywords
    if (source == "'none'") {
        return std::make_pair(CspSource::NONE, "");
    }
    else if (source == "'self'") {
        return std::make_pair(CspSource::SELF, "");
    }
    else if (source == "'unsafe-inline'") {
        return std::make_pair(CspSource::UNSAFE_INLINE, "");
    }
    else if (source == "'unsafe-eval'") {
        return std::make_pair(CspSource::UNSAFE_EVAL, "");
    }
    else if (source == "'strict-dynamic'") {
        return std::make_pair(CspSource::STRICT_DYNAMIC, "");
    }
    
    // Custom source
    return std::make_pair(CspSource::CUSTOM, source);
}

} // namespace security
} // namespace browser