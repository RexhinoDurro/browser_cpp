cmake_minimum_required(VERSION 3.10)
project(browser_cpp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/std:c++20)  # Add explicit C++20 flag for MSVC
    # Define JS_ENGINE_DISABLED to skip JavaScript components
    add_definitions(-DJS_ENGINE_DISABLED)
endif()

# Include filesystem library for C++17
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    link_libraries(stdc++fs)
endif()

# Set vcpkg toolchain file if not already set
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")
endif()

# Find required packages
find_package(OpenGL REQUIRED)

# GLFW handling with vcpkg
message(STATUS "Trying to find GLFW3 in vcpkg...")
message(STATUS "Vcpkg Include Dir: ${CMAKE_CURRENT_SOURCE_DIR}/third_party/vcpkg/installed/x64-windows/include")
message(STATUS "Vcpkg Lib Dir: ${CMAKE_CURRENT_SOURCE_DIR}/third_party/vcpkg/installed/x64-windows/lib")

# Try to find GLFW3 with different approaches
find_package(glfw3 CONFIG QUIET)
if(glfw3_FOUND)
  message(STATUS "Found GLFW3 via CONFIG mode")
else()
  find_package(glfw CONFIG QUIET)
  if(glfw_FOUND)
    message(STATUS "Found GLFW via CONFIG mode")
  else()
    message(STATUS "Setting up GLFW3 manually")
    
    # Set up paths manually
    set(GLFW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vcpkg/installed/x64-windows/include")
    set(GLFW_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vcpkg/installed/x64-windows/lib/glfw3dll.lib")
    
    # Check if files exist
    if(EXISTS "${GLFW_INCLUDE_DIR}/GLFW/glfw3.h" AND EXISTS "${GLFW_LIBRARY}")
      message(STATUS "Manual GLFW setup: files found")
      
      # Create an imported target for GLFW if needed
      if(NOT TARGET glfw AND NOT TARGET glfw3)
        add_library(glfw INTERFACE IMPORTED)
        target_include_directories(glfw INTERFACE ${GLFW_INCLUDE_DIR})
        target_link_libraries(glfw INTERFACE ${GLFW_LIBRARY})
        message(STATUS "Created glfw interface target")
      endif()
    else()
      message(FATAL_ERROR "GLFW files not found at expected locations")
    endif()
  endif()
endif()

# Ensure GLFW targets are properly set up
if(TARGET glfw3 AND NOT TARGET glfw)
  message(STATUS "Creating glfw alias for glfw3")
  add_library(glfw ALIAS glfw3)
endif()

# Add NanoVG library
set(NANOVG_SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/nanovg")
include_directories(${NANOVG_SOURCE_DIR}/src)
add_library(nanovg STATIC
    ${NANOVG_SOURCE_DIR}/src/nanovg.c
)
target_link_libraries(nanovg ${OPENGL_LIBRARIES})

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

# Add networking sources
set(NETWORKING_SOURCES
    src/networking/http_client.cpp
    src/networking/dns_resolver.cpp
    src/networking/cache.cpp
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

# Add UI sources
set(UI_SOURCES
    src/ui/window.cpp
    src/ui/controls.cpp
)

# Add browser core
set(BROWSER_CORE_SOURCES
    src/browser/browser.cpp
)

# Create browser library
add_library(browser_core STATIC
    ${BROWSER_SOURCES}
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

# Create UI library
add_library(ui STATIC
    ${UI_SOURCES}
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
target_link_libraries(networking
    ${PLATFORM_LIBS}
)

target_link_libraries(security
    networking
)

target_link_libraries(storage
    security
)

target_link_libraries(ui
    browser_main
    nanovg
    glfw
    ${OPENGL_LIBRARIES}
)

target_link_libraries(browser_main
    browser_core
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
    networking
    security
    storage
    ui
    nanovg
    glfw
    ${OPENGL_LIBRARIES}
    ${PLATFORM_LIBS}
)

if(WIN32)
    target_link_libraries(browser_cpp wsock32 ws2_32)
endif()