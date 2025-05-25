#ifndef CUSTOM_JS_ENGINE_H
#define CUSTOM_JS_ENGINE_H

#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace browser {
namespace custom_js {

// Forward declarations
class JSValue;
class JSObject;
class JSFunction;
class JSInterpreter;

// JavaScript engine wrapper class
class JSEngine {
public:
    JSEngine();
    ~JSEngine();
    
    // Initialize the JavaScript engine
    bool initialize();
    
    // Execute a script
    bool executeScript(const std::string& script, std::string& result, std::string& error);
    
    // Add a global function
    void addGlobalFunction(const std::string& name, 
                          std::function<JSValue(const std::vector<JSValue>&, JSValue)> func);
    
    // Add a global object
    void addGlobalObject(const std::string& name, std::shared_ptr<JSObject> object);
    
    // Set a global variable
    void setGlobalVariable(const std::string& name, const JSValue& value);
    
    // Get a global variable
    JSValue getGlobalVariable(const std::string& name, const JSValue& defaultValue);
    
    // Set the document object
    void setDocumentObject(std::shared_ptr<JSObject> documentObject);
    
private:
    // The JavaScript interpreter
    std::shared_ptr<JSInterpreter> m_interpreter;
};

} // namespace custom_js
} // namespace browser

#endif // CUSTOM_JS_ENGINE_H