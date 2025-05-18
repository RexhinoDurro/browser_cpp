# Add storage sources for the browser
set(STORAGE_SOURCES
    src/storage/local_storage.cpp
)

# Update main CMakeLists.txt to include the storage components
cmake_minimum_required(VERSION 3.10)
project(browser_cpp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include filesystem library for C++17
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    link_libraries(stdc++fs)
endif()

# Add QuickJS library
add_library(quickjs STATIC IMPORTED)
set_target_properties(quickjs PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/third_party/quickjs/libquickjs.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/third_party/quickjs"
)

# Define your browser source files
set(BROWSER_SOURCES
    src/html/dom_tree.cpp
    src/html/html_parser.cpp
    src/css/css_parser.cpp
    src/css/style_resolver.cpp
    src/layout/box_model.cpp
    src/layout/layout_engine.cpp
    src/rendering/renderer.cpp
    src/rendering/paint_system.cpp
)

# Add JavaScript engine sources
set(JS_SOURCES
    src/javascript/js_engine.cpp
    src/javascript/js_bindings.cpp
)

# Add networking sources
set(NETWORKING_SOURCES
    src/networking/http_client.cpp
    src/networking/dns_resolver.cpp
    src/networking/cache.cpp
    src/networking/resource_loader.h
)

# Add security sources
set(SECURITY_SOURCES
    src/security/same_origin.cpp
    src/security/content_security_policy.cpp
    src/security/certificate_validator.cpp
    src/security/xss_protection.cpp
    src/security/csrf_protection.cpp
    src/security/cookie_security.cpp
    src/security/security_manager.cpp
)

# Add storage sources
set(STORAGE_SOURCES
    src/storage/local_storage.cpp
)

# Add browser core
set(BROWSER_CORE_SOURCES
    src/browser/browser.cpp
)

# Create browser library
add_library(browser_core STATIC
    ${BROWSER_SOURCES}
)

# Create JavaScript engine library
add_library(js_engine STATIC
    ${JS_SOURCES}
)

# Create networking library
add_library(networking STATIC
    ${NETWORKING_SOURCES}
)

# Create security library
add_library(security STATIC
    ${SECURITY_SOURCES}
)

# Create storage library
add_library(storage STATIC
    ${STORAGE_SOURCES}
)

# Create browser main library
add_library(browser_main STATIC
    ${BROWSER_CORE_SOURCES}
)

# Platform-specific libraries
if(WIN32)
    set(PLATFORM_LIBS ws2_32)
else()
    set(PLATFORM_LIBS)
endif()

# Link libraries
target_link_libraries(js_engine
    quickjs
    browser_core
)

target_link_libraries(networking
    ${PLATFORM_LIBS}
)

target_link_libraries(security
    networking
)

target_link_libraries(storage
    security
)

target_link_libraries(browser_main
    browser_core
    js_engine
    networking
    security
    storage
)

# Create main executable
add_executable(browser_cpp
    src/main.cpp
)

target_link_libraries(browser_cpp
    browser_main
    browser_core
    js_engine
    networking
    security
    storage
    quickjs
    pthread
    m
    ${PLATFORM_LIBS}
)

# Create networking example executable
add_executable(networking_example
    examples/networking_example.cpp
)

target_link_libraries(networking_example
    networking
    ${PLATFORM_LIBS}
)

# Create security example executable
add_executable(security_example
    examples/security_example.cpp
)

target_link_libraries(security_example
    security
    networking
    ${PLATFORM_LIBS}
)

# Create local storage example executable
add_executable(local_storage_example
    examples/local_storage_example.cpp
)

target_link_libraries(local_storage_example
    storage
    security
    ${PLATFORM_LIBS}
)

# Enable testing
enable_testing()
enable_testing()

# Add tests directory
add_subdirectory(tests)

# Add the storage tests
add_executable(local_storage_test
    tests/unit/local_storage_test.cpp
)

target_link_libraries(local_storage_test
    storage
    security
    gtest
    gtest_main
    ${PLATFORM_LIBS}
)

add_test(NAME LocalStorageTest COMMAND local_storage_test)