#include "cache.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iomanip>

// For cross-platform filesystem operations
namespace fs = std::filesystem;

namespace browser {
namespace networking {

//-----------------------------------------------------------------------------
// CacheEntry Implementation
//-----------------------------------------------------------------------------

CacheEntry::CacheEntry() {
}

CacheEntry::CacheEntry(const std::string& url, const std::vector<uint8_t>& data,
                     const std::map<std::string, std::string>& headers)
    : m_data(data)
{
    m_metadata.url = url;
    m_metadata.headers = headers;
    m_metadata.timestamp = std::chrono::system_clock::now();
    
    // Extract ETag and Last-Modified headers
    for (const auto& header : headers) {
        if (strcasecmp(header.first.c_str(), "ETag") == 0) {
            m_metadata.etag = header.second;
        } else if (strcasecmp(header.first.c_str(), "Last-Modified") == 0) {
            m_metadata.lastModified = header.second;
        }
    }
    
    // Parse cache control headers to determine expiry
    parseCacheControlHeaders(headers);
}

CacheEntry::~CacheEntry() {
}

bool CacheEntry::isExpired() const {
    return std::chrono::system_clock::now() > m_metadata.expires;
}

bool CacheEntry::canBeValidated() const {
    return !m_metadata.etag.empty() || !m_metadata.lastModified.empty();
}

std::map<std::string, std::string> CacheEntry::getValidationHeaders() const {
    std::map<std::string, std::string> headers;
    
    if (!m_metadata.etag.empty()) {
        headers["If-None-Match"] = m_metadata.etag;
    }
    
    if (!m_metadata.lastModified.empty()) {
        headers["If-Modified-Since"] = m_metadata.lastModified;
    }
    
    return headers;
}

void CacheEntry::update(const std::vector<uint8_t>& data, const std::map<std::string, std::string>& headers) {
    m_data = data;
    m_metadata.headers = headers;
    m_metadata.timestamp = std::chrono::system_clock::now();
    
    // Extract ETag and Last-Modified headers
    m_metadata.etag.clear();
    m_metadata.lastModified.clear();
    
    for (const auto& header : headers) {
        if (strcasecmp(header.first.c_str(), "ETag") == 0) {
            m_metadata.etag = header.second;
        } else if (strcasecmp(header.first.c_str(), "Last-Modified") == 0) {
            m_metadata.lastModified = header.second;
        }
    }
    
    // Parse cache control headers to determine expiry
    parseCacheControlHeaders(headers);
}

void CacheEntry::parseCacheControlHeaders(const std::map<std::string, std::string>& headers) {
    // Default expiry (1 hour)
    m_metadata.expires = m_metadata.timestamp + std::chrono::hours(1);
    
    // Look for Expires header
    for (const auto& header : headers) {
        if (strcasecmp(header.first.c_str(), "Expires") == 0) {
            // TODO: Parse HTTP date format
            // For now, just using default expiry
            break;
        }
    }
    
    // Look for Cache-Control header
    for (const auto& header : headers) {
        if (strcasecmp(header.first.c_str(), "Cache-Control") == 0) {
            std::string cacheControlValue = header.second;
            std::istringstream iss(cacheControlValue);
            std::string directive;
            
            while (std::getline(iss, directive, ',')) {
                // Trim whitespace
                directive.erase(0, directive.find_first_not_of(" \t"));
                directive.erase(directive.find_last_not_of(" \t") + 1);
                
                // Convert to lowercase
                std::transform(directive.begin(), directive.end(), directive.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                
                if (directive == "no-cache" || directive == "no-store") {
                    // Don't cache
                    m_metadata.expires = m_metadata.timestamp;
                    return;
                } else if (directive.find("max-age=") == 0) {
                    // Parse max-age value
                    std::string maxAgeValue = directive.substr(8);
                    try {
                        int maxAge = std::stoi(maxAgeValue);
                        m_metadata.expires = m_metadata.timestamp + std::chrono::seconds(maxAge);
                        return;
                    } catch (const std::exception&) {
                        // Ignore invalid max-age value
                    }
                }
            }
            
            break;
        }
    }
}

//-----------------------------------------------------------------------------
// MemoryCacheStorage Implementation
//-----------------------------------------------------------------------------

MemoryCacheStorage::MemoryCacheStorage() {
}

MemoryCacheStorage::~MemoryCacheStorage() {
}

bool MemoryCacheStorage::store(const CacheEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Store or update entry
    m_entries[entry.metadata().url] = entry;
    
    return true;
}

bool MemoryCacheStorage::load(const std::string& url, CacheEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find entry
    auto it = m_entries.find(url);
    if (it == m_entries.end()) {
        return false;
    }
    
    // Copy entry
    entry = it->second;
    
    return true;
}

bool MemoryCacheStorage::remove(const std::string& url) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Find and remove entry
    auto it = m_entries.find(url);
    if (it == m_entries.end()) {
        return false;
    }
    
    m_entries.erase(it);
    
    return true;
}

void MemoryCacheStorage::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Clear all entries
    m_entries.clear();
}

bool MemoryCacheStorage::exists(const std::string& url) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if entry exists
    return m_entries.find(url) != m_entries.end();
}

std::vector<std::string> MemoryCacheStorage::getAllUrls() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Get all URLs
    std::vector<std::string> urls;
    urls.reserve(m_entries.size());
    
    for (const auto& entry : m_entries) {
        urls.push_back(entry.first);
    }
    
    return urls;
}

//-----------------------------------------------------------------------------
// DiskCacheStorage Implementation
//-----------------------------------------------------------------------------

DiskCacheStorage::DiskCacheStorage(const std::string& directory)
    : m_directory(directory)
{
}

DiskCacheStorage::~DiskCacheStorage() {
}

bool DiskCacheStorage::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Create directory if it doesn't exist
        if (!fs::exists(m_directory)) {
            if (!fs::create_directories(m_directory)) {
                std::cerr << "Failed to create cache directory: " << m_directory << std::endl;
                return false;
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing disk cache: " << e.what() << std::endl;
        return false;
    }
}

bool DiskCacheStorage::store(const CacheEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Save metadata
        if (!saveMetadata(entry.metadata().url, entry.metadata())) {
            return false;
        }
        
        // Save data
        std::string filePath = getFilePath(entry.metadata().url);
        std::ofstream file(filePath, std::ios::binary);
        if (!file) {
            return false;
        }
        
        file.write(reinterpret_cast<const char*>(entry.data().data()), entry.data().size());
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error storing cache entry: " << e.what() << std::endl;
        return false;
    }
}

bool DiskCacheStorage::load(const std::string& url, CacheEntry& entry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Load metadata
        CacheEntryMetadata metadata;
        if (!loadMetadata(url, metadata)) {
            return false;
        }
        
        // Load data
        std::string filePath = getFilePath(url);
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file) {
            return false;
        }
        
        // Get file size
        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        // Read data
        std::vector<uint8_t> data(fileSize);
        if (!file.read(reinterpret_cast<char*>(data.data()), fileSize)) {
            return false;
        }
        
        // Create entry
        entry = CacheEntry(url, data, metadata.headers);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading cache entry: " << e.what() << std::endl;
        return false;
    }
}

bool DiskCacheStorage::remove(const std::string& url) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Remove data file
        std::string filePath = getFilePath(url);
        if (fs::exists(filePath)) {
            if (!fs::remove(filePath)) {
                return false;
            }
        }
        
        // Remove metadata file
        std::string metadataPath = getMetadataPath(url);
        if (fs::exists(metadataPath)) {
            if (!fs::remove(metadataPath)) {
                return false;
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error removing cache entry: " << e.what() << std::endl;
        return false;
    }
}

void DiskCacheStorage::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Remove all files in cache directory
        for (const auto& entry : fs::directory_iterator(m_directory)) {
            fs::remove_all(entry.path());
        }
    } catch (const std::exception& e) {
        std::cerr << "Error clearing cache: " << e.what() << std::endl;
    }
}

bool DiskCacheStorage::exists(const std::string& url) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Check if files exist
        std::string filePath = getFilePath(url);
        std::string metadataPath = getMetadataPath(url);
        
        return fs::exists(filePath) && fs::exists(metadataPath);
    } catch (const std::exception& e) {
        std::cerr << "Error checking cache entry: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> DiskCacheStorage::getAllUrls() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<std::string> urls;
    
    try {
        // Find all metadata files
        for (const auto& entry : fs::directory_iterator(m_directory)) {
            if (entry.path().extension() == ".meta") {
                // Extract URL from filename
                std::string filename = entry.path().stem().string();
                
                // Append to list
                urls.push_back(filename);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting cache URLs: " << e.what() << std::endl;
    }
    
    return urls;
}

std::string DiskCacheStorage::getFilePath(const std::string& url) const {
    return m_directory + "/" + urlToFilename(url) + ".data";
}

std::string DiskCacheStorage::getMetadataPath(const std::string& url) const {
    return m_directory + "/" + urlToFilename(url) + ".meta";
}

std::string DiskCacheStorage::urlToFilename(const std::string& url) const {
    // Hash the URL to create a valid filename
    std::hash<std::string> hasher;
    size_t hash = hasher(url);
    
    std::stringstream ss;
    ss << std::hex << hash;
    
    return ss.str();
}

bool DiskCacheStorage::saveMetadata(const std::string& url, const CacheEntryMetadata& metadata) {
    try {
        // Create metadata file
        std::string metadataPath = getMetadataPath(url);
        std::ofstream file(metadataPath);
        if (!file) {
            return false;
        }
        
        // Write metadata
        file << "URL: " << metadata.url << std::endl;
        file << "ETag: " << metadata.etag << std::endl;
        file << "Last-Modified: " << metadata.lastModified << std::endl;
        
        // Write timestamp and expires
        auto timestampTime = std::chrono::system_clock::to_time_t(metadata.timestamp);
        auto expiresTime = std::chrono::system_clock::to_time_t(metadata.expires);
        
        file << "Timestamp: " << timestampTime << std::endl;
        file << "Expires: " << expiresTime << std::endl;
        
        // Write headers
        file << "Headers: " << metadata.headers.size() << std::endl;
        for (const auto& header : metadata.headers) {
            file << header.first << ": " << header.second << std::endl;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving metadata: " << e.what() << std::endl;
        return false;
    }
}

bool DiskCacheStorage::loadMetadata(const std::string& url, CacheEntryMetadata& metadata) {
    try {
        // Open metadata file
        std::string metadataPath = getMetadataPath(url);
        std::ifstream file(metadataPath);
        if (!file) {
            return false;
        }
        
        // Read metadata
        std::string line;
        
        // URL
        if (!std::getline(file, line) || line.substr(0, 5) != "URL: ") {
            return false;
        }
        metadata.url = line.substr(5);
        
        // ETag
        if (!std::getline(file, line) || line.substr(0, 6) != "ETag: ") {
            return false;
        }
        metadata.etag = line.substr(6);
        
        // Last-Modified
        if (!std::getline(file, line) || line.substr(0, 15) != "Last-Modified: ") {
            return false;
        }
        metadata.lastModified = line.substr(15);
        
        // Timestamp
        if (!std::getline(file, line) || line.substr(0, 11) != "Timestamp: ") {
            return false;
        }
        time_t timestampTime = std::stoll(line.substr(11));
        metadata.timestamp = std::chrono::system_clock::from_time_t(timestampTime);
        
        // Expires
        if (!std::getline(file, line) || line.substr(0, 9) != "Expires: ") {
            return false;
        }
        time_t expiresTime = std::stoll(line.substr(9));
        metadata.expires = std::chrono::system_clock::from_time_t(expiresTime);
        
        // Headers
        if (!std::getline(file, line) || line.substr(0, 9) != "Headers: ") {
            return false;
        }
        int headerCount = std::stoi(line.substr(9));
        
        metadata.headers.clear();
        
        for (int i = 0; i < headerCount; i++) {
            if (!std::getline(file, line)) {
                return false;
            }
            
            size_t colonPos = line.find(": ");
            if (colonPos == std::string::npos) {
                return false;
            }
            
            std::string name = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 2);
            
            metadata.headers[name] = value;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading metadata: " << e.what() << std::endl;
        return false;
    }
}

//-----------------------------------------------------------------------------
// Cache Implementation
//-----------------------------------------------------------------------------

Cache::Cache()
    : m_maxMemoryCacheSize(10 * 1024 * 1024)  // 10 MB
    , m_maxDiskCacheSize(100 * 1024 * 1024)   // 100 MB
    , m_currentMemoryCacheSize(0)
    , m_currentDiskCacheSize(0)
{
}

Cache::~Cache() {
}

bool Cache::initialize(const std::string& cacheDirectory) {
    // Initialize memory cache
    m_memoryCache = std::make_unique<MemoryCacheStorage>();
    
    // Initialize disk cache if directory provided
    if (!cacheDirectory.empty()) {
        m_diskCache = std::make_unique<DiskCacheStorage>(cacheDirectory);
        if (!m_diskCache->initialize()) {
            m_diskCache.reset();
            return false;
        }
    }
    
    return true;
}

bool Cache::get(const std::string& url, CacheEntry& entry) {
    // Try memory cache first
    if (m_memoryCache && m_memoryCache->load(url, entry)) {
        return true;
    }
    
    // Then try disk cache
    if (m_diskCache && m_diskCache->load(url, entry)) {
        // Add to memory cache for faster access next time
        if (m_memoryCache) {
            m_memoryCache->store(entry);
            
            // Update memory cache size
            m_currentMemoryCacheSize += entry.data().size();
            enforceMemoryCacheSize();
        }
        
        return true;
    }
    
    return false;
}

bool Cache::put(const CacheEntry& entry) {
    // Check cache control headers
    if (entry.isExpired()) {
        return false;
    }
    
    // Store in memory cache
    if (m_memoryCache) {
        m_memoryCache->store(entry);
        
        // Update memory cache size
        m_currentMemoryCacheSize += entry.data().size();
        enforceMemoryCacheSize();
    }
    
    // Store in disk cache
    if (m_diskCache) {
        m_diskCache->store(entry);
        
        // Update disk cache size
        m_currentDiskCacheSize += entry.data().size();
        enforceDiskCacheSize();
    }
    
    return true;
}

void Cache::clear() {
    // Clear memory cache
    if (m_memoryCache) {
        m_memoryCache->clear();
        m_currentMemoryCacheSize = 0;
    }
    
    // Clear disk cache
    if (m_diskCache) {
        m_diskCache->clear();
        m_currentDiskCacheSize = 0;
    }
}

void Cache::setMaxMemoryCacheSize(size_t bytes) {
    m_maxMemoryCacheSize = bytes;
    enforceMemoryCacheSize();
}

void Cache::setMaxDiskCacheSize(size_t bytes) {
    m_maxDiskCacheSize = bytes;
    enforceDiskCacheSize();
}

void Cache::enforceMemoryCacheSize() {
    if (!m_memoryCache || m_currentMemoryCacheSize <= m_maxMemoryCacheSize) {
        return;
    }
    
    // Get all URLs
    std::vector<std::string> urls = m_memoryCache->getAllUrls();
    
    // Remove oldest entries until size is below limit
    for (const std::string& url : urls) {
        CacheEntry entry;
        if (m_memoryCache->load(url, entry)) {
            // Remove entry
            m_memoryCache->remove(url);
            
            // Update size
            m_currentMemoryCacheSize -= entry.data().size();
            
            // Check if size is below limit
            if (m_currentMemoryCacheSize <= m_maxMemoryCacheSize) {
                break;
            }
        }
    }
}

void Cache::enforceDiskCacheSize() {
    if (!m_diskCache || m_currentDiskCacheSize <= m_maxDiskCacheSize) {
        return;
    }
    
    // Get all URLs
    std::vector<std::string> urls = m_diskCache->getAllUrls();
    
    // Remove oldest entries until size is below limit
    for (const std::string& url : urls) {
        CacheEntry entry;
        if (m_diskCache->load(url, entry)) {
            // Remove entry
            m_diskCache->remove(url);
            
            // Update size
            m_currentDiskCacheSize -= entry.data().size();
            
            // Check if size is below limit
            if (m_currentDiskCacheSize <= m_maxDiskCacheSize) {
                break;
            }
        }
    }
}

namespace browser {
namespace networking {

namespace {
// Helper for case-insensitive string comparison
int strcasecmp(const char* s1, const char* s2) {
    #ifdef _WIN32
    return _stricmp(s1, s2);
    #else
    return ::strcasecmp(s1, s2);
    #endif
}
}

} // namespace networking
} // namespace browser