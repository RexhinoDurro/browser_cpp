#include "http_client.h"
#include "dns_resolver.h"
#include "cache.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

namespace browser {
namespace networking {

// Resource type enum
enum class ResourceType {
    HTML,
    CSS,
    JAVASCRIPT,
    IMAGE,
    FONT,
    OTHER
};

// Resource request class
class ResourceRequest {
public:
    ResourceRequest(const std::string& url, ResourceType type)
        : m_url(url)
        , m_type(type)
        , m_priority(0)
        , m_isComplete(false)
    {
    }
    
    // Getters
    std::string url() const { return m_url; }
    ResourceType type() const { return m_type; }
    int priority() const { return m_priority; }
    bool isComplete() const { return m_isComplete; }
    
    // Setters
    void setPriority(int priority) { m_priority = priority; }
    void setComplete(bool complete) { m_isComplete = complete; }
    
    // Set completion callback
    void setCompletionCallback(std::function<void(const std::vector<uint8_t>&, const std::map<std::string, std::string>&)> callback) {
        m_completionCallback = callback;
    }
    
    // Call completion callback
    void notifyCompletion(const std::vector<uint8_t>& data, const std::map<std::string, std::string>& headers) {
        if (m_completionCallback) {
            m_completionCallback(data, headers);
        }
    }
    
private:
    std::string m_url;
    ResourceType m_type;
    int m_priority;
    std::atomic<bool> m_isComplete;
    std::function<void(const std::vector<uint8_t>&, const std::map<std::string, std::string>&)> m_completionCallback;
};

// Resource loader class
class ResourceLoader {
public:
    ResourceLoader()
        : m_isRunning(false)
    {
    }
    
    ~ResourceLoader() {
        stop();
    }
    
    // Initialize the resource loader
    bool initialize(const std::string& cacheDirectory = "") {
        // Initialize networking components
        if (!m_httpClient.initialize()) {
            return false;
        }
        
        if (!m_dnsResolver.initialize()) {
            return false;
        }
        
        if (!m_cache.initialize(cacheDirectory)) {
            return false;
        }
        
        return true;
    }
    
    // Start the resource loader thread
    void start() {
        if (m_isRunning) {
            return;
        }
        
        m_isRunning = true;
        m_thread = std::thread(&ResourceLoader::run, this);
    }
    
    // Stop the resource loader thread
    void stop() {
        if (!m_isRunning) {
            return;
        }
        
        m_isRunning = false;
        m_condition.notify_all();
        
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }
    
    // Queue a resource request
    void queueRequest(std::shared_ptr<ResourceRequest> request) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_requestQueue.push(request);
        m_condition.notify_one();
    }
    
    // Check if a resource is in the cache
    bool isResourceCached(const std::string& url) {
        CacheEntry entry;
        return m_cache.get(url, entry) && !entry.isExpired();
    }
    
    // Get a resource from the cache
    bool getResourceFromCache(const std::string& url, std::vector<uint8_t>& data, std::map<std::string, std::string>& headers) {
        CacheEntry entry;
        if (m_cache.get(url, entry) && !entry.isExpired()) {
            data = entry.data();
            headers = entry.metadata().headers;
            return true;
        }
        return false;
    }
    
    // Load a resource (synchronous)
    bool loadResource(const std::string& url, std::vector<uint8_t>& data, 
                     std::map<std::string, std::string>& headers,
                     std::string& error) {
        // Check cache first
        CacheEntry cacheEntry;
        if (m_cache.get(url, cacheEntry)) {
            // Check if expired
            if (cacheEntry.isExpired()) {
                // If entry can be validated, add validation headers
                if (cacheEntry.canBeValidated()) {
                    HttpRequest request(HttpMethod::GET, url);
                    
                    // Add validation headers
                    std::map<std::string, std::string> validationHeaders = cacheEntry.getValidationHeaders();
                    for (const auto& header : validationHeaders) {
                        request.setHeader(header.first, header.second);
                    }
                    
                    // Send request
                    HttpResponse response = m_httpClient.sendRequest(request, error);
                    
                    // Check if not modified
                    if (response.statusCode() == 304) {
                        // Resource not modified, use cache
                        data = cacheEntry.data();
                        headers = cacheEntry.metadata().headers;
                        return true;
                    } else if (response.statusCode() == 200) {
                        // Resource modified, update cache
                        cacheEntry.update(response.body(), response.headers());
                        m_cache.put(cacheEntry);
                        
                        data = response.body();
                        headers = response.headers();
                        return true;
                    } else {
                        error = "HTTP request failed: " + std::to_string(response.statusCode()) + " " + response.statusText();
                        return false;
                    }
                } else {
                    // Refetch without validation
                    HttpResponse response = m_httpClient.get(url, error);
                    
                    if (response.statusCode() == 200) {
                        // Update cache
                        cacheEntry = CacheEntry(url, response.body(), response.headers());
                        m_cache.put(cacheEntry);
                        
                        data = response.body();
                        headers = response.headers();
                        return true;
                    } else {
                        error = "HTTP request failed: " + std::to_string(response.statusCode()) + " " + response.statusText();
                        return false;
                    }
                }
            } else {
                // Cache entry valid, use cache
                data = cacheEntry.data();
                headers = cacheEntry.metadata().headers;
                return true;
            }
        } else {
            // Not found in cache, fetch
            HttpResponse response = m_httpClient.get(url, error);
            
            if (response.statusCode() == 200) {
                // Cache response
                cacheEntry = CacheEntry(url, response.body(), response.headers());
                m_cache.put(cacheEntry);
                
                data = response.body();
                headers = response.headers();
                return true;
            } else {
                error = "HTTP request failed: " + std::to_string(response.statusCode()) + " " + response.statusText();
                return false;
            }
        }
    }
    
private:
    // Thread function
    void run() {
        while (m_isRunning) {
            std::shared_ptr<ResourceRequest> request;
            
            // Get a request from the queue
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait(lock, [this] { return !m_isRunning || !m_requestQueue.empty(); });
                
                if (!m_isRunning) {
                    break;
                }
                
                if (m_requestQueue.empty()) {
                    continue;
                }
                
                request = m_requestQueue.front();
                m_requestQueue.pop();
            }
            
            // Process the request
            if (request) {
                // Check cache first
                CacheEntry cacheEntry;
                if (m_cache.get(request->url(), cacheEntry)) {
                    // Check if expired
                    if (cacheEntry.isExpired()) {
                        // If entry can be validated, add validation headers
                        if (cacheEntry.canBeValidated()) {
                            // Asynchronously validate
                            HttpRequest httpRequest(HttpMethod::GET, request->url());
                            
                            // Add validation headers
                            std::map<std::string, std::string> validationHeaders = cacheEntry.getValidationHeaders();
                            for (const auto& header : validationHeaders) {
                                httpRequest.setHeader(header.first, header.second);
                            }
                            
                            // Send request asynchronously
                            m_httpClient.sendRequestAsync(httpRequest, [this, request, cacheEntry](const HttpResponse& response, const std::string& error) {
                                // Check if not modified
                                if (response.statusCode() == 304) {
                                    // Resource not modified, use cache
                                    request->notifyCompletion(cacheEntry.data(), cacheEntry.metadata().headers);
                                } else if (response.statusCode() == 200) {
                                    // Resource modified, update cache
                                    CacheEntry newEntry(request->url(), response.body(), response.headers());
                                    m_cache.put(newEntry);
                                    
                                    request->notifyCompletion(response.body(), response.headers());
                                } else {
                                    // Request failed, use stale cache entry
                                    request->notifyCompletion(cacheEntry.data(), cacheEntry.metadata().headers);
                                }
                                
                                request->setComplete(true);
                            });
                        } else {
                            // Refetch without validation
                            m_httpClient.getAsync(request->url(), [this, request](const HttpResponse& response, const std::string& error) {
                                if (response.statusCode() == 200) {
                                    // Cache response
                                    CacheEntry entry(request->url(), response.body(), response.headers());
                                    m_cache.put(entry);
                                    
                                    request->notifyCompletion(response.body(), response.headers());
                                }
                                
                                request->setComplete(true);
                            });
                        }
                    } else {
                        // Cache entry valid, use cache
                        request->notifyCompletion(cacheEntry.data(), cacheEntry.metadata().headers);
                        request->setComplete(true);
                    }
                } else {
                    // Not found in cache, fetch
                    m_httpClient.getAsync(request->url(), [this, request](const HttpResponse& response, const std::string& error) {
                        if (response.statusCode() == 200) {
                            // Cache response
                            CacheEntry entry(request->url(), response.body(), response.headers());
                            m_cache.put(entry);
                            
                            request->notifyCompletion(response.body(), response.headers());
                        }
                        
                        request->setComplete(true);
                    });
                }
            }
            
            // Process any pending requests
            m_httpClient.processPendingRequests();
            m_dnsResolver.processPendingResolutions();
        }
    }
    
    HttpClient m_httpClient;
    DnsResolver m_dnsResolver;
    Cache m_cache;
    
    std::thread m_thread;
    std::atomic<bool> m_isRunning;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::queue<std::shared_ptr<ResourceRequest>> m_requestQueue;
};

} // namespace networking
} // namespace browser