#ifndef BROWSER_JS_ENGINE_H
#define BROWSER_JS_ENGINE_H

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include "js_value.h"  // Include the actual JSValue definitions

namespace browser {
namespace custom_js {

// Forward declaration
class JSInterpreter;

// JavaScript Engine
class JSEngine {
public:
    JSEngine();
    ~JSEngine();
    
    // Initialize the engine
    bool initialize();
    
    // Execute JavaScript code
    bool executeScript(const std::string& script, std::string& result, std::string& error);
    
    // Define global variables
    void defineGlobalVariable(const std::string& name, const JSValue& value);
    
    // Get global object
    std::shared_ptr<JSObject> globalObject() const { return m_globalObject; }
    
private:
    std::unique_ptr<JSInterpreter> m_interpreter;
    std::shared_ptr<JSObject> m_globalObject;
    
    // Add built-in functions
    void addBuiltinFunctions();
};

} // namespace custom_js
} // namespace browser

#endif // BROWSER_JS_ENGINE_H