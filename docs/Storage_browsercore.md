# Storage and Browser Core Documentation

## Local Storage

### Overview

The Local Storage implementation (`local_storage.h/cpp`) provides persistent storage for web applications, following the Web Storage API specification. It allows websites to store key-value pairs locally with data persisting even after the browser is closed.

### Architecture

```
┌────────────────────────────────────────────────┐
│              LocalStorage API                  │
│                    │                           │
│  ┌─────────────────┴─────────────────┐        │
│  │         StorageManager            │        │
│  │  ┌──────────┐  ┌──────────────┐ │        │
│  │  │Storage   │  │ Per-Origin   │ │        │
│  │  │Areas     │  │ Isolation    │ │        │
│  │  └──────────┘  └──────────────┘ │        │
│  └───────────────────────────────────┘        │
│                    │                           │
│  ┌─────────────────┴─────────────────┐        │
│  │          Persistence Layer         │        │
│  │  ┌──────────┐  ┌──────────────┐ │        │
│  │  │  Memory  │  │    Disk      │ │        │
│  │  │  Cache   │  │   Storage    │ │        │
│  │  └──────────┘  └──────────────┘ │        │
│  └───────────────────────────────────┘        │
└────────────────────────────────────────────────┘
```

### Storage Area

Each origin gets its own isolated storage area:

```cpp
class StorageArea {
    security::Origin m_origin;
    std::map<std::string, StorageItem> m_items;
    size_t m_size; // Total size in bytes
    
public:
    // Web Storage API methods
    std::string getItem(const std::string& key) const;
    bool setItem(const std::string& key, const std::string& value);
    bool removeItem(const std::string& key);
    void clear();
    size_t length() const;
    std::string key(size_t index) const;
};
```

### Storage Manager

Manages multiple storage areas and enforces quotas:

```cpp
class StorageManager {
    std::unordered_map<std::string, std::shared_ptr<StorageArea>> m_storageAreas;
    size_t m_quotaPerOrigin; // Default: 5MB
    
public:
    // Get storage for origin (creates if needed)
    std::shared_ptr<StorageArea> getStorageArea(const security::Origin& origin);
    
    // Quota management
    void setQuota(size_t bytes);
    size_t getTotalStorageSize() const;
    
    // Persistence
    bool persistAllStorage();
};
```

### JavaScript Binding

```cpp
class LocalStorage {
public:
    // JavaScript-compatible API
    std::string getItem(const std::string& key) const;
    bool setItem(const std::string& key, const std::string& value);
    bool removeItem(const std::string& key);
    void clear();
    size_t length() const;
    std::string key(size_t index) const;
    
    // Storage events
    void addEventListener(StorageEventCallback callback);
};
```

### Storage Events

Storage events fire when data changes:

```cpp
using StorageEventCallback = std::function<void(
    const std::string& key,
    const std::string& oldValue,
    const std::string& newValue,
    const security::Origin& origin
)>;
```

### Persistence Format

Storage is persisted as JSON files per origin:

```json
{
    "origin": "https://example.com",
    "items": [
        {
            "key": "username",
            "value": "john_doe",
            "timestamp": 1634567890
        },
        {
            "key": "preferences",
            "value": "{\"theme\":\"dark\",\"lang\":\"en\"}",
            "timestamp": 1634567900
        }
    ]
}
```

### Usage Example

```cpp
// Create storage for origin
StorageManager manager;
manager.initialize("./browser_storage");

Origin origin("https://example.com");
auto storage = manager.getStorageArea(origin);

// Store data
storage->setItem("user_id", "12345");
storage->setItem("session", "abc-def-ghi");

// Retrieve data
std::string userId = storage->getItem("user_id");

// Remove specific item
storage->removeItem("session");

// Clear all data for origin
storage->clear();
```

### Security Considerations

1. **Origin Isolation**: Each origin has separate storage
2. **Quota Enforcement**: 5MB default limit per origin
3. **No Encryption**: Data stored in plaintext
4. **Synchronous API**: Can block on large operations

## Browser Core

### Overview

The Browser class (`browser.h/cpp`) is the central coordinator that integrates all subsystems - HTML parsing, CSS styling, JavaScript execution, layout, and rendering.

### Architecture

```cpp
class Browser {
    // Core subsystems
    HTMLParser* m_htmlParser;
    StyleResolver* m_styleResolver;
    JSEngine* m_jsEngine;
    LayoutEngine* m_layoutEngine;
    Renderer* m_renderer;
    
    // Supporting systems
    ResourceLoader* m_resourceLoader;
    SecurityManager* m_securityManager;
    StorageManager* m_storageManager;
    
    // Current state
    std::shared_ptr<Document> m_document;
    std::shared_ptr<Box> m_layoutRoot;
};
```

### Initialization

```cpp
bool Browser::initialize() {
    // Initialize subsystems in order
    m_htmlParser = new HTMLParser();
    m_styleResolver = new StyleResolver();
    m_jsEngine = new JSEngine();
    m_layoutEngine = new LayoutEngine();
    m_renderer = new Renderer();
    
    // Initialize supporting systems
    m_resourceLoader = new ResourceLoader();
    m_securityManager = new SecurityManager();
    m_storageManager = new StorageManager();
    
    // Configure JavaScript bindings
    setupJavaScriptBindings();
    
    return true;
}
```

### Page Loading Flow

```cpp
bool Browser::loadUrl(const std::string& url, std::string& error) {
    // 1. Parse URL and check security
    Origin origin(url);
    if (!m_securityManager->canNavigate(m_currentOrigin, origin)) {
        error = "Navigation blocked by security policy";
        return false;
    }
    
    // 2. Fetch resource
    std::vector<uint8_t> data;
    std::map<std::string, std::string> headers;
    if (!m_resourceLoader->loadResource(url, data, headers, error)) {
        return false;
    }
    
    // 3. Parse HTML
    std::string html(data.begin(), data.end());
    DOMTree domTree = m_htmlParser->parse(html);
    m_document = domTree.document();
    
    // 4. Load stylesheets
    loadStylesheets();
    
    // 5. Resolve styles
    m_styleResolver->setDocument(m_document);
    m_styleResolver->resolveStyles();
    
    // 6. Build layout tree
    m_layoutRoot = m_layoutEngine->buildLayoutTree(
        m_document, m_styleResolver
    );
    
    // 7. Calculate layout
    m_layoutEngine->layoutDocument(
        m_document, m_styleResolver, 
        m_viewportWidth, m_viewportHeight
    );
    
    // 8. Execute scripts
    executeScripts();
    
    // 9. Update current state
    m_currentUrl = url;
    m_currentOrigin = origin;
    
    return true;
}
```

### JavaScript Integration

```cpp
void Browser::setupJavaScriptBindings() {
    // Create window object
    auto windowObj = std::make_shared<JSObject>();
    
    // Add document object
    windowObj->set("document", createDocumentObject());
    
    // Add localStorage
    windowObj->set("localStorage", createLocalStorageObject());
    
    // Add console
    windowObj->set("console", createConsoleObject());
    
    // Add navigation methods
    windowObj->set("location", createLocationObject());
    
    // Set as global
    m_jsEngine->setGlobalObject(windowObj);
}
```

### Document Object Binding

```cpp
std::shared_ptr<JSObject> Browser::createDocumentObject() {
    auto docObj = std::make_shared<JSObject>();
    
    // getElementById
    docObj->set("getElementById", JSValue(
        std::make_shared<JSFunction>(
            [this](const std::vector<JSValue>& args, JSValue thisValue) {
                if (args.empty()) return JSValue();
                
                std::string id = args[0].toString();
                Element* element = m_document->getElementById(id);
                
                if (element) {
                    return JSValue(wrapElement(element));
                }
                return JSValue(); // null
            }
        )
    ));
    
    // createElement
    docObj->set("createElement", JSValue(
        std::make_shared<JSFunction>(
            [this](const std::vector<JSValue>& args, JSValue thisValue) {
                if (args.empty()) return JSValue();
                
                std::string tagName = args[0].toString();
                auto element = m_document->createElement(tagName);
                
                return JSValue(wrapElement(element.get()));
            }
        )
    ));
    
    return docObj;
}
```

### LocalStorage Binding

```cpp
std::shared_ptr<JSObject> Browser::createLocalStorageObject() {
    auto storageObj = std::make_shared<JSObject>();
    
    // Get origin's storage
    auto localStorage = std::make_shared<LocalStorage>(
        m_storageManager, m_currentOrigin
    );
    
    // getItem
    storageObj->set("getItem", JSValue(
        std::make_shared<JSFunction>(
            [localStorage](const std::vector<JSValue>& args, JSValue thisValue) {
                if (args.empty()) return JSValue();
                
                std::string key = args[0].toString();
                std::string value = localStorage->getItem(key);
                
                return value.empty() ? JSValue() : JSValue(value);
            }
        )
    ));
    
    // setItem
    storageObj->set("setItem", JSValue(
        std::make_shared<JSFunction>(
            [localStorage](const std::vector<JSValue>& args, JSValue thisValue) {
                if (args.size() < 2) return JSValue();
                
                std::string key = args[0].toString();
                std::string value = args[1].toString();
                
                localStorage->setItem(key, value);
                return JSValue();
            }
        )
    ));
    
    return storageObj;
}
```

### Resource Loading

```cpp
void Browser::loadStylesheets() {
    // Find all <link rel="stylesheet"> elements
    auto links = m_document->getElementsByTagName("link");
    
    for (Element* link : links) {
        if (link->getAttribute("rel") == "stylesheet") {
            std::string href = link->getAttribute("href");
            
            // Resolve relative URL
            std::string fullUrl = resolveUrl(href, m_currentUrl);
            
            // Load stylesheet
            std::vector<uint8_t> data;
            std::map<std::string, std::string> headers;
            std::string error;
            
            if (m_resourceLoader->loadResource(fullUrl, data, headers, error)) {
                std::string css(data.begin(), data.end());
                
                // Parse and add stylesheet
                auto stylesheet = m_cssParser->parseStylesheet(css);
                m_styleResolver->addStyleSheet(*stylesheet);
            }
        }
    }
}
```

### Script Execution

```cpp
void Browser::executeScripts() {
    // Find all <script> elements
    auto scripts = m_document->getElementsByTagName("script");
    
    for (Element* script : scripts) {
        std::string src = script->getAttribute("src");
        
        if (!src.empty()) {
            // External script
            std::string fullUrl = resolveUrl(src, m_currentUrl);
            
            // Check CSP
            if (!m_securityManager->isAllowedByCSP(
                fullUrl, CspResourceType::SCRIPT, m_currentOrigin)) {
                continue; // Blocked by CSP
            }
            
            // Load script
            std::string scriptContent;
            if (loadScript(fullUrl, scriptContent)) {
                executeScript(scriptContent);
            }
        } else {
            // Inline script
            if (!m_securityManager->contentSecurityPolicy()->allowsInlineScript()) {
                continue; // Blocked by CSP
            }
            
            std::string scriptContent = script->textContent();
            executeScript(scriptContent);
        }
    }
}
```

### Event Handling

```cpp
void Browser::handleMouseClick(int x, int y) {
    // Find element at position
    Element* element = findElementAtPosition(x, y);
    if (!element) return;
    
    // Check for links
    if (element->tagName() == "a") {
        std::string href = element->getAttribute("href");
        if (!href.empty()) {
            // Navigate to link
            std::string fullUrl = resolveUrl(href, m_currentUrl);
            loadUrl(fullUrl);
        }
    }
    
    // Fire click event
    fireEvent(element, "click");
}
```

### Rendering Pipeline

```cpp
void Browser::render(RenderTarget* target) {
    if (!m_layoutRoot || !m_renderer) return;
    
    // 1. Create paint context
    PaintContext context;
    
    // 2. Paint layout tree
    m_paintSystem->paintBox(m_layoutRoot.get(), context);
    
    // 3. Render display list
    m_renderer->render(context.displayList(), target);
    
    // 4. Composite layers (if supported)
    m_compositor->composite(target);
}
```

## UI Controls

### Overview

The UI layer provides platform-independent controls and browser chrome implementation.

### Control Hierarchy

```cpp
// Base classes
class UIElement {
    int m_x, m_y, m_width, m_height;
    bool m_visible;
};

class UIControl : public UIElement {
    bool m_enabled;
    bool m_focused;
    bool m_hover;
};

// Concrete controls
class Button : public UIControl { };
class TextInput : public UIControl { };
class ProgressBar : public UIControl { };
```

### Browser Window Integration

```cpp
class BrowserWindow {
    // UI controls
    std::shared_ptr<Toolbar> m_toolbar;
    std::shared_ptr<Button> m_backButton;
    std::shared_ptr<Button> m_forwardButton;
    std::shared_ptr<TextInput> m_addressBar;
    std::shared_ptr<ProgressBar> m_progressBar;
    
    // Browser engine
    std::shared_ptr<Browser> m_browser;
    
    // Navigation state
    std::vector<std::string> m_history;
    size_t m_historyIndex;
};
```

### Custom Rendering

Controls can be rendered using the custom renderer:

```cpp
void Button::draw(CustomRenderContext* ctx) {
    // Draw background
    ctx->beginPath();
    ctx->roundedRect(m_x, m_y, m_width, m_height, 3.0f);
    
    Paint fillPaint;
    fillPaint.setColor(m_hover ? Color(230, 230, 255) : Color(240, 240, 240));
    ctx->setFillPaint(fillPaint);
    ctx->fill();
    
    // Draw text
    ctx->setFont(Font("sans", 12.0f));
    ctx->text(textX, textY, m_text);
}
```

## Platform Integration

### Window System Abstraction

```cpp
// Platform-independent interface
class Window {
    virtual bool create() = 0;
    virtual void show() = 0;
    virtual bool processEvents() = 0;
    virtual Canvas* getCanvas() = 0;
};

// Platform implementations
class Win32Window : public Window { };
class MacOSWindow : public Window { };
class X11Window : public Window { };
```

### Event Loop

```cpp
void BrowserWindow::runEventLoop() {
    while (isOpen()) {
        // Process platform events
        processEvents();
        
        // Update animations
        updateAnimations();
        
        // Render if needed
        if (m_needsRender) {
            renderPage();
            m_needsRender = false;
        }
        
        // Sleep to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}
```

## Testing

### Browser Core Tests

```cpp
TEST(Browser, LoadSimplePage) {
    Browser browser;
    browser.initialize();
    
    std::string html = "<html><body>Hello World</body></html>";
    browser.loadHtml(html);
    
    EXPECT_NE(browser.currentDocument(), nullptr);
    EXPECT_NE(browser.layoutRoot(), nullptr);
}

TEST(Browser, JavaScript) {
    Browser browser;
    browser.initialize();
    
    browser.loadHtml("<html><body><div id='test'></div></body></html>");
    
    std::string script = "document.getElementById('test').textContent = 'Modified';";
    browser.executeScript(script);
    
    Element* div = browser.currentDocument()->getElementById("test");
    EXPECT_EQ(div->textContent(), "Modified");
}
```

### Storage Tests

```cpp
TEST(LocalStorage, BasicOperations) {
    StorageManager manager;
    manager.initialize("./test_storage");
    
    Origin origin("https://example.com");
    LocalStorage storage(&manager, origin);
    
    // Test storage operations
    storage.setItem("key1", "value1");
    EXPECT_EQ(storage.getItem("key1"), "value1");
    EXPECT_EQ(storage.length(), 1);
    
    storage.removeItem("key1");
    EXPECT_EQ(storage.getItem("key1"), "");
    EXPECT_EQ(storage.length(), 0);
}
```

## Performance Considerations

### Memory Management

1. **DOM Tree**: Reference counted nodes
2. **Layout Tree**: Owned by layout engine
3. **Style Cache**: LRU cache for computed styles
4. **Storage**: Memory-mapped files for large data

### Optimization Strategies

1. **Incremental Layout**: Only recalculate changed portions
2. **Style Sharing**: Reuse computed styles for similar elements
3. **Lazy Loading**: Load resources on demand
4. **Storage Compression**: Compress stored data

## Limitations

1. **No Web Workers**: Single-threaded execution
2. **No IndexedDB**: Only localStorage supported
3. **No Service Workers**: No offline capabilities
4. **Limited Media**: No audio/video support
5. **No WebGL**: Basic 2D rendering only

## Future Enhancements

1. **Web Workers**: Background script execution
2. **IndexedDB**: Advanced storage API
3. **WebRTC**: Real-time communication
4. **WebAssembly**: High-performance code execution
5. **Progressive Web Apps**: Offline and installable