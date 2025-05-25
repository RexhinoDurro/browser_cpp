// src/browser/browser.cpp
#include "browser.h"
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>

namespace browser {

// Add the resolveUrl implementation to the Browser class
std::string Browser::resolveUrl(const std::string& baseUrl, const std::string& relativeUrl) {
    // Check if already absolute
    if (relativeUrl.find("://") != std::string::npos) {
        return relativeUrl;
    }
    
    try {
        // Parse base URL
        size_t schemeEnd = baseUrl.find("://");
        if (schemeEnd == std::string::npos) {
            return relativeUrl; // Invalid base URL
        }
        
        std::string scheme = baseUrl.substr(0, schemeEnd + 3);
        std::string baseWithoutScheme = baseUrl.substr(schemeEnd + 3);
        
        // Handle absolute paths
        if (relativeUrl[0] == '/') {
            size_t pathStart = baseWithoutScheme.find('/');
            if (pathStart == std::string::npos) {
                return scheme + baseWithoutScheme + relativeUrl;
            } else {
                return scheme + baseWithoutScheme.substr(0, pathStart) + relativeUrl;
            }
        } 
        
        // Handle relative paths
        size_t lastSlash = baseUrl.find_last_of('/');
        if (lastSlash == std::string::npos || lastSlash < schemeEnd + 3) {
            return baseUrl + "/" + relativeUrl;
        } else {
            return baseUrl.substr(0, lastSlash + 1) + relativeUrl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error resolving URL: " << e.what() << std::endl;
        return relativeUrl;
    }
}

// Other Browser class implementations would go here...

} // namespace browser