cmake_minimum_required(VERSION 3.10)
project(browser_cpp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add QuickJS library
add_library(quickjs STATIC IMPORTED)
set_target_properties(quickjs PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/third_party/quickjs/libquickjs.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/third_party/quickjs"
)

# Define your browser source files
set(BROWSER_SOURCES
    src/main.cpp
    src/html/dom_tree.cpp
    src/html/html_parser.cpp
    src/css/css_parser.cpp
    src/css/style_resolver.cpp
    # Add other existing source files
)

# Add JavaScript engine sources
set(JS_SOURCES
    src/javascript/js_engine.cpp
    src/javascript/js_bindings.cpp
)

# Create browser library
add_library(browser_core STATIC
    ${BROWSER_SOURCES}
)

# Create JavaScript engine library
add_library(js_engine STATIC
    ${JS_SOURCES}
)

# Link libraries
target_link_libraries(js_engine
    quickjs
    browser_core
)

# Create main executable
add_executable(browser_cpp
    src/main.cpp
)

target_link_libraries(browser_cpp
    browser_core
    js_engine
    quickjs
    pthread
    m
)