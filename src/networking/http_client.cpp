#include "http_client.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cctype>

namespace browser {
namespace networking {

//-----------------------------------------------------------------------------
// Helper Functions
//-----------------------------------------------------------------------------

// Case-insensitive string comparison
int strcasecmp(const char* s1, const char* s2) {
    #ifdef _WIN32
    return _stricmp(s1, s2);
    #else
    return ::strcasecmp(s1, s2);
    #endif
}

//-----------------------------------------------------------------------------
// HttpResponse Implementation
//-----------------------------------------------------------------------------

HttpResponse::HttpResponse()
    : m_statusCode(0)
{
}

HttpResponse::~HttpResponse() {
}

bool HttpResponse::hasHeader(const std::string& name) const {
    // Case-insensitive header name comparison
    for (const auto& header : m_headers) {
        if (strcasecmp(header.first.c_str(), name.c_str()) == 0) {
            return true;
        }
    }
    return false;
}

std::string HttpResponse::getHeader(const std::string& name) const {
    // Case-insensitive header name lookup
    for (const auto& header : m_headers) {
        if (strcasecmp(header.first.c_str(), name.c_str()) == 0) {
            return header.second;
        }
    }
    return "";
}

std::string HttpResponse::bodyAsString() const {
    if (m_body.empty()) {
        return "";
    }
    
    return std::string(m_body.begin(), m_body.end());
}

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
    // Case-insensitive header name update
    for (auto& header : m_headers) {
        if (strcasecmp(header.first.c_str(), name.c_str()) == 0) {
            header.second = value;
            return;
        }
    }
    
    // Add new header if not found
    m_headers[name] = value;
}

void HttpResponse::appendToBody(const uint8_t* data, size_t length) {
    if (!data || length == 0) {
        return;
    }
    
    m_body.insert(m_body.end(), data, data + length);
}

bool HttpResponse::parseResponse(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return false;
    }
    
    // Convert data to string for easier parsing
    std::string responseStr(data.begin(), data.end());
    
    // Find the separator between headers and body
    size_t headerEnd = responseStr.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return false;
    }
    
    // Extract headers section
    std::string headersSection = responseStr.substr(0, headerEnd);
    
    // Extract body section
    std::vector<uint8_t> bodyData(data.begin() + headerEnd + 4, data.end());
    m_body = bodyData;
    
    // Parse status line and headers
    std::istringstream headersStream(headersSection);
    std::string line;
    
    // Parse status line
    if (!std::getline(headersStream, line)) {
        return false;
    }
    
    // Remove carriage return if present
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    
    // Parse HTTP version and status
    size_t httpPos = line.find("HTTP/");
    if (httpPos != 0) {
        return false;
    }
    
    size_t statusPos = line.find(' ');
    if (statusPos == std::string::npos) {
        return false;
    }
    
    size_t textPos = line.find(' ', statusPos + 1);
    if (textPos == std::string::npos) {
        return false;
    }
    
    try {
        m_statusCode = std::stoi(line.substr(statusPos + 1, textPos - statusPos - 1));
        m_statusText = line.substr(textPos + 1);
    }
    catch (const std::exception&) {
        return false;
    }
    
    // Parse headers
    while (std::getline(headersStream, line)) {
        // Remove carriage return if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // Empty line marks the end of headers
        if (line.empty()) {
            break;
        }
        
        // Parse header (name: value)
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string name = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim leading/trailing whitespace
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            setHeader(name, value);
        }
    }
    
    // Handle specific headers
    if (hasHeader("Content-Length")) {
        try {
            size_t contentLength = std::stoul(getHeader("Content-Length"));
            if (m_body.size() > contentLength) {
                m_body.resize(contentLength);
            }
        }
        catch (const std::exception&) {
            // Ignore invalid content length
        }
    }
    
    // Handle chunked transfer encoding
    if (getHeader("Transfer-Encoding") == "chunked") {
        std::vector<uint8_t> decodedBody;
        size_t pos = 0;
        
        while (pos < m_body.size()) {
            // Find chunk size line end
            size_t lineEnd = pos;
            while (lineEnd < m_body.size() && 
                   !(m_body[lineEnd] == '\r' && lineEnd + 1 < m_body.size() && m_body[lineEnd + 1] == '\n')) {
                lineEnd++;
            }
            
            if (lineEnd >= m_body.size()) {
                break;
            }
            
            // Parse chunk size (hex)
            std::string sizeHex(m_body.begin() + pos, m_body.begin() + lineEnd);
            size_t chunkSize = std::stoul(sizeHex, nullptr, 16);
            
            // End of chunks
            if (chunkSize == 0) {
                break;
            }
            
            // Move past the chunk size line
            pos = lineEnd + 2;
            
            // Copy chunk data
            if (pos + chunkSize <= m_body.size()) {
                decodedBody.insert(decodedBody.end(), 
                                  m_body.begin() + pos, 
                                  m_body.begin() + pos + chunkSize);
                
                // Move past the chunk data and its trailing CRLF
                pos += chunkSize + 2;
            } else {
                break;
            }
        }
        
        m_body = decodedBody;
    }
    
    return true;
}

//-----------------------------------------------------------------------------
// HttpRequest Implementation
//-----------------------------------------------------------------------------

HttpRequest::HttpRequest()
    : m_method(HttpMethod::GET)
{
}

HttpRequest::HttpRequest(const std::string& url)
    : m_method(HttpMethod::GET)
    , m_url(url)
{
}

HttpRequest::HttpRequest(HttpMethod method, const std::string& url)
    : m_method(method)
    , m_url(url)
{
}

HttpRequest::~HttpRequest() {
}

bool HttpRequest::hasHeader(const std::string& name) const {
    // Case-insensitive header name comparison
    for (const auto& header : m_headers) {
        if (strcasecmp(header.first.c_str(), name.c_str()) == 0) {
            return true;
        }
    }
    return false;
}

std::string HttpRequest::getHeader(const std::string& name) const {
    // Case-insensitive header name lookup
    for (const auto& header : m_headers) {
        if (strcasecmp(header.first.c_str(), name.c_str()) == 0) {
            return header.second;
        }
    }
    return "";
}

void HttpRequest::setHeader(const std::string& name, const std::string& value) {
    // Case-insensitive header name update
    for (auto& header : m_headers) {
        if (strcasecmp(header.first.c_str(), name.c_str()) == 0) {
            header.second = value;
            return;
        }
    }
    
    // Add new header if not found
    m_headers[name] = value;
}

void HttpRequest::setBody(const std::string& body) {
    m_body.assign(body.begin(), body.end());
}

bool HttpRequest::parseUrl(std::string& protocol, std::string& host, 
                         std::string& path, int& port) const {
    if (m_url.empty()) {
        return false;
    }
    
    // Find protocol (http:// or https://)
    size_t protocolEnd = m_url.find("://");
    if (protocolEnd == std::string::npos) {
        return false;
    }
    
    protocol = m_url.substr(0, protocolEnd);
    
    // Convert protocol to lowercase
    std::transform(protocol.begin(), protocol.end(), protocol.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    
    // Validate protocol
    if (protocol != "http" && protocol != "https") {
        return false;
    }
    
    // Set default port
    port = (protocol == "https") ? 443 : 80;
    
    // Parse host and path
    size_t hostStart = protocolEnd + 3;
    size_t pathStart = m_url.find('/', hostStart);
    
    if (pathStart == std::string::npos) {
        // No path specified, use root path
        host = m_url.substr(hostStart);
        path = "/";
    } else {
        host = m_url.substr(hostStart, pathStart - hostStart);
        path = m_url.substr(pathStart);
    }
    
    // Check for port in host
    size_t portPos = host.find(':');
    if (portPos != std::string::npos) {
        try {
            port = std::stoi(host.substr(portPos + 1));
            host = host.substr(0, portPos);
        }
        catch (const std::exception&) {
            return false;
        }
    }
    
    return true;
}

//-----------------------------------------------------------------------------
// HttpClient Implementation
//-----------------------------------------------------------------------------

HttpClient::HttpClient()
    : m_socket(INVALID_SOCKET)
    , m_timeoutSeconds(30)
    , m_maxRedirects(5)
    , m_useSSL(false)
    , m_sslContext(nullptr)
{
}

HttpClient::~HttpClient() {
    closeConnection();
}

bool HttpClient::initialize() {
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }
#endif
    
    return true;
}

HttpResponse HttpClient::sendRequest(const HttpRequest& request, std::string& error) {
    return sendRequestInternal(request, 0, error);
}

HttpResponse HttpClient::get(const std::string& url, std::string& error) {
    HttpRequest request(HttpMethod::GET, url);
    return sendRequest(request, error);
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body, 
                           const std::string& contentType, std::string& error) {
    HttpRequest request(HttpMethod::POST, url);
    request.setBody(body);
    request.setHeader("Content-Type", contentType);
    request.setHeader("Content-Length", std::to_string(body.length()));
    return sendRequest(request, error);
}

void HttpClient::sendRequestAsync(const HttpRequest& request, HttpResponseCallback callback) {
    // Add to pending requests queue
    AsyncRequest asyncRequest;
    asyncRequest.request = request;
    asyncRequest.callback = callback;
    asyncRequest.inProgress = false;
    
    m_pendingRequests.push_back(asyncRequest);
}

void HttpClient::getAsync(const std::string& url, HttpResponseCallback callback) {
    HttpRequest request(HttpMethod::GET, url);
    sendRequestAsync(request, callback);
}

void HttpClient::postAsync(const std::string& url, const std::string& body,
                        const std::string& contentType, HttpResponseCallback callback) {
    HttpRequest request(HttpMethod::POST, url);
    request.setBody(body);
    request.setHeader("Content-Type", contentType);
    request.setHeader("Content-Length", std::to_string(body.length()));
    sendRequestAsync(request, callback);
}

void HttpClient::processPendingRequests() {
    // Process one pending request
    if (m_pendingRequests.empty()) {
        return;
    }
    
    AsyncRequest& asyncRequest = m_pendingRequests.front();
    if (!asyncRequest.inProgress) {
        asyncRequest.inProgress = true;
        
        // Process the request
        std::string error;
        HttpResponse response = sendRequest(asyncRequest.request, error);
        
        // Call the callback
        asyncRequest.callback(response, error);
        
        // Remove the request
        m_pendingRequests.erase(m_pendingRequests.begin());
    }
}

HttpResponse HttpClient::sendRequestInternal(const HttpRequest& request, int redirectCount, std::string& error) {
    HttpResponse response;
    
    // Check for too many redirects
    if (redirectCount > m_maxRedirects) {
        error = "Too many redirects";
        return response;
    }
    
    // Parse URL
    std::string protocol, host, path;
    int port;
    
    if (!request.parseUrl(protocol, host, path, port)) {
        error = "Invalid URL: " + request.url();
        return response;
    }
    
    // Set SSL flag
    m_useSSL = (protocol == "https");
    
    // Open connection
    if (!openConnection(host, port, error)) {
        return response;
    }
    
    // Build request
    std::ostringstream requestStream;
    
    // Request line
    switch (request.method()) {
        case HttpMethod::GET:
            requestStream << "GET ";
            break;
        case HttpMethod::POST:
            requestStream << "POST ";
            break;
        case HttpMethod::HEAD:
            requestStream << "HEAD ";
            break;
        case HttpMethod::PUT:
            requestStream << "PUT ";
            break;
        case HttpMethod::DELETE_:
            requestStream << "DELETE ";
            break;
        case HttpMethod::OPTIONS:
            requestStream << "OPTIONS ";
            break;
    }
    
    requestStream << path << " HTTP/1.1\r\n";
    
    // Host header is required for HTTP/1.1
    if (!request.hasHeader("Host")) {
        requestStream << "Host: " << host << "\r\n";
    }
    
    // Add headers
    for (const auto& header : request.headers()) {
        requestStream << header.first << ": " << header.second << "\r\n";
    }
    
    // Add Content-Length for request with body
    if (!request.body().empty() && !request.hasHeader("Content-Length")) {
        requestStream << "Content-Length: " << request.body().size() << "\r\n";
    }
    
    // End of headers
    requestStream << "\r\n";
    
    // Request as string
    std::string requestString = requestStream.str();
    
    // Convert to bytes
    std::vector<uint8_t> requestData(requestString.begin(), requestString.end());
    
    // Add body
    if (!request.body().empty()) {
        requestData.insert(requestData.end(), request.body().begin(), request.body().end());
    }
    
    // Send request
    if (!sendData(requestData, error)) {
        closeConnection();
        return response;
    }
    
    // Receive response
    std::vector<uint8_t> responseData;
    if (!receiveData(responseData, error)) {
        closeConnection();
        return response;
    }
    
    // Close connection
    closeConnection();
    
    // Parse response
    if (!response.parseResponse(responseData)) {
        error = "Failed to parse response";
        return response;
    }
    
    // Handle redirects
    int statusCode = response.statusCode();
    if ((statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 307 || statusCode == 308) && 
        response.hasHeader("Location")) {
        
        // Get redirect URL
        std::string location = response.getHeader("Location");
        
        // Create new request
        HttpRequest redirectRequest = request;
        
        // Set new URL
        if (location.find("://") != std::string::npos) {
            // Absolute URL
            redirectRequest.setUrl(location);
        } else if (!location.empty() && location[0] == '/') {
            // Absolute path
            redirectRequest.setUrl(protocol + "://" + host + location);
        } else {
            // Relative path
            // Extract the directory from the current path
            size_t lastSlash = path.find_last_of('/');
            std::string directory = (lastSlash != std::string::npos) ? path.substr(0, lastSlash + 1) : "/";
            redirectRequest.setUrl(protocol + "://" + host + directory + location);
        }
        
        // For 303 See Other, change method to GET
        if (statusCode == 303) {
            redirectRequest.setMethod(HttpMethod::GET);
            redirectRequest.setBody(std::vector<uint8_t>());
        }
        
        // Follow redirect
        return sendRequestInternal(redirectRequest, redirectCount + 1, error);
    }
    
    return response;
}

bool HttpClient::openConnection(const std::string& host, int port, std::string& error) {
    // Close any existing connection
    closeConnection();
    
    // Create socket
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        error = "Failed to create socket";
        return false;
    }
    
    // Set timeout
    #ifdef _WIN32
    DWORD timeout = m_timeoutSeconds * 1000;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    #else
    struct timeval tv;
    tv.tv_sec = m_timeoutSeconds;
    tv.tv_usec = 0;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
    #endif
    
    // Resolve host name
    struct addrinfo hints = {0};
    struct addrinfo* result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    int addrResult = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);
    if (addrResult != 0) {
        error = "Failed to resolve host: " + host;
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }
    
    // Connect to server
    int connectResult = connect(m_socket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);
    
    if (connectResult == SOCKET_ERROR) {
        error = "Failed to connect to host: " + host;
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }
    
    // TODO: For HTTPS, initialize SSL
    
    return true;
}

bool HttpClient::sendData(const std::vector<uint8_t>& data, std::string& error) {
    if (m_socket == INVALID_SOCKET) {
        error = "Socket not connected";
        return false;
    }
    
    // TODO: For HTTPS, use SSL_write
    
    size_t totalSent = 0;
    while (totalSent < data.size()) {
        int bytesSent = send(m_socket, (const char*)&data[totalSent], (int)(data.size() - totalSent), 0);
        
        if (bytesSent == SOCKET_ERROR) {
            error = "Failed to send data";
            return false;
        }
        
        totalSent += bytesSent;
    }
    
    return true;
}

bool HttpClient::receiveData(std::vector<uint8_t>& data, std::string& error) {
    if (m_socket == INVALID_SOCKET) {
        error = "Socket not connected";
        return false;
    }
    
    // TODO: For HTTPS, use SSL_read
    
    const size_t bufferSize = 8192;
    char buffer[bufferSize];
    
    while (true) {
        int bytesRead = recv(m_socket, buffer, bufferSize, 0);
        
        if (bytesRead == SOCKET_ERROR) {
            error = "Failed to receive data";
            return false;
        }
        
        if (bytesRead == 0) {
            // Connection closed
            break;
        }
        
        data.insert(data.end(), buffer, buffer + bytesRead);
    }
    
    return true;
}

void HttpClient::closeConnection() {
    // TODO: For HTTPS, clean up SSL
    
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

} // namespace networking
} // namespace browser