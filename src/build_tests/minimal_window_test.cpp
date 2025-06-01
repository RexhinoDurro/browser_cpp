// minimal_window_test.cpp - Fixed version with proper paint handling
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>

#include "ui/window.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace browser::ui;

// Global flag to track if we need to redraw
std::atomic<bool> g_needsRedraw(true);
int g_frameCount = 0;

// Custom window class that handles painting properly
class TestWindow {
public:
    TestWindow(const WindowConfig& config) 
        : m_window(createPlatformWindow(config))
        , m_needsRedraw(true) {
    }
    
    bool create() {
        if (!m_window || !m_window->create()) {
            return false;
        }
        
        // Set up a resize callback to force redraw
        m_window->setResizeCallback([this](int w, int h) {
            m_needsRedraw = true;
        });
        
        return true;
    }
    
    void show() {
        m_window->show();
        forceRedraw();
    }
    
    bool processEvents() {
        bool result = m_window->processEvents();
        
        // Check if we need to redraw
        if (m_needsRedraw || (g_frameCount % 60 == 0)) {
            forceRedraw();
            m_needsRedraw = false;
        }
        
        return result;
    }
    
    void forceRedraw() {
        #ifdef _WIN32
        // On Windows, force a WM_PAINT message
        HWND hwnd = (HWND)m_window->getNativeHandle();
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
        #endif
        
        // Draw our content
        drawContent();
    }
    
    void drawContent() {
        m_window->beginPaint();
        Canvas* canvas = m_window->getCanvas();
        
        if (canvas) {
            // Clear to white
            canvas->clear(Canvas::rgb(255, 255, 255));
            
            // Draw a red rectangle
            canvas->drawRect(50, 50, 200, 100, Canvas::rgb(255, 0, 0), true);
            
            // Draw a blue border around it
            canvas->drawRect(50, 50, 200, 100, Canvas::rgb(0, 0, 255), false, 3);
            
            // Draw some text
            canvas->drawText("Minimal Window Test", 50, 200, Canvas::rgb(0, 0, 0), "Arial", 24);
            canvas->drawText("If you see this, basic windowing works!", 50, 250, Canvas::rgb(0, 0, 255), "Arial", 16);
            
            // Draw frame counter
            std::string frameText = "Frame: " + std::to_string(g_frameCount);
            canvas->drawText(frameText, 50, 300, Canvas::rgb(0, 128, 0), "Arial", 14);
            
            // Draw additional test shapes
            canvas->drawEllipse(300, 50, 100, 100, Canvas::rgb(0, 255, 0), true);
            canvas->drawLine(50, 350, 350, 350, Canvas::rgb(255, 0, 255), 5);
        }
        
        m_window->endPaint();
    }
    
    void close() {
        m_window->close();
    }
    
private:
    std::shared_ptr<Window> m_window;
    bool m_needsRedraw;
};

int main(int argc, char* argv[]) {
    std::cout << "Minimal Window Test - Fixed Version" << std::endl;
    std::cout << "===================================" << std::endl;
    
    // Step 1: Create window config
    std::cout << "\n1. Creating window configuration..." << std::endl;
    WindowConfig config;
    config.title = "Minimal Test Window";
    config.width = 640;
    config.height = 480;
    config.resizable = true;
    
    // Step 2: Create test window
    std::cout << "2. Creating test window..." << std::endl;
    TestWindow testWindow(config);
    
    // Step 3: Initialize window
    std::cout << "3. Initializing window..." << std::endl;
    if (!testWindow.create()) {
        std::cerr << "ERROR: Failed to create window!" << std::endl;
        return 1;
    }
    std::cout << "   Window created successfully" << std::endl;
    
    // Step 4: Show window
    std::cout << "4. Showing window..." << std::endl;
    testWindow.show();
    std::cout << "   Window should now be visible" << std::endl;
    
    // Step 5: Run event loop
    std::cout << "5. Running event loop (close window to exit)..." << std::endl;
    
    // Run until window is closed by user
    while (testWindow.processEvents()) {
        g_frameCount++;
        
        // Print status every second (assuming ~60 FPS)
        if (g_frameCount % 60 == 0) {
            std::cout << "   Frame " << g_frameCount << " - Window is running" << std::endl;
        }
        
        // Small delay
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    std::cout << "   Window closed by user" << std::endl;
    
    // Step 6: Cleanup
    std::cout << "6. Cleaning up..." << std::endl;
    testWindow.close();
    
    std::cout << "\nTest completed successfully!" << std::endl;
    std::cout << "Total frames rendered: " << g_frameCount << std::endl;
    
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