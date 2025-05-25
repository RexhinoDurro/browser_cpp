# Networking Components Documentation

## Overview

The networking layer provides HTTP/HTTPS communication, DNS resolution, caching, and resource loading capabilities for the browser. It handles all network requests and responses, implements caching strategies, and manages asynchronous resource loading.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Resource Loader                           │
│  ┌─────────────┐  ┌──────────────┐  ┌─────────────────┐   │
│  │ HTTP Client │  │ DNS Resolver │  │ Cache Manager   │   │
│  └──────┬──────┘  └──────┬───────┘  └────────┬────────┘   │
│         │                │                    │             │
│         └────────────────┴────────────────────┘             │
│                          │                                   │
│  ┌──────────────────────┴────────────────────────────────┐ │
│  │               Platform Network Layer                    │ │
│  │            (Sockets, SSL/TLS, etc.)                   │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## HTTP Client

### Overview

The HTTP client (`http_client.h/cpp`) provides synchronous and asynchronous HTTP request capabilities with support for:
- Multiple HTTP methods (GET, POST, PUT, DELETE, etc.)
- Custom headers
- Request/response bodies
- Automatic redirect following
- Connection timeouts
- SSL/TLS support (planned)

### Key Classes

#### HttpRequest

```cpp
class HttpRequest {
public:
    HttpRequest(HttpMethod method, const std::string& url);
    
    // Set request properties
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::vector<uint8_t>& body);
    
    // Parse URL components
    bool parseUrl(std::string& protocol, std::string& host, 
                 std::string& path, int& port) const;
};
```

#### HttpResponse

```cpp
class HttpResponse {
public:
    // Response status
    int statusCode() const;
    std::string statusText() const;
    
    // Headers and body
    bool hasHeader(const std::string& name) const;
    std::string getHeader(const std::string& name) const;
    const std::vector<uint8_t>& body() const;
    
    // Parse response from raw data
    bool parseResponse(const std::vector<uint8_t>& data);
};
```

#### HttpClient

```cpp
class HttpClient {
public:
    // Synchronous requests
    HttpResponse sendRequest(const HttpRequest& request, std::string& error);
    HttpResponse get(const std::string& url, std::string& error);
    HttpResponse post(const std::string& url, const std::string& body, 
                     const std::string& contentType, std::string& error);
    
    // Asynchronous requests
    void sendRequestAsync(const HttpRequest& request, HttpResponseCallback callback);
    void getAsync(const std::string& url, HttpResponseCallback callback);
    
    // Configuration
    void setTimeout(int seconds);
    void setMaxRedirects(int maxRedirects);
};
```

### Usage Examples

```cpp
// Synchronous GET request
HttpClient client;
client.initialize();

std::string error;
HttpResponse response = client.get("https://example.com", error);

if (response.statusCode() == 200) {
    std::string body = response.bodyAsString();
    std::cout << "Response: " << body << std::endl;
}

// Asynchronous POST request
client.postAsync("https://api.example.com/data", 
                 "{\"key\": \"value\"}", 
                 "application/json",
                 [](const HttpResponse& response, const std::string& error) {
                     if (error.empty()) {
                         std::cout << "Success: " << response.statusCode() << std::endl;
                     }
                 });
```

## DNS Resolver

### Overview

The DNS resolver (`dns_resolver.h/cpp`) provides hostname resolution with caching:
- IPv4 (A records) and IPv6 (AAAA records) support
- Asynchronous resolution
- TTL-based caching
- Platform-independent implementation

### Key Classes

#### DnsResolver

```cpp
class DnsResolver {
public:
    // Synchronous resolution
    bool resolve(const std::string& hostname, DnsRecordType type, 
                std::vector<std::shared_ptr<DnsRecord>>& records, 
                std::string& error);
    
    // Asynchronous resolution
    void resolveAsync(const std::string& hostname, DnsRecordType type, 
                     DnsResolverCallback callback);
    
    // Cache management
    void setMaxCacheTime(int seconds);
    void clearCache();
};
```

#### DNS Records

```cpp
// Base class
class DnsRecord {
public:
    std::string name() const;
    DnsRecordType type() const;
    int ttl() const;
    bool isExpired() const;
};

// IPv4 address record
class DnsARecord : public DnsRecord {
public:
    IpAddress address() const;
};

// IPv6 address record  
class DnsAAAARecord : public DnsRecord {
public:
    IpAddress address() const;
};
```

### Usage Example

```cpp
DnsResolver resolver;
resolver.initialize();

std::vector<std::shared_ptr<DnsRecord>> records;
std::string error;

if (resolver.resolve("example.com", DnsRecordType::A, records, error)) {
    for (const auto& record : records) {
        if (auto aRecord = std::dynamic_pointer_cast<DnsARecord>(record)) {
            std::cout << "IP: " << aRecord->address().toString() << std::endl;
        }
    }
}
```

## Cache System

### Overview

The cache system (`cache.h/cpp`) implements HTTP caching with:
- Memory and disk storage backends
- Cache validation (ETag, Last-Modified)
- Size-based eviction
- Configurable quotas

### Cache Architecture

```
┌─────────────────────────────────────────┐
│            Cache Manager                │
│  ┌─────────────┐  ┌─────────────────┐ │
│  │ Memory Cache│  │  Disk Cache     │ │
│  │  (Fast)     │  │  (Persistent)   │ │
│  └─────────────┘  └─────────────────┘ │
└─────────────────────────────────────────┘
```

### Key Classes

#### CacheEntry

```cpp
class CacheEntry {
public:
    // Check entry status
    bool isExpired() const;
    bool canBeValidated() const;
    
    // Get validation headers for conditional requests
    std::map<std::string, std::string> getValidationHeaders() const;
    
    // Update entry with new data
    void update(const std::vector<uint8_t>& data, 
               const std::map<std::string, std::string>& headers);
};
```

#### Cache

```cpp
class Cache {
public:
    // Initialize cache with optional disk storage
    bool initialize(const std::string& cacheDirectory = "");
    
    // Cache operations
    bool get(const std::string& url, CacheEntry& entry);
    bool put(const CacheEntry& entry);
    void clear();
    
    // Configuration
    void setMaxMemoryCacheSize(size_t bytes);
    void setMaxDiskCacheSize(size_t bytes);
};
```

### Cache Headers Support

The cache respects standard HTTP cache headers:
- **Cache-Control**: max-age, no-cache, no-store
- **Expires**: HTTP date for expiration
- **ETag**: Entity tag for validation
- **Last-Modified**: Last modification date

### Usage Example

```cpp
Cache cache;
cache.initialize("./browser_cache");
cache.setMaxMemoryCacheSize(10 * 1024 * 1024); // 10MB

// Check cache before making request
CacheEntry entry;
if (cache.get(url, entry) && !entry.isExpired()) {
    // Use cached data
    return entry.data();
}

// Make request and cache response
HttpResponse response = client.get(url, error);
if (response.statusCode() == 200) {
    CacheEntry newEntry(url, response.body(), response.headers());
    cache.put(newEntry);
}
```

## Resource Loader

### Overview

The resource loader (`resource_loader.h`) coordinates all network operations:
- Asynchronous resource loading
- Request prioritization
- Cache integration
- Progress tracking

### Architecture

```cpp
class ResourceLoader {
    HttpClient m_httpClient;
    DnsResolver m_dnsResolver;
    Cache m_cache;
    
    std::thread m_thread;
    std::queue<std::shared_ptr<ResourceRequest>> m_requestQueue;
};
```

### Resource Loading Flow

1. **Check Cache**: Look for valid cached response
2. **DNS Resolution**: Resolve hostname if needed
3. **HTTP Request**: Fetch resource from server
4. **Cache Storage**: Store successful responses
5. **Callback Notification**: Notify completion

### Usage Example

```cpp
ResourceLoader loader;
loader.initialize("./cache");
loader.start();

// Queue a resource request
auto request = std::make_shared<ResourceRequest>(url, ResourceType::CSS);
request->setCompletionCallback(
    [](const std::vector<uint8_t>& data, 
       const std::map<std::string, std::string>& headers) {
        // Process loaded resource
    });

loader.queueRequest(request);
```

## Platform Integration

### Socket Abstraction

Cross-platform socket handling:
- **Windows**: Winsock2 API
- **Unix/Linux**: BSD sockets
- **macOS**: BSD sockets

### SSL/TLS Support (Planned)

Future SSL/TLS integration points:
- Certificate validation
- Cipher suite selection
- Protocol version negotiation

## Performance Optimizations

### Connection Pooling (Planned)

- Keep-alive connections
- Connection reuse
- Per-host connection limits

### Request Pipelining

- Queue multiple requests
- Prioritize critical resources
- Cancel pending requests

### Cache Strategies

1. **Two-tier caching**: Memory (L1) and disk (L2)
2. **LRU eviction**: Remove least recently used items
3. **Size limits**: Configurable quotas per tier

## Error Handling

### Network Errors

```cpp
enum class NetworkError {
    CONNECTION_FAILED,
    TIMEOUT,
    DNS_RESOLUTION_FAILED,
    SSL_ERROR,
    INVALID_RESPONSE
};
```

### Retry Logic

- Automatic retry for transient failures
- Exponential backoff
- Maximum retry limits

## Security Considerations

### Request Validation

- URL sanitization
- Header injection prevention
- Body size limits

### Response Validation

- Content-Type checking
- Content-Length verification
- Charset detection

## Testing

### Unit Tests

```cpp
TEST(HttpClient, ParseUrl) {
    HttpRequest request("https://example.com:8080/path?query=1");
    std::string protocol, host, path;
    int port;
    
    ASSERT_TRUE(request.parseUrl(protocol, host, path, port));
    EXPECT_EQ(protocol, "https");
    EXPECT_EQ(host, "example.com");
    EXPECT_EQ(port, 8080);
    EXPECT_EQ(path, "/path?query=1");
}
```

### Integration Tests

- Mock server responses
- Network failure simulation
- Cache behavior verification

## Limitations

1. **No HTTP/2**: Only HTTP/1.1 supported
2. **Basic Auth Only**: No OAuth, digest auth
3. **No WebSockets**: Standard HTTP only
4. **Limited Compression**: No gzip/deflate
5. **No Streaming**: Full response buffering

## Future Enhancements

1. **HTTP/2 Support**: Multiplexing, server push
2. **WebSocket Support**: Bidirectional communication
3. **Compression**: gzip, deflate, brotli
4. **Better SSL/TLS**: Full implementation
5. **HTTP/3 QUIC**: Future protocol support