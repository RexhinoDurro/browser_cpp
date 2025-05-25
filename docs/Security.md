# Security Components Documentation

## Overview

The security layer implements comprehensive web security policies and protections including Same-Origin Policy, XSS protection, CSRF protection, Content Security Policy (CSP), cookie security, and certificate validation. These components work together to ensure safe browsing and protect against common web vulnerabilities.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                      Security Manager                           │
│  ┌───────────────┐  ┌────────────────┐  ┌─────────────────┐  │
│  │ Same-Origin   │  │ XSS Protection │  │ CSRF Protection │  │
│  │   Policy      │  │                │  │                 │  │
│  └───────────────┘  └────────────────┘  └─────────────────┘  │
│  ┌───────────────┐  ┌────────────────┐  ┌─────────────────┐  │
│  │     CSP       │  │ Cookie Security│  │  Certificate    │  │
│  │               │  │    Manager     │  │   Validator     │  │
│  └───────────────┘  └────────────────┘  └─────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Same-Origin Policy

### Overview

The Same-Origin Policy (`same_origin.h/cpp`) is the fundamental security model that restricts how documents from one origin can interact with resources from another origin.

### Origin Definition

An origin is defined by the triplet: `(scheme, host, port)`

```cpp
class Origin {
public:
    Origin(const std::string& url);
    Origin(const std::string& scheme, const std::string& host, int port);
    
    // Origin comparison
    bool equals(const Origin& other) const;
    
    // Special origins
    static Origin null();   // Represents "null" origin
    static Origin opaque(); // Unique opaque origin
};
```

### Policy Enforcement

```cpp
class SameOriginPolicy {
public:
    // Check if source can access target
    bool canAccess(const Origin& source, const Origin& target) const;
    
    // Specific checks
    bool canReadCookies(const Origin& source, const Origin& target) const;
    bool canExecuteScript(const Origin& source, const Origin& target) const;
    
    // Trusted origins (for development)
    void addTrustedOrigin(const Origin& origin);
};
```

### Examples

```cpp
Origin origin1("https://example.com:443");
Origin origin2("https://example.com");     // Port 443 implied
Origin origin3("http://example.com");      // Different scheme

origin1 == origin2;  // true (same origin)
origin1 == origin3;  // false (different scheme)
```

## XSS Protection

### Overview

Cross-Site Scripting (XSS) protection (`xss_protection.h/cpp`) prevents injection of malicious scripts into web pages.

### Protection Modes

```cpp
enum class XssProtectionMode {
    DISABLED,    // No XSS protection
    ENABLED,     // Filter and sanitize
    BLOCK,       // Block page rendering
    REPORT       // Report violations
};
```

### Detection and Sanitization

```cpp
class XssProtection {
public:
    // Detection
    bool isUrlPotentialXss(const std::string& url) const;
    bool isHtmlPotentialXss(const std::string& html) const;
    
    // Sanitization
    std::string sanitizeHtml(const std::string& html, 
                           SanitizationLevel level) const;
    std::string sanitizeUrl(const std::string& url) const;
    
    // HTML escaping
    std::string escapeHtml(const std::string& input) const;
};
```

### Sanitization Levels

```cpp
enum class SanitizationLevel {
    NONE,         // No sanitization
    ESCAPE_ONLY,  // Only escape HTML entities
    BASIC,        // Remove dangerous tags/attributes
    STRICT        // Whitelist approach
};
```

### HTML Sanitization Rules

**Basic Level**:
- Removes `<script>` tags
- Removes event handlers (`onclick`, `onerror`, etc.)
- Removes `javascript:` URLs

**Strict Level**:
- Whitelists allowed tags
- Whitelists allowed attributes
- Validates URL protocols

### Usage Example

```cpp
XssProtection xss;
xss.initialize();

// Check for XSS
if (xss.isHtmlPotentialXss(userInput)) {
    // Sanitize the input
    std::string safe = xss.sanitizeHtml(userInput, SanitizationLevel::STRICT);
}

// Escape for display
std::string escaped = xss.escapeHtml("<script>alert('XSS')</script>");
// Result: "&lt;script&gt;alert('XSS')&lt;/script&gt;"
```

## CSRF Protection

### Overview

Cross-Site Request Forgery (CSRF) protection (`csrf_protection.h/cpp`) prevents unauthorized commands from being transmitted from a user that the web application trusts.

### Protection Strategies

```cpp
enum class CsrfProtectionMode {
    DISABLED,              // No protection
    SAME_ORIGIN,          // Check same-origin
    ORIGIN_HEADER,        // Verify Origin header
    REFERER_HEADER,       // Verify Referer header
    CSRF_TOKEN,           // Token validation
    DOUBLE_SUBMIT_COOKIE  // Cookie-to-header validation
};
```

### CSRF Token Generation

```cpp
class CsrfToken {
public:
    // Generate cryptographically secure token
    std::string generate();
    
    // Validate token
    bool validate(const std::string& token, 
                 const std::string& expectedToken) const;
};
```

### Request Validation

```cpp
class CsrfProtection {
public:
    // Check if request is potential CSRF
    bool isPotentialCsrf(const std::string& method, 
                        const std::string& targetUrl,
                        const std::map<std::string, std::string>& headers,
                        const std::map<std::string, std::string>& cookies,
                        const std::map<std::string, std::string>& formParams) const;
    
    // Token management
    std::string generateToken();
    bool validateToken(const std::string& token, 
                      const std::string& expectedToken) const;
};
```

### Usage Example

```cpp
CsrfProtection csrf;
csrf.setMode(CsrfProtectionMode::CSRF_TOKEN);

// Generate token for form
std::string token = csrf.generateToken();

// Validate on submission
if (!csrf.validateToken(submittedToken, expectedToken)) {
    // Reject request - potential CSRF
}
```

## Content Security Policy (CSP)

### Overview

CSP (`content_security_policy.h/cpp`) provides a declarative policy to control which resources can be loaded and executed.

### Resource Types

```cpp
enum class CspResourceType {
    DEFAULT,    // default-src
    SCRIPT,     // script-src
    STYLE,      // style-src
    IMG,        // img-src
    CONNECT,    // connect-src
    FONT,       // font-src
    MEDIA,      // media-src
    OBJECT,     // object-src
    FRAME,      // frame-src
    WORKER      // worker-src
};
```

### Policy Sources

```cpp
enum class CspSource {
    NONE,           // 'none'
    SELF,           // 'self'
    UNSAFE_INLINE,  // 'unsafe-inline'
    UNSAFE_EVAL,    // 'unsafe-eval'
    CUSTOM          // Domain/URL
};
```

### Policy Definition

```cpp
class ContentSecurityPolicy {
public:
    // Parse CSP header
    bool parse(const std::string& policyHeader);
    
    // Check if resource is allowed
    bool allowsResource(CspResourceType type, 
                       const std::string& source,
                       const Origin& pageOrigin) const;
    
    // Specific checks
    bool allowsInlineScript() const;
    bool allowsInlineStyle() const;
    bool allowsEval() const;
};
```

### CSP Examples

```cpp
// Parse CSP header
ContentSecurityPolicy csp;
csp.parse("default-src 'self'; script-src 'self' https://cdn.example.com");

// Check if resource allowed
Origin pageOrigin("https://example.com");
bool allowed = csp.allowsResource(CspResourceType::SCRIPT, 
                                 "https://cdn.example.com/script.js",
                                 pageOrigin);
```

### Common CSP Policies

```
// Strict policy
default-src 'none'; script-src 'self'; style-src 'self'; img-src 'self' data:;

// Moderate policy  
default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline';

// Report-only mode
Content-Security-Policy-Report-Only: default-src 'self'; report-uri /csp-report
```

## Cookie Security

### Overview

Cookie security (`cookie_security.h/cpp`) implements secure cookie handling with support for security flags and SameSite attribute.

### Cookie Attributes

```cpp
class Cookie {
    std::string m_name;
    std::string m_value;
    std::string m_domain;
    std::string m_path;
    bool m_secure;          // HTTPS only
    bool m_httpOnly;        // No JavaScript access
    std::string m_sameSite; // "Strict", "Lax", "None"
    std::chrono::system_clock::time_point m_expiresAt;
};
```

### Cookie Jar

```cpp
class CookieJar {
public:
    // Cookie management
    void addCookie(const Cookie& cookie);
    std::vector<Cookie> getCookies(const std::string& domain, 
                                  const std::string& path,
                                  bool secure) const;
    void removeCookie(const std::string& domain, 
                     const std::string& path,
                     const std::string& name);
    
    // Maintenance
    void clearExpired();
};
```

### Security Manager

```cpp
class CookieSecurityManager {
public:
    // Security checks
    bool canSetCookie(const Cookie& cookie, 
                     const Origin& origin,
                     bool isSecureContext) const;
    
    bool canReadCookie(const Cookie& cookie,
                      const Origin& requestOrigin,
                      const Origin& cookieOrigin) const;
    
    // Apply security recommendations
    Cookie applySecurity(const Cookie& cookie, 
                        const Origin& origin) const;
};
```

### SameSite Attribute

```cpp
// SameSite=Strict
// Cookie only sent with same-site requests
cookie.setSameSite("Strict");

// SameSite=Lax (default)
// Cookie sent with same-site and top-level navigation
cookie.setSameSite("Lax");

// SameSite=None
// Cookie sent with all requests (requires Secure flag)
cookie.setSameSite("None");
cookie.setSecure(true);
```

### Usage Example

```cpp
// Parse Set-Cookie header
Cookie cookie = Cookie::parse("session=abc123; Secure; HttpOnly; SameSite=Strict");

// Security validation
CookieSecurityManager manager;
Origin origin("https://example.com");

if (manager.canSetCookie(cookie, origin, true)) {
    jar.addCookie(cookie);
}
```

## Certificate Validation

### Overview

Certificate validation (`certificate_validator.h/cpp`) verifies SSL/TLS certificates for secure connections.

### Certificate Chain

```cpp
class CertificateChain {
    std::vector<std::shared_ptr<Certificate>> m_certificates;
    
public:
    // Add certificates (leaf to root order)
    void addCertificate(std::shared_ptr<Certificate> certificate);
    
    // Get leaf (server) certificate
    std::shared_ptr<Certificate> leafCertificate() const;
    
    // Verify chain
    CertificateVerificationResult verify(const std::string& hostname) const;
};
```

### Validation Results

```cpp
enum class CertificateVerificationResult {
    VALID,              // Certificate is valid
    EXPIRED,            // Certificate has expired
    REVOKED,            // Certificate was revoked
    UNTRUSTED_ROOT,     // Root CA not trusted
    INVALID_SIGNATURE,  // Signature verification failed
    NAME_MISMATCH,      // Hostname doesn't match
    SELF_SIGNED,        // Self-signed certificate
    OTHER_ERROR         // Other validation error
};
```

### Certificate Validator

```cpp
class CertificateValidator {
public:
    // Verify certificate chain
    CertificateVerificationResult verify(const CertificateChain& chain,
                                       const std::string& hostname) const;
    
    // Trusted root management
    void addTrustedRoot(std::shared_ptr<Certificate> certificate);
    bool loadTrustedRoots(const std::string& directory);
    
private:
    // Certificate revocation check
    bool isRevoked(const Certificate& certificate) const;
    
    // Hostname verification
    bool verifyHostname(const Certificate& certificate,
                       const std::string& hostname) const;
};
```

### Usage Example

```cpp
CertificateValidator validator;
validator.loadTrustedRoots("/etc/ssl/certs");

// Build certificate chain
CertificateChain chain;
chain.addCertificate(serverCert);
chain.addCertificate(intermediateCert);
chain.addCertificate(rootCert);

// Verify
auto result = validator.verify(chain, "example.com");
if (result != CertificateVerificationResult::VALID) {
    // Handle invalid certificate
}
```

## Security Manager

### Overview

The Security Manager (`security_manager.h/cpp`) coordinates all security components and provides a unified interface.

### Integrated Security Checks

```cpp
class SecurityManager {
public:
    // Initialize all security components
    bool initialize();
    
    // Request validation
    bool canMakeRequest(const Origin& source, 
                       const Origin& target,
                       const std::string& method,
                       const std::string& contentType) const;
    
    // Content security
    bool isAllowedByCSP(const std::string& url, 
                       CspResourceType type,
                       const Origin& pageOrigin) const;
    
    // XSS protection
    bool isPotentialXSS(const std::string& html) const;
    std::string sanitizeHtml(const std::string& html,
                           SanitizationLevel level) const;
    
    // CSRF protection
    bool isPotentialCSRF(const std::string& method,
                        const std::string& targetUrl,
                        const std::map<std::string, std::string>& headers,
                        const std::map<std::string, std::string>& cookies,
                        const std::map<std::string, std::string>& formParams) const;
};
```

### Usage in Browser

```cpp
SecurityManager security;
security.initialize();

// Before making request
if (!security.canMakeRequest(documentOrigin, requestOrigin, "POST", "application/json")) {
    // Block request
    return;
}

// Before executing script
if (!security.isAllowedByCSP(scriptUrl, CspResourceType::SCRIPT, documentOrigin)) {
    // Block script
    return;
}

// Before rendering user content
std::string safeHtml = security.sanitizeHtml(userContent, SanitizationLevel::STRICT);
```

## Security Best Practices

### Default Policies

1. **Strict CSP by default**
2. **XSS protection enabled**
3. **CSRF tokens for state-changing requests**
4. **Secure and HttpOnly cookies**
5. **Certificate validation required**

### Development Mode

```cpp
// Relaxed policies for development
security.sameOriginPolicy()->addTrustedOrigin(Origin("http://localhost:3000"));
security.xssProtection()->setMode(XssProtectionMode::REPORT);
```

## Testing Security

### Unit Tests

```cpp
TEST(SameOrigin, BasicCheck) {
    Origin origin1("https://example.com");
    Origin origin2("https://example.com:8080");
    
    SameOriginPolicy policy;
    EXPECT_FALSE(policy.canAccess(origin1, origin2));
}

TEST(XssProtection, ScriptTagDetection) {
    XssProtection xss;
    xss.initialize();
    
    EXPECT_TRUE(xss.isHtmlPotentialXss("<script>alert('XSS')</script>"));
    EXPECT_TRUE(xss.isHtmlPotentialXss("<img onerror='alert(1)'>"));
}
```

### Security Audit

Regular security checks:
1. CSP policy review
2. Cookie configuration audit
3. Certificate expiration monitoring
4. XSS filter effectiveness
5. CSRF token entropy

## Limitations

1. **Basic Certificate Validation**: No OCSP/CRL checking
2. **Limited CSP**: Subset of full specification
3. **No HSTS**: HTTP Strict Transport Security not implemented
4. **Basic XSS Filters**: May not catch all attacks
5. **No Subresource Integrity**: SRI not supported

## Future Enhancements

1. **Full CSP 3.0**: Nonces, hashes, strict-dynamic
2. **HSTS Support**: Force HTTPS connections
3. **Public Key Pinning**: HPKP implementation
4. **Subresource Integrity**: Hash verification
5. **Permissions Policy**: Feature restrictions