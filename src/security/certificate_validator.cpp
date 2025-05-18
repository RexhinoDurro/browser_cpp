#include "certificate_validator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

namespace browser {
namespace security {

//-----------------------------------------------------------------------------
// Certificate Implementation
//-----------------------------------------------------------------------------

Certificate::Certificate()
    : m_isValid(false)
    , m_isSelfSigned(false)
{
    // Set default validity period to now
    m_notBefore = std::chrono::system_clock::now();
    m_notAfter = std::chrono::system_clock::now();
}

Certificate::Certificate(const std::string& certData, CertificateType type)
    : m_isValid(false)
    , m_isSelfSigned(false)
{
    // Set default validity period to now
    m_notBefore = std::chrono::system_clock::now();
    m_notAfter = std::chrono::system_clock::now();
    
    // Parse certificate data
    parseCertificate(certData, type);
}

Certificate::~Certificate() {
}

std::string Certificate::fingerprint(const std::string& algorithm) const {
    return computeFingerprint(algorithm);
}

bool Certificate::isValid() const {
    return m_isValid && !isExpired();
}

bool Certificate::isExpired() const {
    auto now = std::chrono::system_clock::now();
    return now < m_notBefore || now > m_notAfter;
}

bool Certificate::isSelfSigned() const {
    return m_isSelfSigned;
}

bool Certificate::verifySignature(const Certificate& issuerCert) const {
    // In a real implementation, this would verify the certificate's signature
    // using the issuer's public key
    
    // For this simplified implementation, we'll just check if the issuer matches
    return m_issuer == issuerCert.subject();
}

bool Certificate::matchesHostname(const std::string& hostname) const {
    // Simple hostname matching for demonstration
    // In a real implementation, this would handle wildcards and other complexities
    
    // Example: Find CN in subject
    std::regex cnRegex("CN=([^,]+)");
    std::smatch match;
    
    if (std::regex_search(m_subject, match, cnRegex)) {
        std::string cn = match[1].str();
        
        // Check for wildcard certificate
        if (cn.substr(0, 2) == "*.") {
            std::string domain = cn.substr(2);
            
            // Check if hostname ends with domain
            if (hostname.length() > domain.length() &&
                hostname.substr(hostname.length() - domain.length()) == domain &&
                hostname.substr(0, hostname.length() - domain.length()).find('.') == std::string::npos) {
                return true;
            }
        }
        else {
            // Exact match
            return cn == hostname;
        }
    }
    
    return false;
}

bool Certificate::loadFromFile(const std::string& filename, CertificateType type) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return loadFromData(buffer.str(), type);
}

bool Certificate::loadFromData(const std::string& certData, CertificateType type) {
    return parseCertificate(certData, type);
}

bool Certificate::parseCertificate(const std::string& certData, CertificateType type) {
    // In a real implementation, this would parse the certificate data according to the type
    // For demonstration, we'll create a simplified parsing mechanism
    
    // Clear any existing data
    m_rawData.clear();
    m_isValid = false;
    
    // Store raw data
    m_rawData.assign(certData.begin(), certData.end());
    
    // Parse certificate fields
    parseSubjectAndIssuer(certData);
    parseValidity(certData);
    parsePublicKey(certData);
    
    // Set self-signed flag
    m_isSelfSigned = (m_subject == m_issuer);
    
    // Certificate is valid if all required fields were parsed successfully
    m_isValid = !m_subject.empty() && !m_issuer.empty() && !m_publicKey.empty();
    
    return m_isValid;
}

void Certificate::parseSubjectAndIssuer(const std::string& certData) {
    // For the sake of simplicity, let's assume certData contains subject and issuer fields
    // In a real implementation, this would parse X.509, DER, or PEM data
    
    // Example subject: "CN=example.com,O=Example Inc,C=US"
    // Example issuer: "CN=Example CA,O=Example Inc,C=US"
    
    // Simple regex parsing for demonstration
    std::regex subjectRegex("Subject: ([^\n]+)");
    std::regex issuerRegex("Issuer: ([^\n]+)");
    std::smatch match;
    
    if (std::regex_search(certData, match, subjectRegex)) {
        m_subject = match[1].str();
    }
    
    if (std::regex_search(certData, match, issuerRegex)) {
        m_issuer = match[1].str();
    }
    
    // Extract serial number
    std::regex serialRegex("Serial Number: ([^\n]+)");
    if (std::regex_search(certData, match, serialRegex)) {
        m_serialNumber = match[1].str();
    }
}

void Certificate::parseValidity(const std::string& certData) {
    // Parse validity period
    // For demonstration, let's use a simple format
    
    // Example: "Not Before: 2020-01-01 00:00:00\nNot After: 2025-01-01 00:00:00"
    
    std::regex notBeforeRegex("Not Before: ([^\n]+)");
    std::regex notAfterRegex("Not After: ([^\n]+)");
    std::smatch match;
    
    if (std::regex_search(certData, match, notBeforeRegex)) {
        // In a real implementation, this would parse the date/time string
        // For simplicity, we'll set it to now minus one year
        m_notBefore = std::chrono::system_clock::now() - std::chrono::hours(24 * 365);
    }
    
    if (std::regex_search(certData, match, notAfterRegex)) {
        // Similarly, set to now plus one year
        m_notAfter = std::chrono::system_clock::now() + std::chrono::hours(24 * 365);
    }
}

void Certificate::parsePublicKey(const std::string& certData) {
    // For the sake of simplicity, extract public key
    // In a real implementation, this would parse the actual key data
    
    std::regex keyRegex("Public Key: ([^\n]+)");
    std::smatch match;
    
    if (std::regex_search(certData, match, keyRegex)) {
        m_publicKey = match[1].str();
    }
    else {
        // If no key found, create a placeholder
        m_publicKey = "DUMMY_PUBLIC_KEY_FOR_DEMONSTRATION";
    }
}

std::string Certificate::computeFingerprint(const std::string& algorithm) const {
    // In a real implementation, this would compute a cryptographic hash of the certificate
    // For demonstration, we'll return a dummy value
    
    std::ostringstream oss;
    oss << "FINGERPRINT_USING_" << algorithm << "_" << m_serialNumber;
    return oss.str();
}

//-----------------------------------------------------------------------------
// CertificateChain Implementation
//-----------------------------------------------------------------------------

CertificateChain::CertificateChain() {
}

CertificateChain::~CertificateChain() {
}

void CertificateChain::addCertificate(std::shared_ptr<Certificate> certificate) {
    if (certificate) {
        m_certificates.push_back(certificate);
    }
}

std::shared_ptr<Certificate> CertificateChain::leafCertificate() const {
    if (m_certificates.empty()) {
        return nullptr;
    }
    
    // In X.509 chains, the leaf (server) certificate is typically the first one
    return m_certificates.front();
}

CertificateVerificationResult CertificateChain::verify(const std::string& hostname) const {
    // Check if chain is empty
    if (m_certificates.empty()) {
        return CertificateVerificationResult::OTHER_ERROR;
    }
    
    // Get leaf certificate
    auto leaf = leafCertificate();
    if (!leaf) {
        return CertificateVerificationResult::OTHER_ERROR;
    }
    
    // Check if leaf certificate is valid
    if (!leaf->isValid()) {
        return CertificateVerificationResult::OTHER_ERROR;
    }
    
    // Check if leaf certificate has expired
    if (leaf->isExpired()) {
        return CertificateVerificationResult::EXPIRED;
    }
    
    // Check hostname match
    if (!leaf->matchesHostname(hostname)) {
        return CertificateVerificationResult::NAME_MISMATCH;
    }
    
    // Simple chain verification
    for (size_t i = 0; i < m_certificates.size() - 1; i++) {
        if (!m_certificates[i]->verifySignature(*m_certificates[i + 1])) {
            return CertificateVerificationResult::INVALID_SIGNATURE;
        }
    }
    
    // Check if root certificate is self-signed
    if (m_certificates.size() > 1 && !m_certificates.back()->isSelfSigned()) {
        return CertificateVerificationResult::UNTRUSTED_ROOT;
    }
    
    return CertificateVerificationResult::VALID;
}

//-----------------------------------------------------------------------------
// CertificateValidator Implementation
//-----------------------------------------------------------------------------

CertificateValidator::CertificateValidator() {
}

CertificateValidator::~CertificateValidator() {
}

bool CertificateValidator::initialize() {
    // In a real implementation, this would load system root certificates
    return true;
}

CertificateVerificationResult CertificateValidator::verify(const CertificateChain& chain, const std::string& hostname) const {
    // First, check the chain itself
    CertificateVerificationResult chainResult = chain.verify(hostname);
    if (chainResult != CertificateVerificationResult::VALID &&
        chainResult != CertificateVerificationResult::UNTRUSTED_ROOT) {
        return chainResult;
    }
    
    // Get the leaf certificate
    auto leaf = chain.leafCertificate();
    if (!leaf) {
        return CertificateVerificationResult::OTHER_ERROR;
    }
    
    // Check if leaf certificate is revoked
    if (isRevoked(*leaf)) {
        return CertificateVerificationResult::REVOKED;
    }
    
    // Verify hostname against certificate
    if (!verifyHostname(*leaf, hostname)) {
        return CertificateVerificationResult::NAME_MISMATCH;
    }
    
    // Verify the certificate chain against trusted roots
    return verifyChain(chain);
}

void CertificateValidator::addTrustedRoot(std::shared_ptr<Certificate> certificate) {
    if (certificate && certificate->isValid() && certificate->isSelfSigned()) {
        m_trustedRoots.push_back(certificate);
    }
}

bool CertificateValidator::loadTrustedRoots(const std::string& directory) {
    try {
        if (!fs::exists(directory) || !fs::is_directory(directory)) {
            return false;
        }
        
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                try {
                    auto cert = std::make_shared<Certificate>();
                    
                    // Attempt to load as PEM, then DER if that fails
                    if (cert->loadFromFile(entry.path().string(), CertificateType::PEM) ||
                        cert->loadFromFile(entry.path().string(), CertificateType::DER)) {
                        
                        // Add if valid and self-signed
                        if (cert->isValid() && cert->isSelfSigned()) {
                            m_trustedRoots.push_back(cert);
                        }
                    }
                }
                catch (const std::exception& e) {
                    // Log and continue
                    std::cerr << "Error loading certificate: " << e.what() << std::endl;
                }
            }
        }
        
        return !m_trustedRoots.empty();
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading trusted roots: " << e.what() << std::endl;
        return false;
    }
}

bool CertificateValidator::isRevoked(const Certificate& certificate) const {
    // In a real implementation, this would check against CRLs and OCSP
    // For simplicity, we'll just check against our revocation lists
    
    auto serialNumber = certificate.serialNumber();
    auto issuer = certificate.issuer();
    
    // Check if issuer has a revocation list
    auto it = m_revocationLists.find(issuer);
    if (it != m_revocationLists.end()) {
        // Check if serial number is in the list
        return std::find(it->second.begin(), it->second.end(), serialNumber) != it->second.end();
    }
    
    return false;
}

bool CertificateValidator::verifyHostname(const Certificate& certificate, const std::string& hostname) const {
    return certificate.matchesHostname(hostname);
}

CertificateVerificationResult CertificateValidator::verifyChain(const CertificateChain& chain) const {
    const auto& certificates = chain.certificates();
    
    // Empty chain is invalid
    if (certificates.empty()) {
        return CertificateVerificationResult::OTHER_ERROR;
    }
    
    // Check if the root certificate is in our trusted roots
    auto root = certificates.back();
    if (!root->isSelfSigned()) {
        return CertificateVerificationResult::UNTRUSTED_ROOT;
    }
    
    // Check if root is trusted
    bool rootTrusted = false;
    for (const auto& trustedRoot : m_trustedRoots) {
        if (root->subject() == trustedRoot->subject() && 
            root->fingerprint() == trustedRoot->fingerprint()) {
            rootTrusted = true;
            break;
        }
    }
    
    if (!rootTrusted) {
        return CertificateVerificationResult::UNTRUSTED_ROOT;
    }
    
    // If we get here, the chain is valid
    return CertificateVerificationResult::VALID;
}

} // namespace security
} // namespace browser