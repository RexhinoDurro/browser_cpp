// src/build_tests/debug_main.cpp - Debug version of main with extensive logging
#include "../browser/browser.h"
#include "../ui/browser_window.h"
#include <iostream>
#include <string>
#include <memory>
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

// Enable debug output
#define DEBUG_LOG(msg) std::cout << "[DEBUG] " << msg << std::endl

int main(int argc, char* argv[]) {
    DEBUG_LOG("Starting debug browser...");
    
    try {
        // Step 1: Create browser instance
        DEBUG_LOG("Step 1: Creating browser instance...");
        auto browser = std::make_shared<browser::Browser>();
        if (!browser) {
            std::cerr << "[ERROR] Failed to create browser instance" << std::endl;
            return 1;
        }
        DEBUG_LOG("Browser instance created successfully");
        
        // Step 2: Initialize browser engine
        DEBUG_LOG("Step 2: Initializing browser engine...");
        if (!browser->initialize()) {
            std::cerr << "[ERROR] Failed to initialize browser engine" << std::endl;
            return 1;
        }
        DEBUG_LOG("Browser engine initialized successfully");
        
        // Check each component
        DEBUG_LOG("Checking browser components:");
        DEBUG_LOG("  - HTML Parser: " << (browser->htmlParser() ? "OK" : "FAILED"));
        DEBUG_LOG("  - CSS Style Resolver: " << (browser->styleResolver() ? "OK" : "FAILED"));
        DEBUG_LOG("  - Layout Engine: " << (browser->layoutEngine() ? "OK" : "FAILED"));
        DEBUG_LOG("  - JS Engine: " << (browser->jsEngine() ? "OK" : "FAILED"));
        DEBUG_LOG("  - Resource Loader: " << (browser->resourceLoader() ? "OK" : "FAILED"));
        DEBUG_LOG("  - Security Manager: " << (browser->securityManager() ? "OK" : "FAILED"));
        
        // Step 3: Create window configuration
        DEBUG_LOG("Step 3: Creating window configuration...");
        browser::ui::WindowConfig config;
        config.title = "Debug Browser";
        config.width = 1024;
        config.height = 768;
        config.resizable = true;
        
        // Step 4: Create browser window
        DEBUG_LOG("Step 4: Creating browser window...");
        auto window = std::make_shared<browser::ui::BrowserWindow>(config);
        if (!window) {
            std::cerr << "[ERROR] Failed to create browser window" << std::endl;
            return 1;
        }
        DEBUG_LOG("Browser window created successfully");
        
        // Step 5: Set browser instance
        DEBUG_LOG("Step 5: Setting browser instance...");
        window->setBrowser(browser);
        
        // Step 6: Initialize window
        DEBUG_LOG("Step 6: Initializing browser window...");
        if (!window->initialize()) {
            std::cerr << "[ERROR] Failed to initialize browser window" << std::endl;
            return 1;
        }
        DEBUG_LOG("Browser window initialized successfully");
        
        // Step 7: Set up callbacks
        DEBUG_LOG("Step 7: Setting up callbacks...");
        window->setUrlChangeCallback([](const std::string& url) {
            DEBUG_LOG("URL changed to: " << url);
        });
        
        window->setTitleChangeCallback([window](const std::string& title) {
            DEBUG_LOG("Title changed to: " << title);
            window->setTitle(title + " - Debug Browser");
        });
        
        window->setLoadingStateCallback([](bool isLoading) {
            DEBUG_LOG("Loading state: " << (isLoading ? "LOADING" : "DONE"));
        });
        
        // Step 8: Show window
        DEBUG_LOG("Step 8: Showing window...");
        window->show();
        
        // Step 9: Load initial page
        DEBUG_LOG("Step 9: Loading initial page...");
        std::string initialUrl = "about:home";
        if (argc > 1) {
            initialUrl = argv[1];
        }
        
        DEBUG_LOG("Loading URL: " << initialUrl);
        window->loadUrl(initialUrl);
        
        // Step 10: Run event loop
        DEBUG_LOG("Step 10: Running event loop...");
        DEBUG_LOG("Window is open. Press Ctrl+C to quit.");
        
        int frameCount = 0;
        auto lastDebugTime = std::chrono::steady_clock::now();
        
        while (window->isOpen()) {
            window->processEvents();
            
            frameCount++;
            
            // Print debug info every second
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastDebugTime);
            if (elapsed.count() >= 1) {
                DEBUG_LOG("Frame " << frameCount << " - FPS: " << frameCount);
                frameCount = 0;
                lastDebugTime = now;
            }
            
            // Small delay to avoid consuming too much CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
        
        DEBUG_LOG("Window closed");
        
    } catch (const std::exception& e) {
        std::cerr << "[FATAL ERROR] Exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "[FATAL ERROR] Unknown exception" << std::endl;
        return 1;
    }
    
    DEBUG_LOG("Debug browser exiting normally");
    return 0;
}

// Windows-specific entry point
#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Allocate console for debug output
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