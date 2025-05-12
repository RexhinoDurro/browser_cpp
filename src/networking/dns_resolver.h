#ifndef BROWSER_DNS_RESOLVER_H
#define BROWSER_DNS_RESOLVER_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <chrono>
#include <functional>
#include <memory>

namespace browser {
namespace networking {

// DNS record types
enum class DnsRecordType {
    A,      // IPv4 address
    AAAA,   // IPv6 address
    CNAME,  // Canonical name
    MX,     // Mail exchange
    NS,     // Name server
    PTR,    // Pointer
    TXT,    // Text
    SOA,    // Start of authority
    SRV,    // Service
    ANY     // Any record type
};

// IP address representation
struct IpAddress {
    // IPv4
    unsigned char ipv4[4];
    // IPv6
    unsigned char ipv6[16];
    // Is IPv6
    bool isIpv6;
    
    // Constructors
    IpAddress();
    IpAddress(const std::string& address);
    
    // Convert to string
    std::string toString() const;
};

// DNS record base class
class DnsRecord {
public:
    DnsRecord(const std::string& name, DnsRecordType type, int ttl);
    virtual ~DnsRecord();
    
    std::string name() const { return m_name; }
    DnsRecordType type() const { return m_type; }
    int ttl() const { return m_ttl; }
    
    // Time when this record expires
    std::chrono::system_clock::time_point expiresAt() const { return m_expiresAt; }
    
    // Check if record is expired
    bool isExpired() const;
    
    // Convert to string
    virtual std::string toString() const = 0;
    
private:
    std::string m_name;
    DnsRecordType m_type;
    int m_ttl;
    std::chrono::system_clock::time_point m_expiresAt;
};

// A record (IPv4 address)
class DnsARecord : public DnsRecord {
public:
    DnsARecord(const std::string& name, int ttl, const IpAddress& address);
    
    IpAddress address() const { return m_address; }
    
    // Convert to string
    virtual std::string toString() const override;
    
private:
    IpAddress m_address;
};

// AAAA record (IPv6 address)
class DnsAAAARecord : public DnsRecord {
public:
    DnsAAAARecord(const std::string& name, int ttl, const IpAddress& address);
    
    IpAddress address() const { return m_address; }
    
    // Convert to string
    virtual std::string toString() const override;
    
private:
    IpAddress m_address;
};

// CNAME record (canonical name)
class DnsCNameRecord : public DnsRecord {
public:
    DnsCNameRecord(const std::string& name, int ttl, const std::string& cname);
    
    std::string cname() const { return m_cname; }
    
    // Convert to string
    virtual std::string toString() const override;
    
private:
    std::string m_cname;
};

// DNS resolver callback
using DnsResolverCallback = std::function<void(const std::vector<std::shared_ptr<DnsRecord>>&, const std::string&)>;

// DNS resolver class
class DnsResolver {
public:
    DnsResolver();
    ~DnsResolver();
    
    // Initialize the resolver
    bool initialize();
    
    // Resolve hostname (synchronous)
    bool resolve(const std::string& hostname, DnsRecordType type, 
                std::vector<std::shared_ptr<DnsRecord>>& records, std::string& error);
    
    // Resolve hostname (asynchronous)
    void resolveAsync(const std::string& hostname, DnsRecordType type, DnsResolverCallback callback);
    
    // Process pending asynchronous resolutions (for event loop integration)
    void processPendingResolutions();
    
    // Set maximum cache time in seconds (0 to disable cache)
    void setMaxCacheTime(int seconds) { m_maxCacheTimeSeconds = seconds; }
    
    // Clear the cache
    void clearCache();
    
private:
    // Cache entry
    struct CacheEntry {
        std::vector<std::shared_ptr<DnsRecord>> records;
        std::chrono::system_clock::time_point timestamp;
    };
    
    // Cache by hostname and record type
    std::map<std::string, std::map<DnsRecordType, CacheEntry>> m_cache;
    std::mutex m_cacheMutex;
    
    // Maximum cache time in seconds
    int m_maxCacheTimeSeconds;
    
    // Check if a hostname is already an IP address
    bool isIpAddress(const std::string& hostname, IpAddress& address);
    
    // Get records from cache
    bool getFromCache(const std::string& hostname, DnsRecordType type, 
                     std::vector<std::shared_ptr<DnsRecord>>& records);
    
    // Add records to cache
    void addToCache(const std::string& hostname, DnsRecordType type, 
                   const std::vector<std::shared_ptr<DnsRecord>>& records);
    
    // Platform-specific DNS resolution
    bool resolveWithPlatformApi(const std::string& hostname, DnsRecordType type, 
                               std::vector<std::shared_ptr<DnsRecord>>& records, std::string& error);
    
    // Pending asynchronous resolutions
    struct AsyncResolution {
        std::string hostname;
        DnsRecordType type;
        DnsResolverCallback callback;
        bool inProgress;
    };
    std::vector<AsyncResolution> m_pendingResolutions;
};

} // namespace networking
} // namespace browser

#endif // BROWSER_DNS_RESOLVER_H