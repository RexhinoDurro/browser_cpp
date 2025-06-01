#!/bin/bash

# Build script for browser UI/rendering tests

echo "Browser Test Build Script"
echo "========================"

# Detect OS
OS="Unknown"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="Linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macOS"
elif [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
    OS="Windows"
fi

echo "Detected OS: $OS"

# Create build directory
BUILD_DIR="build_tests"
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory: $BUILD_DIR"
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure with CMake
echo ""
echo "Configuring with CMake..."
if [ "$OS" == "Windows" ]; then
    cmake -G "Visual Studio 16 2019" -A x64 ..
else
    cmake -DCMAKE_BUILD_TYPE=Debug ..
fi

if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed"
    exit 1
fi

# Build
echo ""
echo "Building tests..."
if [ "$OS" == "Windows" ]; then
    cmake --build . --config Debug
else
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
fi

if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi

echo ""
echo "Build completed successfully!"
echo ""
echo "To run the tests:"
echo "  cd $BUILD_DIR"
echo "  ./minimal_window_test    # Run minimal window test (start here!)"
echo "  ./test_ui_rendering      # Run UI/rendering test suite"
echo "  ./simple_browser_test    # Run simple browser test"
echo ""

# For Windows, create batch files to run tests
if [ "$OS" == "Windows" ]; then
    cat > run_ui_tests.bat << EOF
@echo off
echo Running UI/Rendering Tests...
Debug\\test_ui_rendering.exe
pause
EOF

    cat > run_simple_test.bat << EOF
@echo off
echo Running Simple Browser Test...
Debug\\simple_browser_test.exe
pause
EOF

    echo "On Windows, you can also use:"
    echo "  run_ui_tests.bat"
    echo "  run_simple_test.bat"
fi