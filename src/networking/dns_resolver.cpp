#include "dns_resolver.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>

// Platform-specific include#include "dns_resolver.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>

// Platform-specific includes
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

namespace browser {
namespace networking {

//-----------------------------------------------------------------------------
// IpAddress Implementation
//-----------------------------------------------------------------------------

IpAddress::IpAddress() : isIpv6(false) {
    std::memset(ipv4, 0, sizeof(ipv4));
    std::memset(ipv6, 0, sizeof(ipv6));
}

IpAddress::IpAddress(const std::string& address) : isIpv6(false) {
    std::memset(ipv4, 0, sizeof(ipv4));
    std::memset(ipv6, 0, sizeof(ipv6));
    
    // Check if IPv4 or IPv6
    if (address.find(':') != std::string::npos) {
        // IPv6
        isIpv6 = true;
        
        // Parse IPv6 address
        in6_addr addr;
        if (inet_pton(AF_INET6, address.c_str(), &addr) == 1) {
            std::memcpy(ipv6, &addr, sizeof(ipv6));
        }
    } else {
        // IPv4
        isIpv6 = false;
        
        // Parse IPv4 address
        in_addr addr;
        if (inet_pton(AF_INET, address.c_str(), &addr) == 1) {
            std::memcpy(ipv4, &addr, sizeof(ipv4));
        }
    }
}

std::string IpAddress::toString() const {
    char buffer[INET6_ADDRSTRLEN];
    
    if (isIpv6) {
        // IPv6
        if (inet_ntop(AF_INET6, ipv6, buffer, INET6_ADDRSTRLEN) != nullptr) {
            return buffer;
        }
    } else {
        // IPv4
        if (inet_ntop(AF_INET, ipv4, buffer, INET_ADDRSTRLEN) != nullptr) {
            return buffer;
        }
    }
    
    return "";
}

//-----------------------------------------------------------------------------
// DnsRecord Implementation
//-----------------------------------------------------------------------------

DnsRecord::DnsRecord(const std::string& name, DnsRecordType type, int ttl)
    : m_name(name)
    , m_type(type)
    , m_ttl(ttl)
{
    // Calculate expiration time
    m_expiresAt = std::chrono::system_clock::now() + std::chrono::seconds(ttl);
}

DnsRecord::~DnsRecord() {
}

bool DnsRecord::isExpired() const {
    return std::chrono::system_clock::now() > m_expiresAt;
}

//-----------------------------------------------------------------------------
// DnsARecord Implementation
//-----------------------------------------------------------------------------

DnsARecord::DnsARecord(const std::string& name, int ttl, const IpAddress& address)
    : DnsRecord(name, DnsRecordType::A, ttl)
    , m_address(address)
{
}

std::string DnsARecord::toString() const {
    std::ostringstream oss;
    oss << name() << " " << ttl() << " IN A " << m_address.toString();
    return oss.str();
}

//-----------------------------------------------------------------------------
// DnsAAAARecord Implementation
//-----------------------------------------------------------------------------

DnsAAAARecord::DnsAAAARecord(const std::string& name, int ttl, const IpAddress& address)
    : DnsRecord(name, DnsRecordType::AAAA, ttl)
    , m_address(address)
{
}

std::string DnsAAAARecord::toString() const {
    std::ostringstream oss;
    oss << name() << " " << ttl() << " IN AAAA " << m_address.toString();
    return oss.str();
}

//-----------------------------------------------------------------------------
// DnsCNameRecord Implementation
//-----------------------------------------------------------------------------

DnsCNameRecord::DnsCNameRecord(const std::string& name, int ttl, const std::string& cname)
    : DnsRecord(name, DnsRecordType::CNAME, ttl)
    , m_cname(cname)
{
}

std::string DnsCNameRecord::toString() const {
    std::ostringstream oss;
    oss << name() << " " << ttl() << " IN CNAME " << m_cname;
    return oss.str();
}

//-----------------------------------------------------------------------------
// DnsResolver Implementation
//-----------------------------------------------------------------------------

DnsResolver::DnsResolver()
    : m_maxCacheTimeSeconds(300) // 5 minutes default
{
}

DnsResolver::~DnsResolver() {
}

bool DnsResolver::initialize() {
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }
#endif
    
    return true;
}

bool DnsResolver::resolve(const std::string& hostname, DnsRecordType type, 
                       std::vector<std::shared_ptr<DnsRecord>>& records, std::string& error) {
    // Check if hostname is already an IP address
    IpAddress ipAddr;
    if (isIpAddress(hostname, ipAddr)) {
        // Create appropriate record
        if ((type == DnsRecordType::A && !ipAddr.isIpv6) || 
            (type == DnsRecordType::AAAA && ipAddr.isIpv6) || 
            type == DnsRecordType::ANY) {
            
            if (ipAddr.isIpv6) {
                records.push_back(std::make_shared<DnsAAAARecord>(hostname, 86400, ipAddr));
            } else {
                records.push_back(std::make_shared<DnsARecord>(hostname, 86400, ipAddr));
            }
        }
        
        return true;
    }
    
    // Check cache first
    if (getFromCache(hostname, type, records)) {
        return true;
    }
    
    // Perform actual DNS resolution
    if (!resolveWithPlatformApi(hostname, type, records, error)) {
        return false;
    }
    
    // Add to cache
    addToCache(hostname, type, records);
    
    return true;
}

void DnsResolver::resolveAsync(const std::string& hostname, DnsRecordType type, DnsResolverCallback callback) {
    // Add to pending resolutions queue
    AsyncResolution asyncResolution;
    asyncResolution.hostname = hostname;
    asyncResolution.type = type;
    asyncResolution.callback = callback;
    asyncResolution.inProgress = false;
    
    m_pendingResolutions.push_back(asyncResolution);
}

void DnsResolver::processPendingResolutions() {
    // Process one pending resolution
    for (auto& asyncResolution : m_pendingResolutions) {
        if (!asyncResolution.inProgress) {
            asyncResolution.inProgress = true;
            
            // Process the resolution
            std::vector<std::shared_ptr<DnsRecord>> records;
            std::string error;
            bool result = resolve(asyncResolution.hostname, asyncResolution.type, records, error);
            
            // Call the callback
            asyncResolution.callback(records, error);
            
            // Remove the resolution
            asyncResolution = m_pendingResolutions.back();
            m_pendingResolutions.pop_back();
            break;
        }
    }
}

void DnsResolver::clearCache() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_cache.clear();
}

bool DnsResolver::isIpAddress(const std::string& hostname, IpAddress& address) {
    // Try to parse as IPv4
    in_addr addr4;
    if (inet_pton(AF_INET, hostname.c_str(), &addr4) == 1) {
        address = IpAddress(hostname);
        return true;
    }
    
    // Try to parse as IPv6
    in6_addr addr6;
    if (inet_pton(AF_INET6, hostname.c_str(), &addr6) == 1) {
        address = IpAddress(hostname);
        return true;
    }
    
    return false;
}

bool DnsResolver::getFromCache(const std::string& hostname, DnsRecordType type, 
                            std::vector<std::shared_ptr<DnsRecord>>& records) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    // Check if hostname is in cache
    auto hostIt = m_cache.find(hostname);
    if (hostIt == m_cache.end()) {
        return false;
    }
    
    // Check if type is in cache
    auto typeIt = hostIt->second.find(type);
    if (typeIt == hostIt->second.end()) {
        return false;
    }
    
    // Check if cache entry is expired
    auto& entry = typeIt->second;
    auto now = std::chrono::system_clock::now();
    
    if (m_maxCacheTimeSeconds > 0 && 
        (now - entry.timestamp) > std::chrono::seconds(m_maxCacheTimeSeconds)) {
        // Cache entry is expired, remove it
        hostIt->second.erase(typeIt);
        if (hostIt->second.empty()) {
            m_cache.erase(hostIt);
        }
        return false;
    }
    
    // Check if individual records are expired
    bool allExpired = true;
    records.clear();
    
    for (const auto& record : entry.records) {
        if (!record->isExpired()) {
            records.push_back(record);
            allExpired = false;
        }
    }
    
    if (allExpired) {
        // All records are expired, remove the cache entry
        hostIt->second.erase(typeIt);
        if (hostIt->second.empty()) {
            m_cache.erase(hostIt);
        }
        return false;
    }
    
    return !records.empty();
}

void DnsResolver::addToCache(const std::string& hostname, DnsRecordType type, 
                          const std::vector<std::shared_ptr<DnsRecord>>& records) {
    if (m_maxCacheTimeSeconds <= 0 || records.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    
    // Add to cache
    CacheEntry entry;
    entry.records = records;
    entry.timestamp = std::chrono::system_clock::now();
    
    m_cache[hostname][type] = entry;
}

bool DnsResolver::resolveWithPlatformApi(const std::string& hostname, DnsRecordType type, 
                                      std::vector<std::shared_ptr<DnsRecord>>& records, std::string& error) {
    // Determine address family based on record type
    int family = AF_UNSPEC;
    
    if (type == DnsRecordType::A) {
        family = AF_INET;
    } else if (type == DnsRecordType::AAAA) {
        family = AF_INET6;
    }
    
    // Set up hints for getaddrinfo
    struct addrinfo hints = {0}, *result = nullptr;
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;
    
    // Call getaddrinfo
    int ret = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
    if (ret != 0) {
        error = "DNS resolution failed: ";
        #ifdef _WIN32
        error += std::to_string(WSAGetLastError());
        #else
        error += gai_strerror(ret);
        #endif
        return false;
    }
    
    // Process results
    for (struct addrinfo* res = result; res != nullptr; res = res->ai_next) {
        if (res->ai_family == AF_INET && (type == DnsRecordType::A || type == DnsRecordType::ANY)) {
            // IPv4 address
            struct sockaddr_in* addr = (struct sockaddr_in*)res->ai_addr;
            IpAddress ipAddr;
            ipAddr.isIpv6 = false;
            std::memcpy(ipAddr.ipv4, &addr->sin_addr, sizeof(ipAddr.ipv4));
            
            records.push_back(std::make_shared<DnsARecord>(hostname, 300, ipAddr));
        } else if (res->ai_family == AF_INET6 && (type == DnsRecordType::AAAA || type == DnsRecordType::ANY)) {
            // IPv6 address
            struct sockaddr_in6* addr = (struct sockaddr_in6*)res->ai_addr;
            IpAddress ipAddr;
            ipAddr.isIpv6 = true;
            std::memcpy(ipAddr.ipv6, &addr->sin6_addr, sizeof(ipAddr.ipv6));
            
            records.push_back(std::make_shared<DnsAAAARecord>(hostname, 300, ipAddr));
        }
    }
    
    // Free result
    freeaddrinfo(result);
    
    return !records.empty();
}

} // namespace networking
} // namespace browser