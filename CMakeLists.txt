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

# Create main executable
add_executable(browser_cpp
    src/main.cpp
)

target_link_libraries(browser_cpp
    browser_core
    js_engine
    networking
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

# Enable testing
enable_testing()

# Add tests directory
add_subdirectory(tests)