@echo off
REM build_tests.bat - Windows build script for browser tests

echo Browser Test Build Script (Windows)
echo ==================================

REM Create build directory
set BUILD_DIR=build_tests
if not exist "%BUILD_DIR%" (
    echo Creating build directory: %BUILD_DIR%
    mkdir "%BUILD_DIR%"
)

cd "%BUILD_DIR%"

REM Configure with CMake
echo.
echo Configuring with CMake...

REM Try to detect Visual Studio version
where cl >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Using current Visual Studio environment
    cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
    if %ERRORLEVEL% NEQ 0 goto :cmake_error
    
    echo.
    echo Building tests...
    nmake
    if %ERRORLEVEL% NEQ 0 goto :build_error
) else (
    REM Try Visual Studio 2019
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019" (
        echo Using Visual Studio 2019
        cmake -G "Visual Studio 16 2019" -A x64 ..
    ) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022" (
        echo Using Visual Studio 2022
        cmake -G "Visual Studio 17 2022" -A x64 ..
    ) else (
        echo ERROR: Visual Studio not found
        echo Please install Visual Studio or run this script from a Developer Command Prompt
        goto :error
    )
    
    if %ERRORLEVEL% NEQ 0 goto :cmake_error
    
    echo.
    echo Building tests...
    cmake --build . --config Debug
    if %ERRORLEVEL% NEQ 0 goto :build_error
)

echo.
echo Build completed successfully!
echo.
echo To run the tests:
echo   cd %BUILD_DIR%
echo   minimal_window_test.exe    (Minimal window test - start here!)
echo   test_ui_rendering.exe      (Run UI/rendering test suite)
echo   simple_browser_test.exe    (Run simple browser test)
echo.

REM Create convenience batch files
echo @echo off > run_minimal_test.bat
echo echo Running Minimal Window Test... >> run_minimal_test.bat
echo Debug\minimal_window_test.exe >> run_minimal_test.bat
echo if not exist Debug\minimal_window_test.exe minimal_window_test.exe >> run_minimal_test.bat
echo pause >> run_minimal_test.bat

echo @echo off > run_ui_tests.bat
echo echo Running UI/Rendering Tests... >> run_ui_tests.bat
echo Debug\test_ui_rendering.exe >> run_ui_tests.bat
echo if not exist Debug\test_ui_rendering.exe test_ui_rendering.exe >> run_ui_tests.bat
echo pause >> run_ui_tests.bat

echo @echo off > run_simple_test.bat
echo echo Running Simple Browser Test... >> run_simple_test.bat
echo Debug\simple_browser_test.exe >> run_simple_test.bat
echo if not exist Debug\simple_browser_test.exe simple_browser_test.exe >> run_simple_test.bat
echo pause >> run_simple_test.bat

echo You can also use the convenience batch files:
echo   run_minimal_test.bat
echo   run_ui_tests.bat
echo   run_simple_test.bat

pause
exit /b 0

:cmake_error
echo ERROR: CMake configuration failed
pause
exit /b 1

:build_error
echo ERROR: Build failed
pause
exit /b 1

:error
pause
exit /b 1