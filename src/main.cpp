// src/main.cpp - Main entry point for the Simple Browser

#include "browser/browser.h"
#include "ui/browser_window.h"
#include <iostream>
#include <string>
#include <memory>
#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

// Global browser instance
std::shared_ptr<browser::Browser> g_browser;
std::shared_ptr<browser::ui::BrowserWindow> g_window;

// Print usage information
void printUsage(const char* programName) {
    std::cout << "Simple Browser - A web browser built from scratch\n\n";
    std::cout << "Usage: " << programName << " [options] [URL]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "  -v, --version        Show version information\n";
    std::cout << "  --width <pixels>     Set initial window width (default: 1024)\n";
    std::cout << "  --height <pixels>    Set initial window height (default: 768)\n";
    std::cout << "  --maximized          Start with maximized window\n";
    std::cout << "  --cache-dir <path>   Set cache directory\n";
    std::cout << "  --no-cache           Disable caching\n";
    std::cout << "  --incognito          Start in incognito mode (no persistent storage)\n";
    std::cout << "  --debug              Enable debug output\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " https://example.com\n";
    std::cout << "  " << programName << " --width 1280 --height 720 https://example.com\n";
}

// Parse command line arguments
struct CommandLineArgs {
    std::string initialUrl;
    int windowWidth = 1024;
    int windowHeight = 768;
    bool maximized = false;
    std::string cacheDir;
    bool noCache = false;
    bool incognito = false;
    bool debug = false;
    bool showHelp = false;
    bool showVersion = false;
};

CommandLineArgs parseCommandLine(int argc, char* argv[]) {
    CommandLineArgs args;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            args.showHelp = true;
        }
        else if (arg == "-v" || arg == "--version") {
            args.showVersion = true;
        }
        else if (arg == "--width" && i + 1 < argc) {
            args.windowWidth = std::stoi(argv[++i]);
        }
        else if (arg == "--height" && i + 1 < argc) {
            args.windowHeight = std::stoi(argv[++i]);
        }
        else if (arg == "--maximized") {
            args.maximized = true;
        }
        else if (arg == "--cache-dir" && i + 1 < argc) {
            args.cacheDir = argv[++i];
        }
        else if (arg == "--no-cache") {
            args.noCache = true;
        }
        else if (arg == "--incognito") {
            args.incognito = true;
        }
        else if (arg == "--debug") {
            args.debug = true;
        }
        else if (arg[0] != '-') {
            // Assume it's a URL
            args.initialUrl = arg;
        }
        else {
            std::cerr << "Unknown option: " << arg << std::endl;
            args.showHelp = true;
        }
    }
    
    return args;
}

// Get default cache directory
std::string getDefaultCacheDir() {
#ifdef _WIN32
    char* appData = std::getenv("APPDATA");
    if (appData) {
        return std::string(appData) + "\\SimpleBrowser\\cache";
    }
    return "cache";
#elif defined(__APPLE__)
    char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/Library/Caches/SimpleBrowser";
    }
    return "cache";
#else
    char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/.cache/simplebrowser";
    }
    return "cache";
#endif
}

// Get default storage directory
std::string getDefaultStorageDir() {
#ifdef _WIN32
    char* appData = std::getenv("APPDATA");
    if (appData) {
        return std::string(appData) + "\\SimpleBrowser\\storage";
    }
    return "storage";
#elif defined(__APPLE__)
    char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/Library/Application Support/SimpleBrowser";
    }
    return "storage";
#else
    char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/.local/share/simplebrowser";
    }
    return "storage";
#endif
}

// Initialize logging
void initializeLogging(bool debug) {
    if (debug) {
        std::cout << "Debug logging enabled\n";
        // In a real implementation, you would configure a logging library here
    }
}

// Clean shutdown handler
void shutdownHandler(int signal) {
    std::cout << "\nShutting down browser..." << std::endl;
    
    if (g_window) {
        g_window->close();
    }
    
    std::exit(0);
}

// Main entry point
int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        CommandLineArgs args = parseCommandLine(argc, argv);
        
        // Handle help/version
        if (args.showHelp) {
            printUsage(argv[0]);
            return 0;
        }
        
        if (args.showVersion) {
            std::cout << "Simple Browser version 1.0.0" << std::endl;
            return 0;
        }
        
        // Initialize logging
        initializeLogging(args.debug);
        
        // Set up signal handlers
#ifdef _WIN32
        SetConsoleCtrlHandler([](DWORD) -> BOOL {
            shutdownHandler(0);
            return TRUE;
        }, TRUE);
#else
        signal(SIGINT, shutdownHandler);
        signal(SIGTERM, shutdownHandler);
#endif
        
        // Create browser instance
        g_browser = std::make_shared<browser::Browser>();
        
        // Initialize browser
        std::cout << "Initializing browser engine..." << std::endl;
        if (!g_browser->initialize()) {
            std::cerr << "Failed to initialize browser engine" << std::endl;
            return 1;
        }
        
        // Configure cache
        if (!args.noCache && !args.incognito) {
            std::string cacheDir = args.cacheDir.empty() ? getDefaultCacheDir() : args.cacheDir;
            
            // Create cache directory if it doesn't exist
            try {
                fs::create_directories(cacheDir);
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to create cache directory: " << e.what() << std::endl;
            }
        }
        
        // Configure storage
        if (!args.incognito) {
            std::string storageDir = getDefaultStorageDir();
            
            // Create storage directory if it doesn't exist
            try {
                fs::create_directories(storageDir);
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to create storage directory: " << e.what() << std::endl;
            }
        }
        
        // Create window configuration
        browser::ui::WindowConfig windowConfig;
        windowConfig.title = "Simple Browser";
        windowConfig.width = args.windowWidth;
        windowConfig.height = args.windowHeight;
        windowConfig.maximized = args.maximized;
        
        // Create browser window
        std::cout << "Creating browser window..." << std::endl;
        g_window = std::make_shared<browser::ui::BrowserWindow>(windowConfig);
        
        // Set browser instance
        g_window->setBrowser(g_browser);
        
        // Initialize window
        if (!g_window->initialize()) {
            std::cerr << "Failed to initialize browser window" << std::endl;
            return 1;
        }
        
        std::cout << "Browser window initialized successfully" << std::endl;
        
        // Set up callbacks
        g_window->setUrlChangeCallback([](const std::string& url) {
            std::cout << "Navigated to: " << url << std::endl;
        });
        
        g_window->setTitleChangeCallback([](const std::string& title) {
            if (g_window) {
                g_window->setTitle(title + " - Simple Browser");
            }
        });
        
        g_window->setLoadingStateCallback([](bool isLoading) {
            if (isLoading) {
                std::cout << "Loading..." << std::endl;
            } else {
                std::cout << "Page loaded" << std::endl;
            }
        });
        
        // IMPORTANT: Show the window!
        std::cout << "Showing browser window..." << std::endl;
        g_window->show();
        
        // Load initial URL if specified
        if (!args.initialUrl.empty()) {
            std::cout << "Loading URL: " << args.initialUrl << std::endl;
            g_window->loadUrl(args.initialUrl);
        }
        // Note: showDefaultPage() is already called in BrowserWindow::initialize()
        
        // Give the window time to show
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Run event loop - THIS IS CRITICAL!
        std::cout << "Browser is running. Press Ctrl+C to quit." << std::endl;
        g_window->runEventLoop();
        
        // Cleanup
        std::cout << "Browser closed." << std::endl;
        g_window.reset();
        g_browser.reset();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error" << std::endl;
        return 1;
    }
}

// Windows-specific entry point
#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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
    
    return result;
}
#endif