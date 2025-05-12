#ifndef BROWSER_CACHE_H
#define BROWSER_CACHE_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <chrono>
#include <memory>
#include <functional>

namespace browser {
namespace networking {

// Cache entry metadata
struct CacheEntryMetadata {
    std::string url;
    std::string etag;
    std::string lastModified;
    std::map<std::string, std::string> headers;
    std::chrono::system_clock::time_point expires;
    std::chrono::system_clock::time_point timestamp;
    
    bool operator==(const CacheEntryMetadata& other) const {
        return url == other.url;
    }
};

// Cache entry
class CacheEntry {
public:
    CacheEntry();
    CacheEntry(const std::string& url, const std::vector<uint8_t>& data,
              const std::map<std::string, std::string>& headers);
    ~CacheEntry();
    
    // Get metadata
    const CacheEntryMetadata& metadata() const { return m_metadata; }
    
    // Get data
    const std::vector<uint8_t>& data() const { return m_data; }
    
    // Check if entry is expired
    bool isExpired() const;
    
    // Check if entry can be validated (has ETag or Last-Modified)
    bool canBeValidated() const;
    
    // Get validation headers for conditional request
    std::map<std::string, std::string> getValidationHeaders() const;
    
    // Update entry with new data and headers
    void update(const std::vector<uint8_t>& data, const std::map<std::string, std::string>& headers);
    
private:
    CacheEntryMetadata m_metadata;
    std::vector<uint8_t> m_data;
    
    // Parse cache control headers
    void parseCacheControlHeaders(const std::map<std::string, std::string>& headers);
};

// Cache storage interface
class CacheStorage {
public:
    virtual ~CacheStorage() {}
    
    // Store entry
    virtual bool store(const CacheEntry& entry) = 0;
    
    // Load entry
    virtual bool load(const std::string& url, CacheEntry& entry) = 0;
    
    // Delete entry
    virtual bool remove(const std::string& url) = 0;
    
    // Clear all entries
    virtual void clear() = 0;
    
    // Check if entry exists
    virtual bool exists(const std::string& url) = 0;
    
    // Get all URLs in cache
    virtual std::vector<std::string> getAllUrls() = 0;
};

// Memory-based cache storage
class MemoryCacheStorage : public CacheStorage {
public:
    MemoryCacheStorage();
    virtual ~MemoryCacheStorage();
    
    // Store entry
    virtual bool store(const CacheEntry& entry) override;
    
    // Load entry
    virtual bool load(const std::string& url, CacheEntry& entry) override;
    
    // Delete entry
    virtual bool remove(const std::string& url) override;
    
    // Clear all entries
    virtual void clear() override;
    
    // Check if entry exists
    virtual bool exists(const std::string& url) override;
    
    // Get all URLs in cache
    virtual std::vector<std::string> getAllUrls() override;
    
private:
    std::map<std::string, CacheEntry> m_entries;
    std::mutex m_mutex;
};

// Disk-based cache storage (persistent)
class DiskCacheStorage : public CacheStorage {
public:
    DiskCacheStorage(const std::string& directory);
    virtual ~DiskCacheStorage();
    
    // Initialize storage
    bool initialize();
    
    // Store entry
    virtual bool store(const CacheEntry& entry) override;
    
    // Load entry
    virtual bool load(const std::string& url, CacheEntry& entry) override;
    
    // Delete entry
    virtual bool remove(const std::string& url) override;
    
    // Clear all entries
    virtual void clear() override;
    
    // Check if entry exists
    virtual bool exists(const std::string& url) override;
    
    // Get all URLs in cache
    virtual std::vector<std::string> getAllUrls() override;
    
private:
    std::string m_directory;
    std::mutex m_mutex;
    
    // Helpers
    std::string getFilePath(const std::string& url) const;
    std::string getMetadataPath(const std::string& url) const;
    std::string urlToFilename(const std::string& url) const;
    
    bool saveMetadata(const std::string& url, const CacheEntryMetadata& metadata);
    bool loadMetadata(const std::string& url, CacheEntryMetadata& metadata);
};

// Cache manager
class Cache {
public:
    Cache();
    ~Cache();
    
    // Initialize cache
    bool initialize(const std::string& cacheDirectory = "");
    
    // Get entry from cache
    bool get(const std::string& url, CacheEntry& entry);
    
    // Put entry in cache
    bool put(const CacheEntry& entry);
    
    // Clear cache
    void clear();
    
    // Set maximum memory cache size (in bytes)
    void setMaxMemoryCacheSize(size_t bytes);
    
    // Set maximum disk cache size (in bytes)
    void setMaxDiskCacheSize(size_t bytes);
    
private:
    // Cache storages
    std::unique_ptr<MemoryCacheStorage> m_memoryCache;
    std::unique_ptr<DiskCacheStorage> m_diskCache;
    
    // Cache sizes
    size_t m_maxMemoryCacheSize;
    size_t m_maxDiskCacheSize;
    size_t m_currentMemoryCacheSize;
    size_t m_currentDiskCacheSize;
    
    // Enforce cache size limits
    void enforceMemoryCacheSize();
    void enforceDiskCacheSize();
};

} // namespace networking
} // namespace browser

#endif // BROWSER_CACHE_H