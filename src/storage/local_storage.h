#ifndef BROWSER_LOCAL_STORAGE_H
#define BROWSER_LOCAL_STORAGE_H

#include <string>
#include <map>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <memory>
#include <functional>
#include "../security/same_origin.h"

namespace browser {
namespace storage {

// Forward declarations
class StorageArea;
class StorageManager;

// Local Storage item
struct StorageItem {
    std::string key;
    std::string value;
    std::chrono::system_clock::time_point timestamp;

    StorageItem() : timestamp(std::chrono::system_clock::now()) {}
    StorageItem(const std::string& k, const std::string& v) 
        : key(k), value(v), timestamp(std::chrono::system_clock::now()) {}
};

// Storage Area for a specific origin
class StorageArea {
public:
    StorageArea(const security::Origin& origin);
    ~StorageArea();

    // Basic localStorage API
    std::string getItem(const std::string& key) const;
    bool setItem(const std::string& key, const std::string& value);
    bool removeItem(const std::string& key);
    void clear();
    size_t length() const;
    std::string key(size_t index) const;
    bool contains(const std::string& key) const;

    // Get the associated origin
    const security::Origin& origin() const { return m_origin; }

    // Get the total size of data stored (in bytes)
    size_t size() const { return m_size; }

    // Serialize and deserialize the storage area
    bool saveToFile(const std::string& filePath) const;
    bool loadFromFile(const std::string& filePath);

private:
    security::Origin m_origin;
    std::map<std::string, StorageItem> m_items; // Using map for ordered keys
    size_t m_size; // Total size in bytes
    mutable std::mutex m_mutex;

    // Update size calculation after storage changes
    void updateSize();
};

// Storage Manager - handles multiple storage areas
class StorageManager {
public:
    StorageManager();
    ~StorageManager();

    // Initialize with storage directory
    bool initialize(const std::string& storageDirectory);

    // Get storage area for origin (creates if doesn't exist)
    std::shared_ptr<StorageArea> getStorageArea(const security::Origin& origin);

    // Check if origin has storage area
    bool hasStorageArea(const security::Origin& origin) const;

    // Clear all storage for origin
    bool clearOriginStorage(const security::Origin& origin);

    // Persist all storage to disk
    bool persistAllStorage();

    // Get/set storage quota per origin (in bytes)
    size_t getQuota() const { return m_quotaPerOrigin; }
    void setQuota(size_t bytes) { m_quotaPerOrigin = bytes; }

    // Estimate total storage size
    size_t getTotalStorageSize() const;

private:
    // Storage areas by origin
    std::unordered_map<std::string, std::shared_ptr<StorageArea>> m_storageAreas;
    
    // Storage directory on disk
    std::string m_storageDir;
    
    // Storage quota per origin (default: 5MB)
    size_t m_quotaPerOrigin;
    
    // Mutex for thread safety
    mutable std::mutex m_mutex;

    // Helper methods
    std::string getStorageFilePath(const security::Origin& origin) const;
    bool ensureDirectoryExists(const std::string& directory) const;
    bool loadStorageArea(const security::Origin& origin);
    bool saveStorageArea(const security::Origin& origin);
};

// Local Storage API wrapper for JavaScript binding
class LocalStorage {
public:
    LocalStorage(StorageManager* manager, const security::Origin& origin);
    ~LocalStorage();

    // Basic localStorage API methods
    std::string getItem(const std::string& key) const;
    bool setItem(const std::string& key, const std::string& value);
    bool removeItem(const std::string& key);
    void clear();
    size_t length() const;
    std::string key(size_t index) const;

    // Event callback for storage changes
    using StorageEventCallback = std::function<void(const std::string& key, 
                                                   const std::string& oldValue,
                                                   const std::string& newValue,
                                                   const security::Origin& origin)>;

    // Add storage event listener
    void addEventListener(StorageEventCallback callback);

    // Fire storage event (would be called when changes are made)
    void fireStorageEvent(const std::string& key, 
                         const std::string& oldValue,
                         const std::string& newValue);

private:
    StorageManager* m_manager;
    security::Origin m_origin;
    std::shared_ptr<StorageArea> m_storageArea;
    std::vector<StorageEventCallback> m_eventListeners;
};

} // namespace storage
} // namespace browser

#endif // BROWSER_LOCAL_STORAGE_H