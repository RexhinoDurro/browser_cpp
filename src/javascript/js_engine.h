#ifndef BROWSER_JS_ENGINE_H
#define BROWSER_JS_ENGINE_H

#include <string>
#include <memory>
#include <map>
#include <functional>
#include "../html/dom_tree.h"

// Forward declare QuickJS structs to avoid including them in header
struct JSRuntime;
struct JSContext;
struct JSValue;

namespace browser {
namespace javascript {

// JavaScript engine wrapper class
class JSEngine {
public:
    JSEngine();
    ~JSEngine();
    
    // Initialize the JavaScript engine
    bool initialize();
    
    // Execute a script on a document
    bool executeScript(html::Document* document, const std::string& script, 
                      std::string& result, std::string& error);
    
    // Process the JavaScript event loop (timers, promises, etc.)
    void processEventLoop();
    
    // Add event listener to an element
    bool addEventListener(html::Element* element, const std::string& eventType,
                         const std::string& jsCode, bool useCapture = false);
    
    // Trigger an event on an element
    bool triggerEvent(html::Element* element, const std::string& eventType);
    
private:
    // QuickJS runtime and context
    JSRuntime* m_runtime;
    JSContext* m_context;
    
    // Initialize DOM bindings for document
    void setupDOMBindings(html::Document* document);
    
    // Bind document object
    void bindDocumentObject(html::Document* document);
    
    // Bind element object
    void bindElementObject(html::Element* element);
    
    // Process any pending JavaScript jobs
    void processPendingJobs();
    
    // Map of elements to their event listeners
    struct EventListener {
        std::string eventType;
        std::string jsCode;
        bool useCapture;
    };
    std::map<html::Element*, std::vector<EventListener>> m_eventListeners;
};

} // namespace javascript
} // namespace browser

#endif // BROWSER_JS_ENGINE_H