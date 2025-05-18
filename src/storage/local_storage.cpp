#include "local_storage.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <iomanip>

namespace fs = std::filesystem;

namespace browser {
namespace storage {

// Simple JSON serialization helpers
namespace {
    // Escape a string for JSON output
    std::string escapeJsonString(const std::string& str) {
        std::ostringstream oss;
        for (auto c : str) {
            switch (c) {
                case '\"': oss << "\\\""; break;
                case '\\': oss << "\\\\"; break;
                case '\b': oss << "\\b"; break;
                case '\f': oss << "\\f"; break;
                case '\n': oss << "\\n"; break;
                case '\r': oss << "\\r"; break;
                case '\t': oss << "\\t"; break;
                default:
                    if ('\x00' <= c && c <= '\x1f') {
                        // Control character - use \uXXXX format
                        oss << "\\u" 
                            << std::hex << std::setw(4) << std::setfill('0') 
                            << static_cast<int>(c);
                    } else {
                        oss << c;
                    }
            }
        }
        return oss.str();
    }

    // Unescape a JSON string
    std::string unescapeJsonString(const std::string& str) {
        std::ostringstream oss;
        for (size_t i = 0; i < str.length(); i++) {
            if (str[i] == '\\' && i + 1 < str.length()) {
                switch (str[i + 1]) {
                    case '\"': oss << '\"'; break;
                    case '\\': oss << '\\'; break;
                    case '/': oss << '/'; break;
                    case 'b': oss << '\b'; break;
                    case 'f': oss << '\f'; break;
                    case 'n': oss << '\n'; break;
                    case 'r': oss << '\r'; break;
                    case 't': oss << '\t'; break;
                    case 'u':
                        // Unicode escape - this is simplified and only handles basic cases
                        if (i + 5 < str.length()) {
                            std::string hex = str.substr(i + 2, 4);
                            try {
                                int code = std::stoi(hex, nullptr, 16);
                                oss << static_cast<char>(code); // This only works for ASCII range
                            } catch (const std::exception&) {
                                // Skip invalid escape sequence
                            }
                            i += 5;
                        }
                        break;
                    default:
                        oss << str[i + 1];
                }
                i++; // Skip the escaped character
            } else {
                oss << str[i];
            }
        }
        return oss.str();
    }

    // Parse a simple JSON object - this is very basic and only handles
    // the specific format we need for storage files
    std::map<std::string, std::string> parseJsonObject(const std::string& json) {
        std::map<std::string, std::string> result;
        
        // Find start of object
        size_t startPos = json.find('{');
        if (startPos == std::string::npos) {
            return result;
        }
        
        // Find end of object
        size_t endPos = json.rfind('}');
        if (endPos == std::string::npos || endPos <= startPos) {
            return result;
        }
        
        // Extract object content
        std::string objectContent = json.substr(startPos + 1, endPos - startPos - 1);
        
        // Parse key-value pairs
        size_t pos = 0;
        while (pos < objectContent.length()) {
            // Find key
            size_t keyStart = objectContent.find('\"', pos);
            if (keyStart == std::string::npos) break;
            
            size_t keyEnd = objectContent.find('\"', keyStart + 1);
            if (keyEnd == std::string::npos) break;
            
            std::string key = objectContent.substr(keyStart + 1, keyEnd - keyStart - 1);
            
            // Find colon
            size_t colonPos = objectContent.find(':', keyEnd + 1);
            if (colonPos == std::string::npos) break;
            
            // Find value
            size_t valueStart = objectContent.find_first_not_of(" \t\n\r", colonPos + 1);
            if (valueStart == std::string::npos) break;
            
            std::string value;
            
            if (objectContent[valueStart] == '\"') {
                // String value
                size_t valueEnd = objectContent.find('\"', valueStart + 1);
                while (valueEnd != std::string::npos && objectContent[valueEnd - 1] == '\\') {
                    // Skip escaped quotes
                    valueEnd = objectContent.find('\"', valueEnd + 1);
                }
                if (valueEnd == std::string::npos) break;
                
                value = objectContent.substr(valueStart + 1, valueEnd - valueStart - 1);
                value = unescapeJsonString(value);
                
                pos = objectContent.find(',', valueEnd + 1);
            } else if (objectContent[valueStart] == '{') {
                // Object value - skip for now
                int braceCount = 1;
                size_t valueEnd = valueStart + 1;
                while (valueEnd < objectContent.length() && braceCount > 0) {
                    if (objectContent[valueEnd] == '{') braceCount++;
                    else if (objectContent[valueEnd] == '}') braceCount--;
                    valueEnd++;
                }
                
                // Skip this value
                pos = objectContent.find(',', valueEnd);
            } else {
                // Other value (number, boolean, null)
                size_t valueEnd = objectContent.find_first_of(",}", valueStart);
                if (valueEnd == std::string::npos) valueEnd = objectContent.length();
                
                value = objectContent.substr(valueStart, valueEnd - valueStart);
                value = value.substr(0, value.find_last_not_of(" \t\n\r") + 1); // Trim right
                
                pos = valueEnd;
            }
            
            // Store key-value pair
            result[key] = value;
            
            // Move to next pair
            if (pos != std::string::npos) {
                pos++;
            } else {
                break;
            }
        }
        
        return result;
    }
    
    // Parse a JSON array of objects
    std::vector<std::map<std::string, std::string>> parseJsonArray(const std::string& json, const std::string& arrayName) {
        std::vector<std::map<std::string, std::string>> result;
        
        // Find array
        std::string arrayTag = "\"" + arrayName + "\":";
        size_t arrayPos = json.find(arrayTag);
        if (arrayPos == std::string::npos) {
            return result;
        }
        
        // Find start of array
        size_t startPos = json.find('[', arrayPos);
        if (startPos == std::string::npos) {
            return result;
        }
        
        // Find end of array
        size_t endPos = startPos + 1;
        int bracketCount = 1;
        while (endPos < json.length() && bracketCount > 0) {
            if (json[endPos] == '[') bracketCount++;
            else if (json[endPos] == ']') bracketCount--;
            endPos++;
        }
        
        if (bracketCount != 0) {
            return result;
        }
        
        // Extract array content
        std::string arrayContent = json.substr(startPos + 1, endPos - startPos - 2);
        
        // Parse array items
        size_t pos = 0;
        while (pos < arrayContent.length()) {
            // Find start of object
            size_t objectStart = arrayContent.find('{', pos);
            if (objectStart == std::string::npos) break;
            
            // Find end of object
            int braceCount = 1;
            size_t objectEnd = objectStart + 1;
            while (objectEnd < arrayContent.length() && braceCount > 0) {
                if (arrayContent[objectEnd] == '{') braceCount++;
                else if (arrayContent[objectEnd] == '}') braceCount--;
                objectEnd++;
            }
            
            if (braceCount != 0) break;
            
            // Parse object
            std::string objectJson = arrayContent.substr(objectStart, objectEnd - objectStart);
            std::map<std::string, std::string> object = parseJsonObject(objectJson);
            
            // Add to result
            result.push_back(object);
            
            // Move to next object
            pos = arrayContent.find(',', objectEnd);
            if (pos != std::string::npos) {
                pos++;
            } else {
                break;
            }
        }
        
        return result;
    }
}

//-----------------------------------------------------------------------------
// StorageArea Implementation
//-----------------------------------------------------------------------------

StorageArea::StorageArea(const security::Origin& origin)
    : m_origin(origin)
    , m_size(0)
{
}

StorageArea::~StorageArea() {
}

std::string StorageArea::getItem(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_items.find(key);
    if (it != m_items.end()) {
        // Update access timestamp
        const_cast<StorageItem&>(it->second).timestamp = std::chrono::system_clock::now();
        return it->second.value;
    }
    
    return "";
}

bool StorageArea::setItem(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check if updating an existing item
    auto it = m_items.find(key);
    if (it != m_items.end()) {
        // Calculate size change
        size_t oldSize = it->first.size() + it->second.value.size();
        size_t newSize = key.size() + value.size();
        
        // Update the item
        it->second.value = value;
        it->second.timestamp = std::chrono::system_clock::now();
        
        // Update total size
        m_size = m_size - oldSize + newSize;
        
        return true;
    }
    else {
        // Add new item
        StorageItem item(key, value);
        m_items[key] = item;
        
        // Update total size
        m_size += (key.size() + value.size());
        
        return true;
    }
}

bool StorageArea::removeItem(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_items.find(key);
    if (it != m_items.end()) {
        // Subtract size of removed item
        m_size -= (it->first.size() + it->second.value.size());
        
        // Remove item
        m_items.erase(it);
        return true;
    }
    
    return false;
}

void StorageArea::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_items.clear();
    m_size = 0;
}

size_t StorageArea::length() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_items.size();
}

std::string StorageArea::key(size_t index) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (index >= m_items.size()) {
        return "";
    }
    
    auto it = m_items.begin();
    std::advance(it, index);
    return it->first;
}

bool StorageArea::contains(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_items.find(key) != m_items.end();
}

void StorageArea::updateSize() {
    m_size = 0;
    for (const auto& item : m_items) {
        m_size += (item.first.size() + item.second.value.size());
    }
}

bool StorageArea::saveToFile(const std::string& filePath) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Create output file
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        
        // Create JSON structure manually
        file << "{\n";
        file << "    \"origin\": \"" << escapeJsonString(m_origin.toString()) << "\",\n";
        file << "    \"items\": [\n";
        
        size_t itemCount = 0;
        for (const auto& item : m_items) {
            file << "        {\n";
            file << "            \"key\": \"" << escapeJsonString(item.first) << "\",\n";
            file << "            \"value\": \"" << escapeJsonString(item.second.value) << "\",\n";
            
            // Store timestamp
            auto timestamp = std::chrono::system_clock::to_time_t(item.second.timestamp);
            file << "            \"timestamp\": " << timestamp << "\n";
            
            file << "        }";
            
            if (++itemCount < m_items.size()) {
                file << ",";
            }
            file << "\n";
        }
        
        file << "    ]\n";
        file << "}\n";
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving storage: " << e.what() << std::endl;
        return false;
    }
}

bool StorageArea::loadFromFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        // Check if file exists
        if (!fs::exists(filePath)) {
            return false;
        }
        
        // Read file content
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonContent = buffer.str();
        
        // Parse JSON manually
        std::map<std::string, std::string> jsonObj = parseJsonObject(jsonContent);
        
        // Verify origin matches
        auto originIt = jsonObj.find("origin");
        if (originIt == jsonObj.end() || originIt->second != "\"" + m_origin.toString() + "\"") {
            std::cerr << "Origin mismatch in storage file: " << filePath << std::endl;
            return false;
        }
        
        // Clear existing items
        m_items.clear();
        
        // Load items
        auto items = parseJsonArray(jsonContent, "items");
        for (const auto& itemObj : items) {
            StorageItem item;
            
            // Get key
            auto keyIt = itemObj.find("key");
            if (keyIt != itemObj.end()) {
                // Remove quotes
                std::string keyStr = keyIt->second;
                if (keyStr.size() >= 2 && keyStr.front() == '\"' && keyStr.back() == '\"') {
                    item.key = unescapeJsonString(keyStr.substr(1, keyStr.size() - 2));
                }
            }
            
            // Get value
            auto valueIt = itemObj.find("value");
            if (valueIt != itemObj.end()) {
                // Remove quotes
                std::string valueStr = valueIt->second;
                if (valueStr.size() >= 2 && valueStr.front() == '\"' && valueStr.back() == '\"') {
                    item.value = unescapeJsonString(valueStr.substr(1, valueStr.size() - 2));
                }
            }
            
            // Get timestamp if present
            auto timestampIt = itemObj.find("timestamp");
            if (timestampIt != itemObj.end()) {
                try {
                    std::time_t timestamp = std::stoll(timestampIt->second);
                    item.timestamp = std::chrono::system_clock::from_time_t(timestamp);
                } catch (const std::exception&) {
                    // Use current time if parsing fails
                    item.timestamp = std::chrono::system_clock::now();
                }
            }
            
            // Add item to storage
            if (!item.key.empty()) {
                m_items[item.key] = item;
            }
        }
        
        // Update size
        updateSize();
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading storage: " << e.what() << std::endl;
        return false;
    }
}

//-----------------------------------------------------------------------------
// StorageManager Implementation
//-----------------------------------------------------------------------------

StorageManager::StorageManager()
    : m_quotaPerOrigin(5 * 1024 * 1024) // Default 5MB per origin
{
}

StorageManager::~StorageManager() {
    // Persist all storage on shutdown
    persistAllStorage();
}

bool StorageManager::initialize(const std::string& storageDirectory) {
    m_storageDir = storageDirectory;
    
    // Create directory if it doesn't exist
    if (!ensureDirectoryExists(m_storageDir)) {
        std::cerr << "Failed to create storage directory: " << m_storageDir << std::endl;
        return false;
    }
    
    // Try to load existing storage areas
    try {
        for (const auto& entry : fs::directory_iterator(m_storageDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                // Extract origin from filename
                std::string filename = entry.path().stem().string();
                
                // Simple origin extraction from filename (in practice, this would be more robust)
                security::Origin origin(filename);
                
                // Load storage area
                loadStorageArea(origin);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error scanning storage directory: " << e.what() << std::endl;
        return false;
    }
    
    return true;
}

std::shared_ptr<StorageArea> StorageManager::getStorageArea(const security::Origin& origin) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string originStr = origin.toString();
    
    // Check if we already have this storage area
    auto it = m_storageAreas.find(originStr);
    if (it != m_storageAreas.end()) {
        return it->second;
    }
    
    // Create new storage area
    auto storageArea = std::make_shared<StorageArea>(origin);
    
    // Try to load existing data from disk
    std::string filePath = getStorageFilePath(origin);
    storageArea->loadFromFile(filePath);
    
    // Store in map
    m_storageAreas[originStr] = storageArea;
    
    return storageArea;
}

bool StorageManager::hasStorageArea(const security::Origin& origin) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_storageAreas.find(origin.toString()) != m_storageAreas.end();
}

bool StorageManager::clearOriginStorage(const security::Origin& origin) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string originStr = origin.toString();
    
    // Check if we have this storage area
    auto it = m_storageAreas.find(originStr);
    if (it != m_storageAreas.end()) {
        // Clear storage area
        it->second->clear();
        
        // Remove storage file
        std::string filePath = getStorageFilePath(origin);
        try {
            if (fs::exists(filePath)) {
                fs::remove(filePath);
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error removing storage file: " << e.what() << std::endl;
            return false;
        }
        
        return true;
    }
    
    return false;
}

bool StorageManager::persistAllStorage() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    bool success = true;
    
    for (const auto& [originStr, storageArea] : m_storageAreas) {
        std::string filePath = getStorageFilePath(storageArea->origin());
        if (!storageArea->saveToFile(filePath)) {
            std::cerr << "Failed to save storage for origin: " << originStr << std::endl;
            success = false;
        }
    }
    
    return success;
}

size_t StorageManager::getTotalStorageSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    size_t totalSize = 0;
    for (const auto& [originStr, storageArea] : m_storageAreas) {
        totalSize += storageArea->size();
    }
    
    return totalSize;
}

std::string StorageManager::getStorageFilePath(const security::Origin& origin) const {
    // Create a filename from the origin
    // In a real implementation, we'd need to properly encode the origin
    
    // Convert origin to safe filename
    std::string filename = origin.toString();
    
    // Replace characters that are invalid in filenames
    std::replace(filename.begin(), filename.end(), ':', '_');
    std::replace(filename.begin(), filename.end(), '/', '_');
    
    return m_storageDir + "/" + filename + ".json";
}

bool StorageManager::ensureDirectoryExists(const std::string& directory) const {
    try {
        // Check if directory exists
        if (fs::exists(directory)) {
            return fs::is_directory(directory);
        }
        
        // Create directory
        return fs::create_directories(directory);
    }
    catch (const std::exception& e) {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
        return false;
    }
}

bool StorageManager::loadStorageArea(const security::Origin& origin) {
    std::string filePath = getStorageFilePath(origin);
    
    // Check if file exists
    if (!fs::exists(filePath)) {
        return false;
    }
    
    // Create storage area
    auto storageArea = std::make_shared<StorageArea>(origin);
    
    // Load from file
    if (!storageArea->loadFromFile(filePath)) {
        return false;
    }
    
    // Add to map
    m_storageAreas[origin.toString()] = storageArea;
    
    return true;
}

bool StorageManager::saveStorageArea(const security::Origin& origin) {
    std::string originStr = origin.toString();
    
    // Check if we have this storage area
    auto it = m_storageAreas.find(originStr);
    if (it == m_storageAreas.end()) {
        return false;
    }
    
    // Save to file
    std::string filePath = getStorageFilePath(origin);
    return it->second->saveToFile(filePath);
}

//-----------------------------------------------------------------------------
// LocalStorage Implementation
//-----------------------------------------------------------------------------

LocalStorage::LocalStorage(StorageManager* manager, const security::Origin& origin)
    : m_manager(manager)
    , m_origin(origin)
{
    if (m_manager) {
        m_storageArea = m_manager->getStorageArea(origin);
    }
}

LocalStorage::~LocalStorage() {
    // Nothing to do here
}

std::string LocalStorage::getItem(const std::string& key) const {
    if (m_storageArea) {
        return m_storageArea->getItem(key);
    }
    return "";
}

bool LocalStorage::setItem(const std::string& key, const std::string& value) {
    if (!m_storageArea) {
        return false;
    }
    
    // Get the old value before setting new one (for event)
    std::string oldValue = m_storageArea->getItem(key);
    
    // Set the item
    bool result = m_storageArea->setItem(key, value);
    
    // Fire storage event
    if (result) {
        fireStorageEvent(key, oldValue, value);
    }
    
    return result;
}

bool LocalStorage::removeItem(const std::string& key) {
    if (!m_storageArea) {
        return false;
    }
    
    // Get the old value before removing (for event)
    std::string oldValue = m_storageArea->getItem(key);
    
    // Remove the item
    bool result = m_storageArea->removeItem(key);
    
    // Fire storage event
    if (result) {
        fireStorageEvent(key, oldValue, "");
    }
    
    return result;
}

void LocalStorage::clear() {
    if (m_storageArea) {
        m_storageArea->clear();
        
        // Fire storage event for clear
        fireStorageEvent(nullptr, "", "");
    }
}

size_t LocalStorage::length() const {
    return m_storageArea ? m_storageArea->length() : 0;
}

std::string LocalStorage::key(size_t index) const {
    return m_storageArea ? m_storageArea->key(index) : "";
}

void LocalStorage::addEventListener(StorageEventCallback callback) {
    m_eventListeners.push_back(callback);
}

void LocalStorage::fireStorageEvent(const std::string& key, 
                                  const std::string& oldValue,
                                  const std::string& newValue) {
    // Notify all listeners
    for (const auto& listener : m_eventListeners) {
        listener(key, oldValue, newValue, m_origin);
    }
}

} // namespace storage
} // namespace browser