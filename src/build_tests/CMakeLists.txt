# CMakeLists.txt for UI/Rendering Tests
cmake_minimum_required(VERSION 3.10)
project(BrowserTests VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type to Debug for tests
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
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

# Include directories - point to parent src directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Collect all source files - adjust paths to parent directory
file(GLOB_RECURSE ALL_SOURCES 
    ../html/*.cpp
    ../css/*.cpp
    ../custom_js/*.cpp
    ../layout/*.cpp
    ../rendering/*.cpp
    ../networking/*.cpp
    ../security/*.cpp
    ../storage/*.cpp
    ../ui/*.cpp
    ../browser/*.cpp
)

# Platform-specific sources
if(WIN32)
    list(APPEND ALL_SOURCES ../ui/window_win32.cpp)
elseif(APPLE)
    list(APPEND ALL_SOURCES ../ui/window_macos.mm)
else()
    list(APPEND ALL_SOURCES ../ui/window_x11.cpp)
endif()

# Remove platform-specific files that aren't for this platform
if(WIN32)
    list(REMOVE_ITEM ALL_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/../ui/window_macos.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/../ui/window_x11.cpp)
elseif(APPLE)
    list(REMOVE_ITEM ALL_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/../ui/window_win32.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../ui/window_x11.cpp)
else()
    list(REMOVE_ITEM ALL_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/../ui/window_win32.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../ui/window_macos.mm)
endif()

# Create the browser library
add_library(browser_test_lib STATIC ${ALL_SOURCES})

# Platform-specific include directories
if(NOT WIN32 AND NOT APPLE)
    target_include_directories(browser_test_lib PUBLIC ${X11_INCLUDE_DIR})
endif()

# Test executables
add_executable(test_ui_rendering test_ui_rendering.cpp)
target_link_libraries(test_ui_rendering browser_test_lib ${PLATFORM_LIBS})

add_executable(simple_browser_test simple_browser_test.cpp)
target_link_libraries(simple_browser_test browser_test_lib ${PLATFORM_LIBS})

add_executable(minimal_window_test minimal_window_test.cpp)
target_link_libraries(minimal_window_test browser_test_lib ${PLATFORM_LIBS})

# Copy test files to build directory (they're in the current source directory)
configure_file(test_ui_rendering.cpp ${CMAKE_CURRENT_BINARY_DIR}/test_ui_rendering.cpp COPYONLY)
configure_file(simple_browser_test.cpp ${CMAKE_CURRENT_BINARY_DIR}/simple_browser_test.cpp COPYONLY)
configure_file(minimal_window_test.cpp ${CMAKE_CURRENT_BINARY_DIR}/minimal_window_test.cpp COPYONLY)

# Add custom target to run tests
add_custom_target(run_ui_tests
    COMMAND test_ui_rendering
    DEPENDS test_ui_rendering
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running UI/Rendering tests..."
)

add_custom_target(run_simple_test
    COMMAND simple_browser_test
    DEPENDS simple_browser_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running simple browser test..."
)

add_custom_target(run_minimal_test
    COMMAND minimal_window_test
    DEPENDS minimal_window_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running minimal window test..."
)

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")