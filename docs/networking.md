# Browser Networking Implementation

This implementation adds full networking capabilities to the browser engine, including:

1. **HTTP Client** - For making web requests
2. **DNS Resolver** - For domain name resolution
3. **Cache System** - For storing and retrieving resources
4. **Resource Loader** - High-level API for resource loading with caching

## Components

### HTTP Client

The `HttpClient` class provides:
- Support for HTTP/1.1 requests
- Both synchronous and asynchronous API
- Support for different HTTP methods (GET, POST, etc.)
- Header manipulation
- Automatic handling of redirects
- Connection pooling and timeout handling

### DNS Resolver

The `DnsResolver` class provides:
- Caching of DNS records
- Support for A (IPv4) and AAAA (IPv6) records
- Both synchronous and asynchronous API
- Background DNS resolution

### Cache System

The `Cache` class provides:
- In-memory and on-disk caching
- HTTP caching semantics (ETag, Last-Modified)
- Cache validation
- Cache control directive handling
- Size-limited caching with LRU eviction

### Resource Loader

The `ResourceLoader` class provides:
- High-level API for resource loading
- Automatic caching
- Background loading of resources
- Resource prioritization
- Callback-based API for asynchronous loading

## Building

To build the networking components:

```bash
mkdir build
cd build
cmake ..
make
```

This will build the main browser executable and the networking example.

## Using the Networking Components

### Basic HTTP Request

```cpp
#include "networking/http_client.h"

// Initialize HTTP client
browser::networking::HttpClient httpClient;
httpClient.initialize();

// Make a synchronous request
std::string error;
browser::networking::HttpResponse response = httpClient.get("https://example.com", error);

if (response.statusCode() == 200) {
    std::string body = response.bodyAsString();
    std::cout << "Response: " << body << std::endl;
} else {
    std::cerr << "Error: " << error << std::endl;
}
```

### Asynchronous HTTP Request

```cpp
httpClient.getAsync("https://example.com", [](const HttpResponse& response, const std::string& error) {
    if (error.empty() && response.statusCode() == 200) {
        std::cout << "Response: " << response.bodyAsString() << std::endl;
    } else {
        std::cerr << "Error: " << error << std::endl;
    }
});

// Process pending requests
httpClient.processPendingRequests();
```

### Using the Resource Loader

```cpp
#include "networking/resource_loader.h"

// Initialize resource loader
browser::networking::ResourceLoader resourceLoader;
resourceLoader.initialize("./browser_cache");
resourceLoader.start();

// Create a resource request
auto request = std::make_shared<browser::networking::ResourceRequest>(
    "https://example.com",
    browser::networking::ResourceType::HTML
);

// Set completion callback
request->setCompletionCallback([](const std::vector<uint8_t>& data, 
                               const std::map<std::string, std::string>& headers) {
    std::string body(data.begin(), data.end());
    std::cout << "Loaded resource: " << body.substr(0, 100) << "..." << std::endl;
});

// Queue the request
resourceLoader.queueRequest(request);

// Wait for completion or timeout
while (!request->isComplete()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// Stop the resource loader when done
resourceLoader.stop();
```

## Error Handling

All networking components include proper error handling. For synchronous operations, errors are returned through an error string parameter. For asynchronous operations, errors are provided through callbacks.

## Thread Safety

The networking components are designed to be thread-safe:
- `HttpClient` can be used from multiple threads
- `DnsResolver` can be used from multiple threads
- `Cache` can be used from multiple threads
- `ResourceLoader` manages its own thread for asynchronous loading

## Future Improvements

1. **HTTP/2 Support** - Add support for HTTP/2 protocol
2. **WebSocket Support** - Add WebSocket client implementation
3. **HTTPS Improvements** - Better handling of SSL/TLS certificates
4. **Request Prioritization** - More sophisticated prioritization of resource requests
5. **Service Worker API** - Implement a Service Worker-like API for offline caching
6. **Network Inspector** - Add a network inspector for debugging
7. **Bandwidth Throttling** - Add support for bandwidth throttling for testing

## Example

See `examples/networking_example.cpp` for a complete example of using the networking components.