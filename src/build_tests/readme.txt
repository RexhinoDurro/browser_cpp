# Browser UI and Rendering Tests

This directory contains test programs to help debug and verify the UI and rendering components of the Simple Browser.

## Test Programs

### 1. `test_ui_rendering.cpp`
A comprehensive test suite that tests individual components:
- Basic window creation
- Canvas drawing operations
- Custom renderer functionality
- Custom canvas integration
- Browser controls
- Paint system
- Full rendering pipeline

### 2. `simple_browser_test.cpp`
A minimal test that creates a browser window and attempts to load a page, with detailed logging to help identify issues.

### 3. `minimal_window_test.cpp`
The absolute minimal test that only creates a window and draws basic shapes. Use this to verify that the platform-specific windowing code works before testing the browser components.

### 4. `debug_main.cpp`
A debug version of the main program with extensive logging to trace execution flow.

## Building the Tests

### Prerequisites
- CMake 3.10 or higher
- C++17 compatible compiler
- Platform-specific dependencies:
  - **Windows**: Visual Studio 2019 or 2022
  - **macOS**: Xcode Command Line Tools
  - **Linux**: X11 development libraries (`libx11-dev`)

### Build Instructions

#### Option 1: Using the build scripts

**On Linux/macOS:**
```bash
chmod +x build_tests.sh
./build_tests.sh
```

**On Windows:**
```batch
build_tests.bat
```

#### Option 2: Manual build

```bash
# Create build directory
mkdir build_tests
cd build_tests

# Configure
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Build
cmake --build .

# Or on Unix-like systems:
make -j4
```

## Running the Tests

After building, from the `build_tests` directory:

### Minimal Window Test (Start Here!)
```bash
# Linux/macOS
./minimal_window_test

# Windows
minimal_window_test.exe
# or
Debug\minimal_window_test.exe
```

This is the simplest test - it just creates a window and draws a red rectangle. If this doesn't work, the issue is in the platform window code.

### Test Suite
```bash
# Linux/macOS
./test_ui_rendering

# Windows
test_ui_rendering.exe
# or
Debug\test_ui_rendering.exe
```

This runs through all test cases and reports which ones pass or fail.

### Simple Browser Test
```bash
# Linux/macOS
./simple_browser_test

# Windows
simple_browser_test.exe
# or
Debug\simple_browser_test.exe
```

This creates a browser window and attempts to load a page with detailed logging.

## Debugging Tips

### Common Issues and Solutions

1. **Window doesn't appear**
   - Check if the platform window creation succeeds in the logs
   - Verify that the event loop is running
   - On Linux, ensure X11 is running and DISPLAY is set

2. **Nothing is rendered**
   - Check if the canvas is properly initialized
   - Verify that beginPaint/endPaint are called
   - Look for errors in the paint system initialization

3. **Browser controls not visible**
   - Check if BrowserControls::initialize() succeeds
   - Verify the toolbar height is correct
   - Check if controls are being drawn in the correct coordinates

4. **Page doesn't load**
   - Check HTML parser initialization
   - Verify style resolver is working
   - Check layout engine calculations
   - Look for errors in the renderer

### Debug Output

The tests produce detailed output to help identify issues:

```
[DEBUG] Step 1: Creating browser instance...
[DEBUG] Browser instance created successfully
[DEBUG] Step 2: Initializing browser engine...
[DEBUG] Browser engine initialized successfully
[DEBUG] Checking browser components:
[DEBUG]   - HTML Parser: OK
[DEBUG]   - CSS Style Resolver: OK
[DEBUG]   - Layout Engine: OK
[DEBUG]   - Renderer: OK
...
```

### Platform-Specific Notes

#### Windows
- If you get linking errors, make sure you're linking against: `ws2_32.lib`, `winmm.lib`, `gdi32.lib`, `user32.lib`
- Run from Visual Studio for better debugging experience
- Use the Debug configuration to get more detailed error messages

#### macOS
- If window doesn't appear, check if the app needs to be authorized in System Preferences
- Make sure you're running from Terminal or with proper app bundle setup
- Check Console.app for additional error messages

#### Linux
- Ensure X11 development headers are installed: `sudo apt-get install libx11-dev`
- Check if `$DISPLAY` environment variable is set
- Try running with `DISPLAY=:0 ./test_ui_rendering` if having display issues

## Expected Output

### Successful Test Run
```
Browser UI and Rendering Test Suite
===================================

Running: Basic Window Creation
Window created successfully
Test 1 PASSED

Running: Canvas Drawing
Test 2 PASSED

...

Test Summary:
=============
Passed: 7
Failed: 0
Total:  7

All tests PASSED!
```

### Simple Browser Test Success
```
Simple Browser Test
=========================

1. Creating browser instance...
2. Initializing browser engine...
   Browser engine initialized successfully
3. Creating browser window...
4. Setting browser instance...
5. Initializing browser window...
   Browser window initialized successfully
6. Showing browser window...
7. Loading test page...
   HTML parsed successfully
   about:home loaded successfully
8. Running event loop (window should be visible)...
   Window will close automatically in 5 seconds
   Frame 60 - Window is open
   ...
9. Closing browser window...

Test completed successfully!
```

## Next Steps

If the tests reveal issues:

1. **Isolate the problem**: Run individual tests to find which component fails
2. **Check initialization order**: Many issues are caused by components not being initialized in the correct order
3. **Verify platform code**: Platform-specific window implementations may have bugs
4. **Debug rendering pipeline**: Use the ASCII renderer to see if layout is working even if visual rendering isn't
5. **Check error messages**: Look for any error messages in the console output

## Adding New Tests

To add a new test:

1. Add a new test function in `test_ui_rendering.cpp`:
```cpp
bool testNewFeature() {
    std::cout << "\n=== Test N: New Feature ===" << std::endl;
    // Test implementation
    return true; // or false if failed
}
```

2. Add it to the test array in main():
```cpp
Test tests[] = {
    // ... existing tests ...
    {"New Feature", testNewFeature}
};
```

3. Rebuild and run the tests.