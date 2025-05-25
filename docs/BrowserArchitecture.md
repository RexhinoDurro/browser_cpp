# Browser Architecture Overview

## High-Level Architecture

The browser is built using a modular architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────────┐
│                         Browser Window                          │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                      UI Controls                           │ │
│  │  (Address Bar, Navigation Buttons, Progress Bar)          │ │
│  └───────────────────────────────────────────────────────────┘ │
│                               │                                 │
│  ┌───────────────────────────┴─────────────────────────────┐  │
│  │                    Browser Engine                        │  │
│  │  ┌─────────────┐  ┌──────────────┐  ┌───────────────┐ │  │
│  │  │ HTML Parser │  │ CSS Engine   │  │ JS Engine     │ │  │
│  │  └──────┬──────┘  └──────┬───────┘  └───────┬───────┘ │  │
│  │         │                │                   │          │  │
│  │         └────────────────┴───────────────────┘          │  │
│  │                          │                              │  │
│  │  ┌──────────────────────┴────────────────────────────┐ │  │
│  │  │                  DOM Tree                          │ │  │
│  │  └──────────────────────┬────────────────────────────┘ │  │
│  │                          │                              │  │
│  │  ┌──────────────────────┴────────────────────────────┐ │  │
│  │  │               Layout Engine                        │ │  │
│  │  │         (Box Model, Layout Tree)                   │ │  │
│  │  └──────────────────────┬────────────────────────────┘ │  │
│  │                          │                              │  │
│  │  ┌──────────────────────┴────────────────────────────┐ │  │
│  │  │              Rendering System                      │ │  │
│  │  │        (Paint System, Display List)                │ │  │
│  │  └────────────────────────────────────────────────────┘ │  │
│  └──────────────────────────────────────────────────────────┘  │
│                               │                                 │
│  ┌───────────────────────────┴─────────────────────────────┐  │
│  │                Platform Integration                      │  │
│  │     (Windows/macOS/Linux Window System)                 │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. Browser Engine (`src/browser/`)

The central coordinator that manages all subsystems:

```cpp
class Browser {
    HTMLParser* m_htmlParser;
    StyleResolver* m_styleResolver;
    LayoutEngine* m_layoutEngine;
    Renderer* m_renderer;
    JSEngine* m_jsEngine;
    ResourceLoader* m_resourceLoader;
    SecurityManager* m_securityManager;
};
```

### 2. HTML Parser (`src/html/`)

Converts HTML text into a DOM tree:
- **Tokenizer**: Breaks HTML into tokens
- **Tree Builder**: Constructs DOM nodes
- **Error Recovery**: Handles malformed HTML

### 3. CSS Engine (`src/css/`)

Processes stylesheets and computes styles:
- **CSS Parser**: Parses CSS syntax
- **Style Resolver**: Matches selectors to elements
- **Cascade**: Determines final property values

### 4. JavaScript Engine (`src/custom_js/`)

Executes JavaScript code:
- **Lexer**: Tokenizes JavaScript
- **Parser**: Builds AST
- **Interpreter**: Executes AST nodes
- **Value System**: Manages JS values and objects

### 5. Layout Engine (`src/layout/`)

Calculates positions and sizes:
- **Box Model**: Margin, border, padding, content
- **Layout Tree**: Parallel tree to DOM
- **Flow Layout**: Block and inline layout

### 6. Rendering System (`src/rendering/`)

Draws the final output:
- **Paint System**: Creates display list
- **Custom Renderer**: Vector graphics API
- **Render Targets**: Platform-specific output

## Data Flow

### Page Load Sequence

```
1. User enters URL
   ↓
2. ResourceLoader fetches HTML
   ↓
3. HTMLParser creates DOM tree
   ↓
4. StyleResolver computes styles
   ↓
5. LayoutEngine builds layout tree
   ↓
6. Renderer paints to screen
   ↓
7. JSEngine executes scripts
   ↓
8. (Repeat 4-6 for updates)
```

### Event Handling

```
User Input → Window System → Browser Window → DOM Events → JS Handlers
                                   ↓
                          UI Controls Update
```

## Threading Model

The browser uses a single-threaded model with event loop:

```cpp
void BrowserWindow::runEventLoop() {
    while (isOpen()) {
        processEvents();      // Handle UI events
        
        if (m_needsRender) {
            renderPage();     // Render updates
            m_needsRender = false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
```

## Memory Management

- **Smart Pointers**: `std::shared_ptr` for shared ownership
- **DOM Nodes**: Reference counted with parent-child relationships
- **Layout Boxes**: Owned by layout tree
- **Render Objects**: Temporary, recreated each frame

## Security Architecture

### Same-Origin Policy
```cpp
class Origin {
    std::string scheme;  // http, https
    std::string host;    // example.com
    uint16_t port;       // 80, 443
};
```

### Content Security Policy
- Script execution control
- Resource loading restrictions
- XSS prevention

## Platform Abstraction

The browser uses abstract interfaces for platform-specific features:

```cpp
// Platform-independent interface
class Window {
    virtual bool create() = 0;
    virtual void show() = 0;
    virtual Canvas* getCanvas() = 0;
};

// Platform implementations
class Win32Window : public Window { };
class MacOSWindow : public Window { };
class X11Window : public Window { };
```

## Extension Points

The architecture supports extensions through:

1. **Custom Protocols**: Register handlers for new URL schemes
2. **Script Bindings**: Add native functions to JavaScript
3. **Render Backends**: Implement new render targets
4. **UI Themes**: Customize control appearance