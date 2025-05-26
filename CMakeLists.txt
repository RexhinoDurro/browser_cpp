cmake_minimum_required(VERSION 3.10)
project(SimpleBrowser VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
endif()

# Platform-specific settings
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7+
    set(PLATFORM_LIBS ws2_32 winmm gdi32 user32)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(CORE_VIDEO CoreVideo REQUIRED)
    set(PLATFORM_LIBS ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${CORE_VIDEO})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ObjC++")
else()
    # Linux/Unix
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
    set(PLATFORM_LIBS ${X11_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()

# Source files organized by component
set(HTML_SOURCES
    src/html/html_parser.cpp
    src/html/html_parser.h
    src/html/dom_tree.h
    src/html/dom_tree.cpp
)

set(CSS_SOURCES
    src/css/style_resolver.cpp
    src/css/style_resolver.h
    src/css/css_parser.cpp
    src/css/css_parser.h
)

set(JS_SOURCES
    src/custom_js/js_engine.cpp
    src/custom_js/js_engine.h
    src/custom_js/js_lexer.cpp
    src/custom_js/js_lexer.h
    src/custom_js/js_parser.cpp
    src/custom_js/js_parser.h
    src/custom_js/js_interpreter.cpp
    src/custom_js/js_interpreter.h
    src/custom_js/js_value.cpp
    src/custom_js/js_value.h
)

set(LAYOUT_SOURCES
    src/layout/layout_engine.cpp
    src/layout/layout_engine.h
    src/layout/box_model.h
    src/layout/box_model.cpp
)

set(RENDERING_SOURCES
    
    src/rendering/renderer.h
    src/rendering/paint_system.cpp
    src/rendering/paint_system.h
    src/rendering/custom_renderer.cpp
    src/rendering/custom_renderer.h
    src/rendering/modified_renderer.cpp
    src/rendering/renderer_integration.h
    src/rendering/paint_system.h
    src/rendering/paint_system.cpp
    src/rendering/custom_render_target.cpp
    src/rendering/custom_render_target.h
)

set(NETWORKING_SOURCES
    src/networking/resource_loader.h
    src/networking/http_client.cpp
    src/networking/http_client.h
    src/networking/dns_resolver.cpp
    src/networking/dns_resolver.h
    src/networking/cache.cpp
    src/networking/cache.h
)

set(SECURITY_SOURCES
    src/security/security_manager.cpp
    src/security/security_manager.h
    src/security/same_origin.cpp
    src/security/same_origin.h
    src/security/content_security_policy.cpp
    src/security/content_security_policy.h
    src/security/xss_protection.cpp
    src/security/xss_protection.h
    src/security/csrf_protection.cpp
    src/security/csrf_protection.h
    src/security/cookie_security.cpp
    src/security/cookie_security.h
    src/security/certificate_validator.cpp
    src/security/certificate_validator.h
)

set(STORAGE_SOURCES
    src/storage/local_storage.cpp
    src/storage/local_storage.h
)

# Platform-independent UI sources
set(UI_SOURCES
    src/ui/window.cpp
    src/ui/window.h
    src/ui/browser_window.cpp
    src/ui/browser_window.h
    src/ui/custom_controls.cpp
    src/ui/custom_controls.h
    src/ui/custom_canvas.cpp
    src/ui/custom_canvas.h
)

# Platform-specific UI sources
if(WIN32)
    list(APPEND UI_SOURCES
        src/ui/window_win32.cpp
        src/ui/window_win32.h
    )
elseif(APPLE)
    list(APPEND UI_SOURCES
        src/ui/window_macos.mm
        src/ui/window_macos.h
    )
else()
    list(APPEND UI_SOURCES
        src/ui/window_x11.cpp
        src/ui/window_x11.h
    )
endif()

set(BROWSER_SOURCES
    src/browser/browser.cpp
    src/browser/browser.h
)

# Main executable source
set(MAIN_SOURCE
    src/main.cpp
)

# Create source groups for IDE organization
source_group("HTML" FILES ${HTML_SOURCES})
source_group("CSS" FILES ${CSS_SOURCES})
source_group("JavaScript" FILES ${JS_SOURCES})
source_group("Layout" FILES ${LAYOUT_SOURCES})
source_group("Rendering" FILES ${RENDERING_SOURCES})
source_group("Networking" FILES ${NETWORKING_SOURCES})
source_group("Security" FILES ${SECURITY_SOURCES})
source_group("Storage" FILES ${STORAGE_SOURCES})
source_group("UI" FILES ${UI_SOURCES})
source_group("Browser" FILES ${BROWSER_SOURCES})
source_group("Main" FILES ${MAIN_SOURCE})

# Create the browser library
add_library(browser_lib STATIC
    ${HTML_SOURCES}
    ${CSS_SOURCES}
    ${JS_SOURCES}
    ${LAYOUT_SOURCES}
    ${RENDERING_SOURCES}
    ${NETWORKING_SOURCES}
    ${SECURITY_SOURCES}
    ${STORAGE_SOURCES}
    ${UI_SOURCES}
    ${BROWSER_SOURCES}
)

# Include directories
target_include_directories(browser_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Platform-specific include directories
if(NOT WIN32 AND NOT APPLE)
    target_include_directories(browser_lib PUBLIC ${X11_INCLUDE_DIR})
endif()

# Create the main executable
add_executable(SimpleBrowser ${MAIN_SOURCE})

# Link libraries
target_link_libraries(SimpleBrowser
    browser_lib
    ${PLATFORM_LIBS}
)

# Copy resources to build directory
set(RESOURCE_FILES
    resources/default.html
    resources/default.css
    resources/icon.png
)

foreach(RESOURCE ${RESOURCE_FILES})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE}
                   ${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE} COPYONLY)
endforeach()

# Installation rules
install(TARGETS SimpleBrowser
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY resources/
    DESTINATION share/simplebrowser/resources
)

# Create a desktop file for Linux
if(UNIX AND NOT APPLE)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/simplebrowser.desktop.in
        ${CMAKE_CURRENT_BINARY_DIR}/simplebrowser.desktop
        @ONLY
    )
    
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/simplebrowser.desktop
        DESTINATION share/applications
    )
endif()

# Testing support (optional)
option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation (optional)
option(BUILD_DOCS "Build documentation" OFF)

if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
            ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            @ONLY
        )
        
        add_custom_target(docs
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

# Package configuration
set(CPACK_PACKAGE_NAME "SimpleBrowser")
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A simple web browser built from scratch")
set(CPACK_PACKAGE_VENDOR "SimpleBrowser Team")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "DMG")
else()
    set(CPACK_GENERATOR "DEB;RPM")
endif()

include(CPack)