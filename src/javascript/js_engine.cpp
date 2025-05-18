#include "js_engine.h"
#include <iostream>
#include <string>

// Include QuickJS headers
extern "C" {
#include "quickjs.h"
}

namespace browser {
namespace javascript {

// Helper function to convert JSValue to std::string
static std::string JSValueToString(JSContext* ctx, JSValue val) {
    const char* str = JS_ToCString(ctx, val);
    std::string result = str ? str : "";
    JS_FreeCString(ctx, str);
    return result;
}

JSEngine::JSEngine()
    : m_runtime(nullptr)
    , m_context(nullptr)
{
}

JSEngine::~JSEngine() {
    // Clean up QuickJS resources
    if (m_context) {
        JS_FreeContext(m_context);
        m_context = nullptr;
    }
    
    if (m_runtime) {
        JS_FreeRuntime(m_runtime);
        m_runtime = nullptr;
    }
}

bool JSEngine::initialize() {
    // Create QuickJS runtime
    m_runtime = JS_NewRuntime();
    if (!m_runtime) {
        std::cerr << "Failed to create QuickJS runtime" << std::endl;
        return false;
    }
    
    // Set memory limit (64MB)
    JS_SetMemoryLimit(m_runtime, 64 * 1024 * 1024);
    
    // Create context
    m_context = JS_NewContext(m_runtime);
    if (!m_context) {
        std::cerr << "Failed to create QuickJS context" << std::endl;
        JS_FreeRuntime(m_runtime);
        m_runtime = nullptr;
        return false;
    }
    
    // Add standard JavaScript objects
    JS_AddIntrinsicBaseObjects(m_context);
    JS_AddIntrinsicDate(m_context);
    JS_AddIntrinsicEval(m_context);
    JS_AddIntrinsicStringNormalize(m_context);
    JS_AddIntrinsicRegExp(m_context);
    JS_AddIntrinsicJSON(m_context);
    JS_AddIntrinsicPromise(m_context);
    JS_AddIntrinsicMapSet(m_context);
    JS_AddIntrinsicTypedArrays(m_context);
    
    return true;
}

bool JSEngine::executeScript(html::Document* document, const std::string& script, 
                          std::string& result, std::string& error) {
    if (!m_context) {
        error = "JavaScript engine not initialized";
        return false;
    }
    
    // Set up DOM bindings for the document
    setupDOMBindings(document);
    
    // Evaluate the script
    JSValue val = JS_Eval(m_context, script.c_str(), script.length(), 
                          "<script>", JS_EVAL_TYPE_GLOBAL);
    
    if (JS_IsException(val)) {
        // Get the exception
        JSValue exceptionVal = JS_GetException(m_context);
        error = JSValueToString(m_context, exceptionVal);
        JS_FreeValue(m_context, exceptionVal);
        return false;
    }
    
    // Get the result as string
    result = JSValueToString(m_context, val);
    
    // Free the result value
    JS_FreeValue(m_context, val);
    
    // Process any pending jobs (promises, etc.)
    processPendingJobs();
    
    return true;
}

void JSEngine::processEventLoop() {
    // Process any pending JavaScript jobs
    processPendingJobs();
    
    // In a real browser, this would also check for timer events,
    // handle network callbacks, etc.
}

bool JSEngine::addEventListener(html::Element* element, const std::string& eventType,
                             const std::string& jsCode, bool useCapture) {
    if (!element) {
        return false;
    }
    
    // Add to event listeners
    EventListener listener = {eventType, jsCode, useCapture};
    m_eventListeners[element].push_back(listener);
    
    return true;
}

bool JSEngine::triggerEvent(html::Element* element, const std::string& eventType) {
    if (!element) {
        return false;
    }
    
    auto it = m_eventListeners.find(element);
    if (it == m_eventListeners.end()) {
        return true; // No listeners for this element
    }
    
    bool handled = false;
    
    // Execute all matching event listeners
    for (const auto& listener : it->second) {
        if (listener.eventType == eventType) {
            // Create event object
            std::string eventScript = "var event = { type: '" + eventType + 
                                    "', target: document.getElementById('" + 
                                    element->id() + "') };\n" + 
                                    listener.jsCode;
            
            std::string result, error;
            executeScript(element->ownerDocument(), eventScript, result, error);
            
            if (!error.empty()) {
                std::cerr << "Error in event handler: " << error << std::endl;
            } else {
                handled = true;
            }
        }
    }
    
    return handled;
}

void JSEngine::setupDOMBindings(html::Document* document) {
    if (!document) {
        return;
    }
    
    // Bind document object
    bindDocumentObject(document);
    
    // Add window global object (same as global object in browsers)
    JSValue globalObj = JS_GetGlobalObject(m_context);
    JS_SetPropertyStr(m_context, globalObj, "window", JS_DupValue(m_context, globalObj));
    
    // Add basic browser API functions
    
    // console.log
    JSValue consoleObj = JS_NewObject(m_context);
    JS_SetPropertyStr(m_context, globalObj, "console", consoleObj);
    
    JSValue logFunc = JS_NewCFunction(m_context, 
                                   [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                                       for (int i = 0; i < argc; i++) {
                                           const char* str = JS_ToCString(ctx, argv[i]);
                                           if (str) {
                                               std::cout << "console.log: " << str << std::endl;
                                               JS_FreeCString(ctx, str);
                                           }
                                       }
                                       return JS_UNDEFINED;
                                   }, "log", 1);
    
    JS_SetPropertyStr(m_context, consoleObj, "log", logFunc);
    
    JS_FreeValue(m_context, globalObj);
}

void JSEngine::bindDocumentObject(html::Document* document) {
    if (!document) {
        return;
    }
    
    JSValue globalObj = JS_GetGlobalObject(m_context);
    JSValue documentObj = JS_NewObject(m_context);
    
    // Add document properties
    JS_SetPropertyStr(m_context, documentObj, "title", 
                     JS_NewString(m_context, document->title().c_str()));
    
    // Add document.getElementById method
    JSValue getByIdFunc = JS_NewCFunction(m_context, 
                               [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                                   if (argc < 1 || !JS_IsString(argv[0])) {
                                       return JS_NULL;
                                   }
                                   
                                   // This is a simplification - in a real implementation,
                                   // we would store a pointer to the document in the document object
                                   // Here, we're assuming the document is available somehow
                                   
                                   // For now, just return a mock element
                                   JSValue elementObj = JS_NewObject(ctx);
                                   JS_SetPropertyStr(ctx, elementObj, "innerHTML", JS_NewString(ctx, "Mock content"));
                                   return elementObj;
                               }, "getElementById", 1);
    
    JS_SetPropertyStr(m_context, documentObj, "getElementById", getByIdFunc);
    
    // Add document to global scope
    JS_SetPropertyStr(m_context, globalObj, "document", documentObj);
    JS_FreeValue(m_context, globalObj);
}

void JSEngine::bindElementObject(html::Element* element) {
    // This would create a JavaScript object for an HTML element
    // In a full implementation, this would expose all DOM properties and methods
}

void JSEngine::processPendingJobs() {
    if (!m_runtime || !m_context) {
        return;
    }
    
    // Process all pending jobs (for promises, etc.)
    JSContext* ctx = nullptr;
    int err;
    
    for (;;) {
        err = JS_ExecutePendingJob(m_runtime, &ctx);
        if (err <= 0) {
            break;
        }
    }
    
    if (err < 0) {
        JSValue exception = JS_GetException(ctx);
        std::string error = JSValueToString(ctx, exception);
        std::cerr << "Error in pending job: " << error << std::endl;
        JS_FreeValue(ctx, exception);
    }
}

} // namespace javascript
} // namespace browser