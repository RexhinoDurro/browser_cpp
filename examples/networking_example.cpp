#include "http_client.h"
#include "dns_resolver.h"
#include "cache.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

using namespace browser::networking;

// Simple example of how to use the networking components
int main() {
    // Initialize networking components
    HttpClient httpClient;
    DnsResolver dnsResolver;
    Cache cache;
    
    if (!httpClient.initialize()) {
        std::cerr << "Failed to initialize HTTP client" << std::endl;
        return 1;
    }
    
    if (!dnsResolver.initialize()) {
        std::cerr << "Failed to initialize DNS resolver" << std::endl;
        return 1;
    }
    
    if (!cache.initialize("./browser_cache")) {
        std::cerr << "Failed to initialize cache" << std::endl;
        return 1;
    }
    
    // Example 1: Synchronous DNS resolution
    std::cout << "Resolving example.com..." << std::endl;
    std::vector<std::shared_ptr<DnsRecord>> records;
    std::string error;
    
    if (dnsResolver.resolve("example.com", DnsRecordType::A, records, error)) {
        for (const auto& record : records) {
            std::cout << "DNS Record: " << record->toString() << std::endl;
        }
    } else {
        std::cerr << "DNS resolution failed: " << error << std::endl;
    }
    
    // Example 2: Synchronous HTTP request
    std::cout << "\nFetching https://example.com..." << std::endl;
    
    // Check cache first
    CacheEntry cacheEntry;
    bool useCache = false;
    
    if (cache.get("https://example.com/", cacheEntry)) {
        std::cout << "Found in cache!" << std::endl;
        
        // Check if expired
        if (cacheEntry.isExpired()) {
            std::cout << "Cache entry expired, refetching..." << std::endl;
            
            // If entry can be validated, add validation headers
            if (cacheEntry.canBeValidated()) {
                HttpRequest request(HttpMethod::GET, "https://example.com/");
                
                // Add validation headers
                std::map<std::string, std::string> validationHeaders = cacheEntry.getValidationHeaders();
                for (const auto& header : validationHeaders) {
                    request.setHeader(header.first, header.second);
                }
                
                // Send request
                HttpResponse response = httpClient.sendRequest(request, error);
                
                // Check if not modified
                if (response.statusCode() == 304) {
                    std::cout << "Resource not modified, using cache" << std::endl;
                    useCache = true;
                } else if (response.statusCode() == 200) {
                    std::cout << "Resource modified, updating cache" << std::endl;
                    
                    // Update cache
                    cacheEntry.update(response.body(), response.headers());
                    cache.put(cacheEntry);
                } else {
                    std::cerr << "HTTP request failed: " << response.statusCode() << " " << response.statusText() << std::endl;
                }
            } else {
                // Refetch without validation
                HttpResponse response = httpClient.get("https://example.com/", error);
                
                if (response.statusCode() == 200) {
                    std::cout << "Refetched resource, updating cache" << std::endl;
                    
                    // Update cache
                    cacheEntry = CacheEntry("https://example.com/", response.body(), response.headers());
                    cache.put(cacheEntry);
                } else {
                    std::cerr << "HTTP request failed: " << response.statusCode() << " " << response.statusText() << std::endl;
                }
            }
        } else {
            std::cout << "Cache entry valid, using cache" << std::endl;
            useCache = true;
        }
    } else {
        std::cout << "Not found in cache, fetching..." << std::endl;
        
        // Fetch resource
        HttpResponse response = httpClient.get("https://example.com/", error);
        
        if (response.statusCode() == 200) {
            std::cout << "Fetched resource, caching" << std::endl;
            
            // Cache response
            cacheEntry = CacheEntry("https://example.com/", response.body(), response.headers());
            cache.put(cacheEntry);
        } else {
            std::cerr << "HTTP request failed: " << response.statusCode() << " " << response.statusText() << std::endl;
        }
    }
    
    // Display resource content
    if (!cacheEntry.data().empty()) {
        std::string body(cacheEntry.data().begin(), cacheEntry.data().end());
        std::cout << "\nFirst 100 bytes of content: " << body.substr(0, 100) << "..." << std::endl;
    }
    
    // Example 3: Asynchronous HTTP request
    std::cout << "\nAsynchronously fetching https://httpbin.org/get..." << std::endl;
    
    httpClient.getAsync("https://httpbin.org/get", [](const HttpResponse& response, const std::string& error) {
        if (error.empty() && response.statusCode() == 200) {
            std::cout << "Async request completed successfully!" << std::endl;
            std::cout << "Status: " << response.statusCode() << " " << response.statusText() << std::endl;
            std::cout << "Content-Type: " << response.getHeader("Content-Type") << std::endl;
            std::cout << "First 100 bytes: " << response.bodyAsString().substr(0, 100) << "..." << std::endl;
        } else {
            std::cerr << "Async request failed: " << error << std::endl;
        }
    });
    
    // Process pending requests (in a real browser, this would be in an event loop)
    std::cout << "Processing pending requests..." << std::endl;
    for (int i = 0; i < 10; i++) {
        httpClient.processPendingRequests();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Example 4: Multiple requests in parallel
    std::cout << "\nFetching multiple resources in parallel..." << std::endl;
    
    std::vector<std::string> urls = {
        "https://httpbin.org/get",
        "https://httpbin.org/headers",
        "https://httpbin.org/ip"
    };
    
    for (const auto& url : urls) {
        httpClient.getAsync(url, [url](const HttpResponse& response, const std::string& error) {
            if (error.empty() && response.statusCode() == 200) {
                std::cout << "Fetched " << url << ": " << response.bodyAsString().substr(0, 30) << "..." << std::endl;
            } else {
                std::cerr << "Failed to fetch " << url << ": " << error << std::endl;
            }
        });
    }
    
    // Process pending requests
    std::cout << "Processing pending requests..." << std::endl;
    for (int i = 0; i < 20; i++) {
        httpClient.processPendingRequests();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "\nNetworking example completed successfully!" << std::endl;
    
    return 0;
}