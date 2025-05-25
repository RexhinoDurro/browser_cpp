cmake_minimum_required(VERSION 3.14)

# Set project name and language
project(BrowserEngine
        VERSION 0.1.0
        DESCRIPTION "A browser engine implementation in C++"
        LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define build type if not defined
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Option to enable JavaScript support
option(ENABLE_JAVASCRIPT "Enable JavaScript support" ON)

# Output directory configuration
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# Find packages
find_package(OpenGL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

# Include directories for dependencies
include_directories(
    ${OPENGL_INCLUDE_DIR}
)

# QuickJS library setup
if(ENABLE_JAVASCRIPT)
    set(QUICKJS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/quickjs)
    
    # Check which source files actually exist
    set(QUICKJS_SOURCES "")
    
    # Core QuickJS files
    foreach(file
            quickjs.c
            libregexp.c
            libunicode.c
            cutils.c)
        if(EXISTS ${QUICKJS_DIR}/${file})
            list(APPEND QUICKJS_SOURCES ${QUICKJS_DIR}/${file})
        else()
            message(WARNING "QuickJS source file not found: ${QUICKJS_DIR}/${file}")
        endif()
    endforeach()
    
    # Optional files - only add if they exist
    foreach(file
            libbf.c
            quickjs-libc.c)
        if(EXISTS ${QUICKJS_DIR}/${file})
            list(APPEND QUICKJS_SOURCES ${QUICKJS_DIR}/${file})
        endif()
    endforeach()
    
    # Create QuickJS library if we found the source files
    if(QUICKJS_SOURCES)
        add_library(quickjs STATIC ${QUICKJS_SOURCES})
        
        # Set QuickJS compiler flags
        target_compile_definitions(quickjs PRIVATE CONFIG_VERSION="2021-03-27")
        
        # Enable JavaScript support
        add_definitions(-DJS_ENGINE_ENABLED)
    else()
        message(WARNING "No QuickJS source files found, disabling JavaScript support")
        set(ENABLE_JAVASCRIPT OFF)
        add_definitions(-DJS_ENGINE_DISABLED)
    endif()
else()
    add_definitions(-DJS_ENGINE_DISABLED)
endif()

# NanoVG library setup
set(NANOVG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nanovg)
add_library(nanovg STATIC ${NANOVG_DIR}/src/nanovg.c)
target_include_directories(nanovg PUBLIC ${NANOVG_DIR}/src)
target_link_libraries(nanovg ${OPENGL_LIBRARIES})

# Define source file groups
set(BROWSER_SOURCES
    src/browser/browser.cpp
)

set(HTML_SOURCES
    src/html/dom_tree.cpp
    src/html/html_parser.cpp
)

set(CSS_SOURCES
    src/css/css_parser.cpp
    src/css/style_resolver.cpp
)

set(LAYOUT_SOURCES
    src/layout/box_model.cpp
    src/layout/layout_engine.cpp
)

set(RENDERING_SOURCES
    src/rendering/renderer.cpp
    src/rendering/paint_system.cpp
)

set(JAVASCRIPT_SOURCES
    src/javascript/js_engine.cpp
    src/javascript/js_bindings.cpp
)

set(NETWORKING_SOURCES
    src/networking/http_client.cpp
    src/networking/dns_resolver.cpp
    src/networking/cache.cpp
)

set(SECURITY_SOURCES
    src/security/same_origin.cpp
    src/security/content_security_policy.cpp
    src/security/xss_protection.cpp
    src/security/csrf_protection.cpp
    src/security/cookie_security.cpp
    src/security/certificate_validator.cpp
    src/security/security_manager.cpp
)

set(STORAGE_SOURCES
    src/storage/local_storage.cpp
)

set(UI_SOURCES
    src/ui/window.cpp
    src/ui/controls.cpp
)

# Combine all sources
set(ALL_SOURCES
    ${BROWSER_SOURCES}
    ${HTML_SOURCES}
    ${CSS_SOURCES}
    ${LAYOUT_SOURCES}
    ${RENDERING_SOURCES}
    ${JAVASCRIPT_SOURCES}
    ${NETWORKING_SOURCES}
    ${SECURITY_SOURCES}
    ${STORAGE_SOURCES}
    ${UI_SOURCES}
)

# Create browser engine library
if(ALL_SOURCES)
    add_library(browser_engine STATIC ${ALL_SOURCES})

    # Link browser engine with dependencies
    target_link_libraries(browser_engine
        ${OPENGL_LIBRARIES}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    # Link with QuickJS if JavaScript is enabled
    if(ENABLE_JAVASCRIPT AND TARGET quickjs)
        target_link_libraries(browser_engine quickjs)
    endif()

    # Link with NanoVG
    if(TARGET nanovg)
        target_link_libraries(browser_engine nanovg)
    endif()

    # Find GLFW package for the UI
    find_package(glfw3 CONFIG REQUIRED)

    # Create main executable
    add_executable(browser_app src/main.cpp)
    target_link_libraries(browser_app browser_engine glfw3::glfw)
else()
    message(FATAL_ERROR "No browser engine source files found. Check your file paths.")
endif()

# Display configuration information
message(STATUS "Browser Engine Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  JavaScript support: ${ENABLE_JAVASCRIPT}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")

# Installation rules
install(TARGETS browser_app
        RUNTIME DESTINATION bin)