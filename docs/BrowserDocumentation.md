# Simple Browser Documentation

## Overview

This documentation covers the architecture and implementation of a simple web browser built from scratch in C++. The browser implements core web technologies including HTML parsing, CSS styling, JavaScript execution, layout engine, and rendering.

## Table of Contents

1. [Architecture Overview](architecture.md)
2. [Core Components](components/)
   - [HTML Parser](components/html-parser.md)
   - [CSS Engine](components/css-engine.md)
   - [JavaScript Engine](components/javascript-engine.md)
   - [Layout Engine](components/layout-engine.md)
   - [Rendering System](components/rendering-system.md)
   - [Networking](components/networking.md)
   - [Browser Core](components/browser-core.md)
3. [Platform Integration](platform/)
   - [Window System](platform/window-system.md)
   - [UI Controls](platform/ui-controls.md)
4. [Storage & Security](storage-security/)
   - [Local Storage](storage-security/local-storage.md)
   - [Security Manager](storage-security/security-manager.md)
5. [Build Instructions](build.md)
6. [API Reference](api/)

## Quick Start

```cpp
#include "browser/browser.h"
#include "ui/browser_window.h"

int main() {
    // Create browser window
    browser::ui::WindowConfig config;
    config.title = "Simple Browser";
    config.width = 1024;
    config.height = 768;
    
    auto window = std::make_shared<browser::ui::BrowserWindow>(config);
    
    // Initialize and run
    if (window->initialize()) {
        window->show();
        window->loadUrl("https://example.com");
        window->runEventLoop();
    }
    
    return 0;
}
```

## Project Structure

```
src/
├── browser/          # Core browser engine
├── html/            # HTML parser and DOM tree
├── css/             # CSS parser and style resolver
├── custom_js/       # JavaScript engine
├── layout/          # Layout engine and box model
├── rendering/       # Rendering system
├── networking/      # Resource loading
├── security/        # Security policies
├── storage/         # Local storage
└── ui/             # Platform UI and controls
```

## Features

- **HTML5 Parser**: Tokenization and tree construction
- **CSS3 Support**: Selectors, cascade, and inheritance
- **JavaScript Engine**: Custom interpreter with ES5 features
- **Layout Engine**: Box model, block/inline layout
- **Rendering**: Custom renderer with text and graphics
- **Cross-Platform**: Windows, macOS, and Linux support
- **Security**: Same-origin policy and CSP
- **Storage**: LocalStorage API implementation