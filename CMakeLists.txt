cmake_minimum_required(VERSION 3.15)
project(SimpleBrowser VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX- /MP)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
endif()

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
else()
    set(PLATFORM_LINUX TRUE)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Browser Core Library
set(BROWSER_CORE_SOURCES
    src/browser/browser.cpp
    src/browser/browser.h
)

add_library(browser_core STATIC ${BROWSER_CORE_SOURCES})

# CSS Library
set(CSS_SOURCES
    src/css/css_parser.cpp
    src/css/css_parser.h
    src/css/style_resolver.cpp
    src/css/style_resolver.h
)

add_library(css STATIC ${CSS_SOURCES})

# Custom JavaScript Engine Library
set(CUSTOM_JS_SOURCES
    src/custom_js/js_engine.cpp
    src/custom_js/js_engine.h
    src/custom_js/js_interpreter.cpp
    src/custom_js/js_interpreter.h
    src/custom_js/js_lexer.cpp
    src/custom_js/js_lexer.h
    src/custom_js/js_parser.cpp
    src/custom_js/js_parser.h
    src/custom_js/js_value.cpp
    src/custom_js/js_value.h
)

add_library(custom_js STATIC ${CUSTOM_JS_SOURCES})

# HTML Library
set(HTML_SOURCES
    src/html/dom_tree.cpp
    src/html/dom_tree.h
    src/html/html_parser.cpp
    src/html/html_parser.h
)

add_library(html STATIC ${HTML_SOURCES})

# Layout Engine Library
set(LAYOUT_SOURCES
    src/layout/box_model.cpp
    src/layout/box_model.h
    src/layout/layout_engine.cpp
    src/layout/layout_engine.h
)

add_library(layout STATIC ${LAYOUT_SOURCES})

# Networking Library
set(NETWORKING_SOURCES
    src/networking/cache.cpp
    src/networking/cache.h
    src/networking/dns_resolver.cpp
    src/networking/dns_resolver.h
    src/networking/http_client.cpp
    src/networking/http_client.h
    src/networking/resource_loader.cpp
    src/networking/resource_loader.h
)

add_library(networking STATIC ${NETWORKING_SOURCES})

# Rendering Library
set(RENDERING_SOURCES
    src/rendering/custom_render_target.cpp
    src/rendering/custom_render_target.h
    src/rendering/custom_renderer.cpp
    src/rendering/custom_renderer.h
    src/rendering/renderer.cpp
    src/rendering/renderer.h
    src/rendering/paint_system.cpp
    src/rendering/paint_system.h
    src/rendering/renderer_integration.h
)

add_library(rendering STATIC ${RENDERING_SOURCES})

# Security Library
set(SECURITY_SOURCES
    src/security/certificate_validator.cpp
    src/security/certificate_validator.h
    src/security/content_security_policy.cpp
    src/security/content_security_policy.h
    src/security/cookie_security.cpp
    src/security/cookie_security.h
    src/security/csrf_protection.cpp
    src/security/csrf_protection.h
    src/security/same_origin.cpp
    src/security/same_origin.h
    src/security/security_manager.cpp
    src/security/security_manager.h
    src/security/xss_protection.cpp
    src/security/xss_protection.h
)

add_library(security STATIC ${SECURITY_SOURCES})

# Storage Library
set(STORAGE_SOURCES
    src/storage/local_storage.cpp
    src/storage/local_storage.h
)

add_library(storage STATIC ${STORAGE_SOURCES})

# UI Library
set(UI_COMMON_SOURCES
    src/ui/browser_window.cpp
    src/ui/browser_window.h
    src/ui/custom_canvas.cpp
    src/ui/custom_canvas.h
    src/ui/custom_controls.cpp
    src/ui/custom_controls.h
    src/ui/window.cpp
    src/ui/window.h
)

# Platform-specific UI sources
if(PLATFORM_WINDOWS)
    list(APPEND UI_COMMON_SOURCES
        src/ui/window_win32.cpp
        src/ui/window_win32.h
    )
elseif(PLATFORM_MACOS)
    list(APPEND UI_COMMON_SOURCES
        src/ui/window_macos.mm
        src/ui/window_macos.h
    )
    # Enable Objective-C++ for macOS
    set_source_files_properties(src/ui/window_macos.mm PROPERTIES COMPILE_FLAGS "-x objective-c++")
else() # Linux/Unix
    list(APPEND UI_COMMON_SOURCES
        src/ui/window_unix.cpp
        src/ui/window_unix.h
    )
endif()

add_library(ui STATIC ${UI_COMMON_SOURCES})

# Main executable
add_executable(SimpleBrowser src/main.cpp)

# Link libraries
target_link_libraries(SimpleBrowser
    browser_core
    css
    custom_js
    html
    layout
    networking
    rendering
    security
    storage
    ui
)

# Platform-specific linking
if(PLATFORM_WINDOWS)
    target_link_libraries(SimpleBrowser
        user32
        gdi32
        shell32
        ole32
        oleaut32
        ws2_32
        wininet
        crypt32
    )
elseif(PLATFORM_MACOS)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
    find_library(COREGRAPHICS_LIBRARY CoreGraphics REQUIRED)
    find_library(SECURITY_LIBRARY Security REQUIRED)
    
    target_link_libraries(SimpleBrowser
        ${COCOA_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${COREGRAPHICS_LIBRARY}
        ${SECURITY_LIBRARY}
    )
else() # Linux/Unix
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
    
    target_link_libraries(SimpleBrowser
        ${X11_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        dl
        crypto
        ssl
        curl
    )
    
    target_include_directories(SimpleBrowser PRIVATE ${X11_INCLUDE_DIR})
endif()

# Set library dependencies (order matters)
target_link_libraries(browser_core
    html
    css
    custom_js
    layout
    rendering
    networking
    security
    storage
)

target_link_libraries(layout
    html
    css
)

target_link_libraries(rendering
    layout
    css
)

# Installation rules
install(TARGETS SimpleBrowser DESTINATION bin)

# Create a simple main.cpp if it doesn't exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp" "
#include \"ui/browser_window.h\"
#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
    try {
        // Create window configuration
        browser::ui::WindowConfig config;
        config.title = \"Simple Browser\";
        config.width = 1024;
        config.height = 768;
        
        // Create browser window
        auto window = std::make_unique<browser::ui::BrowserWindow>(config);
        
        // Initialize the window
        if (!window->initialize()) {
            std::cerr << \"Failed to initialize browser window\" << std::endl;
            return -1;
        }
        
        // Show the window
        window->show();
        
        // Load default page or URL from command line
        if (argc > 1) {
            window->loadUrl(argv[1]);
        } else {
            window->loadUrl(\"about:home\");
        }
        
        // Run the event loop
        window->runEventLoop();
        
    } catch (const std::exception& e) {
        std::cerr << \"Error: \" << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
")
endif()

# Output build information
message(STATUS \"Build type: ${CMAKE_BUILD_TYPE}\")
message(STATUS \"C++ standard: ${CMAKE_CXX_STANDARD}\")
message(STATUS \"Platform: ${CMAKE_SYSTEM_NAME}\")

# Add custom target for cleaning build artifacts
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake_clean.cmake
)