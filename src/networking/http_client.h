#ifndef BROWSER_HTTP_CLIENT_H
#define BROWSER_HTTP_CLIENT_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <cstdint> // For uint8_t

// Forward declare addrinfo struct to avoid redefinition issues
#ifdef _WIN32
struct addrinfo;
#else
struct addrinfo;
#endif

// Platform-specific includes
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define closesocket close
#endif

namespace browser {
namespace networking {

// HTTP request method enum
enum class HttpMethod {
    GET,
    POST,
    HEAD,
    PUT,
    DELETE_,
    OPTIONS
};

// HTTP response class
class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();
    
    // Response status
    int statusCode() const { return m_statusCode; }
    std::string statusText() const { return m_statusText; }
    
    // Response headers
    const std::map<std::string, std::string>& headers() const { return m_headers; }
    bool hasHeader(const std::string& name) const;
    std::string getHeader(const std::string& name) const;
    
    // Response body
    const std::vector<uint8_t>& body() const { return m_body; }
    std::string bodyAsString() const;
    
    // Set response properties
    void setStatusCode(int code) { m_statusCode = code; }
    void setStatusText(const std::string& text) { m_statusText = text; }
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::vector<uint8_t>& body) { m_body = body; }
    void appendToBody(const uint8_t* data, size_t length);
    
    // Parse response from raw data
    bool parseResponse(const std::vector<uint8_t>& data);
    
private:
    int m_statusCode;
    std::string m_statusText;
    std::map<std::string, std::string> m_headers;
    std::vector<uint8_t> m_body;
};

// HTTP request class
class HttpRequest {
public:
    HttpRequest();
    explicit HttpRequest(const std::string& url);
    HttpRequest(HttpMethod method, const std::string& url);
    ~HttpRequest();
    
    // Request properties
    HttpMethod method() const { return m_method; }
    void setMethod(HttpMethod method) { m_method = method; }
    
    std::string url() const { return m_url; }
    void setUrl(const std::string& url) { m_url = url; }
    
    // Request headers
    const std::map<std::string, std::string>& headers() const { return m_headers; }
    bool hasHeader(const std::string& name) const;
    std::string getHeader(const std::string& name) const;
    void setHeader(const std::string& name, const std::string& value);
    
    // Request body
    const std::vector<uint8_t>& body() const { return m_body; }
    void setBody(const std::vector<uint8_t>& body) { m_body = body; }
    void setBody(const std::string& body);
    
    // Parse URL into components
    bool parseUrl(std::string& protocol, std::string& host, 
                 std::string& path, int& port) const;
    
private:
    HttpMethod m_method;
    std::string m_url;
    std::map<std::string, std::string> m_headers;
    std::vector<uint8_t> m_body;
};

// Callback types for asynchronous operations
using HttpResponseCallback = std::function<void(const HttpResponse&, const std::string&)>;

// HTTP client class
class HttpClient {
public:
    HttpClient();
    ~HttpClient();
    
    // Initialize the client
    bool initialize();
    
    // Synchronous request methods
    HttpResponse sendRequest(const HttpRequest& request, std::string& error);
    HttpResponse get(const std::string& url, std::string& error);
    HttpResponse post(const std::string& url, const std::string& body, 
                     const std::string& contentType, std::string& error);
    
    // Asynchronous request methods
    void sendRequestAsync(const HttpRequest& request, HttpResponseCallback callback);
    void getAsync(const std::string& url, HttpResponseCallback callback);
    void postAsync(const std::string& url, const std::string& body,
                  const std::string& contentType, HttpResponseCallback callback);
    
    // Set connection timeout in seconds
    void setTimeout(int seconds) { m_timeoutSeconds = seconds; }
    
    // Set maximum redirects to follow
    void setMaxRedirects(int maxRedirects) { m_maxRedirects = maxRedirects; }
    
    // Process any pending asynchronous requests (for event loop integration)
    void processPendingRequests();
    
private:
    // Internal request processing
    HttpResponse sendRequestInternal(const HttpRequest& request, int redirectCount, std::string& error);
    
    // Platform-specific socket handling
    bool openConnection(const std::string& host, int port, std::string& error);
    bool sendData(const std::vector<uint8_t>& data, std::string& error);
    bool receiveData(std::vector<uint8_t>& data, std::string& error);
    void closeConnection();
    
    // Connection socket
    socket_t m_socket;
    
    // Configuration
    int m_timeoutSeconds;
    int m_maxRedirects;
    
    // Thread pool for async requests (simplified in this implementation)
    struct AsyncRequest {
        HttpRequest request;
        HttpResponseCallback callback;
        bool inProgress;
    };
    std::vector<AsyncRequest> m_pendingRequests;
    
    // Platform-specific SSL handling
    bool m_useSSL;
    void* m_sslContext;  // Opaque pointer to SSL context
};

// Helper function for case-insensitive string comparison
int strcasecmp(const char* s1, const char* s2);

} // namespace networking
} // namespace browser

#endif // BROWSER_HTTP_CLIENT_H