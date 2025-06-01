// minimal_window_test.cpp - Absolute minimal test for window creation
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

// Only include the window headers
#include "ui/window.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace browser::ui;

int main(int argc, char* argv[]) {
    std::cout << "Minimal Window Test" << std::endl;
    std::cout << "==================" << std::endl;
    
    // Step 1: Create window config
    std::cout << "\n1. Creating window configuration..." << std::endl;
    WindowConfig config;
    config.title = "Minimal Test Window";
    config.width = 640;
    config.height = 480;
    config.resizable = true;
    
    // Step 2: Create platform window
    std::cout << "2. Creating platform window..." << std::endl;
    auto window = createPlatformWindow(config);
    if (!window) {
        std::cerr << "ERROR: createPlatformWindow returned null!" << std::endl;
        return 1;
    }
    std::cout << "   Platform window created" << std::endl;
    
    // Step 3: Initialize window
    std::cout << "3. Calling window->create()..." << std::endl;
    if (!window->create()) {
        std::cerr << "ERROR: window->create() failed!" << std::endl;
        return 1;
    }
    std::cout << "   Window created successfully" << std::endl;
    
    // Step 4: Show window
    std::cout << "4. Showing window..." << std::endl;
    window->show();
    std::cout << "   Window should now be visible" << std::endl;
    
    // Step 5: Simple event loop with drawing
    std::cout << "5. Running event loop (close window to exit)..." << std::endl;

int frameCount = 0;
bool needsRedraw = true;

// Run until window is closed by user
while (true) {
    // Process events
    if (!window->processEvents()) {
        std::cout << "   Window closed by user" << std::endl;
        break;
    }
    
    // Draw something simple
    if (needsRedraw || frameCount % 60 == 0) {  // Redraw every second
        window->beginPaint();
        Canvas* canvas = window->getCanvas();
        
        if (canvas) {
            // Clear to white
            canvas->clear(Canvas::rgb(255, 255, 255));
            
            // Draw a red rectangle
            canvas->drawRect(50, 50, 200, 100, Canvas::rgb(255, 0, 0), true);
            
            // Draw some text
            canvas->drawText("Minimal Window Test", 50, 200, Canvas::rgb(0, 0, 0), "Arial", 24);
            canvas->drawText("If you see this, basic windowing works!", 50, 250, Canvas::rgb(0, 0, 255), "Arial", 16);
            
            // Draw frame counter
            std::string frameText = "Frame: " + std::to_string(frameCount);
            canvas->drawText(frameText, 50, 300, Canvas::rgb(0, 128, 0), "Arial", 14);
        }
        
        window->endPaint();
        needsRedraw = false;
    }
    
    frameCount++;
    
    // Small delay
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
}
    
    // Step 6: Close window
    std::cout << "6. Closing window..." << std::endl;
    window->close();
    
    std::cout << "\nTest completed successfully!" << std::endl;
    std::cout << "Total frames rendered: " << frameCount << std::endl;
    
    return 0;
}

// Windows-specific entry point
#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Allocate console for output
    AllocConsole();
    FILE* pCout;
    freopen_s(&pCout, "CONOUT$", "w", stdout);
    FILE* pCerr;
    freopen_s(&pCerr, "CONOUT$", "w", stderr);
    
    // Get command line arguments
    int argc;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    // Convert to char**
    char** argv = new char*[argc];
    for (int i = 0; i < argc; i++) {
        int size = WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, nullptr, 0, nullptr, nullptr);
        argv[i] = new char[size];
        WideCharToMultiByte(CP_UTF8, 0, argvW[i], -1, argv[i], size, nullptr, nullptr);
    }
    
    LocalFree(argvW);
    
    // Call main
    int result = main(argc, argv);
    
    // Cleanup
    for (int i = 0; i < argc; i++) {
        delete[] argv[i];
    }
    delete[] argv;
    
    // Keep console open
    std::cout << "\nPress Enter to close..." << std::endl;
    std::cin.get();
    
    fclose(pCout);
    fclose(pCerr);
    FreeConsole();
    
    return result;
}
#endif