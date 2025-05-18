#ifndef BROWSER_CERTIFICATE_VALIDATOR_H
#define BROWSER_CERTIFICATE_VALIDATOR_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

namespace browser {
namespace security {

// Certificate types
enum class CertificateType {
    X509,
    DER,
    PEM
};

// Certificate verification result
enum class CertificateVerificationResult {
    VALID,
    EXPIRED,
    REVOKED,
    UNTRUSTED_ROOT,
    INVALID_SIGNATURE,
    NAME_MISMATCH,
    SELF_SIGNED,
    OTHER_ERROR
};

// X.509 Certificate class
class Certificate {
public:
    Certificate();
    Certificate(const std::string& certData, CertificateType type);
    ~Certificate();
    
    // Get certificate information
    std::string subject() const { return m_subject; }
    std::string issuer() const { return m_issuer; }
    std::string serialNumber() const { return m_serialNumber; }
    std::chrono::system_clock::time_point notBefore() const { return m_notBefore; }
    std::chrono::system_clock::time_point notAfter() const { return m_notAfter; }
    
    // Get the public key
    std::string publicKey() const { return m_publicKey; }
    
    // Get the certificate fingerprint
    std::string fingerprint(const std::string& algorithm = "SHA-256") const;
    
    // Check if the certificate is valid
    bool isValid() const;
    
    // Check if the certificate has expired
    bool isExpired() const;
    
    // Check if the certificate is self-signed
    bool isSelfSigned() const;
    
    // Verify the certificate signature
    bool verifySignature(const Certificate& issuerCert) const;
    
    // Check if the certificate is issued for a specific hostname
    bool matchesHostname(const std::string& hostname) const;
    
    // Load certificate from file
    bool loadFromFile(const std::string& filename, CertificateType type);
    
    // Load certificate from data
    bool loadFromData(const std::string& certData, CertificateType type);
    
private:
    std::string m_subject;
    std::string m_issuer;
    std::string m_serialNumber;
    std::chrono::system_clock::time_point m_notBefore;
    std::chrono::system_clock::time_point m_notAfter;
    std::string m_publicKey;
    bool m_isValid;
    bool m_isSelfSigned;
    
    // The raw certificate data
    std::vector<uint8_t> m_rawData;
    
    // Parse certificate data
    bool parseCertificate(const std::string& certData, CertificateType type);
    
    // Parse certificate fields
    void parseSubjectAndIssuer(const std::string& certData);
    void parseValidity(const std::string& certData);
    void parsePublicKey(const std::string& certData);
    
    // Compute fingerprint
    std::string computeFingerprint(const std::string& algorithm) const;
};

// Certificate chain
class CertificateChain {
public:
    CertificateChain();
    ~CertificateChain();
    
    // Add a certificate to the chain
    void addCertificate(std::shared_ptr<Certificate> certificate);
    
    // Get all certificates in the chain
    const std::vector<std::shared_ptr<Certificate>>& certificates() const { return m_certificates; }
    
    // Get the leaf (server) certificate
    std::shared_ptr<Certificate> leafCertificate() const;
    
    // Verify the certificate chain
    CertificateVerificationResult verify(const std::string& hostname) const;
    
private:
    std::vector<std::shared_ptr<Certificate>> m_certificates;
};

// Certificate validator class
class CertificateValidator {
public:
    CertificateValidator();
    ~CertificateValidator();
    
    // Initialize the validator
    bool initialize();
    
    // Verify a certificate chain for a hostname
    CertificateVerificationResult verify(const CertificateChain& chain, const std::string& hostname) const;
    
    // Add a trusted root certificate
    void addTrustedRoot(std::shared_ptr<Certificate> certificate);
    
    // Load trusted root certificates from a directory
    bool loadTrustedRoots(const std::string& directory);
    
private:
    // List of trusted root certificates
    std::vector<std::shared_ptr<Certificate>> m_trustedRoots;
    
    // Certificate revocation lists (CRLs)
    std::map<std::string, std::vector<std::string>> m_revocationLists;
    
    // Check if a certificate is revoked
    bool isRevoked(const Certificate& certificate) const;
    
    // Verify hostname against certificate
    bool verifyHostname(const Certificate& certificate, const std::string& hostname) const;
    
    // Build and verify certificate chain
    CertificateVerificationResult verifyChain(const CertificateChain& chain) const;
};

} // namespace security
} // namespace browser

#endif // BROWSER_CERTIFICATE_VALIDATOR_H