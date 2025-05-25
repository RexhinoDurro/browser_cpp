#ifndef BROWSER_JS_ENGINE_H
#define BROWSER_JS_ENGINE_H

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <functional>

namespace browser {
namespace custom_js {

// Forward declarations
class JSObject;
class JSFunction;
class JSInterpreter;

// JavaScript value types
enum class JSValueType {
    UNDEFINED,
    NULL_TYPE,
    BOOLEAN,
    NUMBER,
    STRING,
    OBJECT,
    FUNCTION,
    ARRAY
};

// JavaScript Value class
class JSValue {
public:
    JSValue();  // Creates undefined value
    JSValue(bool value);
    JSValue(double value);
    JSValue(const std::string& value);
    JSValue(std::shared_ptr<JSObject> object);
    JSValue(std::shared_ptr<JSFunction> function);
    
    // Type checking
    JSValueType type() const { return m_type; }
    bool isUndefined() const { return m_type == JSValueType::UNDEFINED; }
    bool isNull() const { return m_type == JSValueType::NULL_TYPE; }
    bool isBoolean() const { return m_type == JSValueType::BOOLEAN; }
    bool isNumber() const { return m_type == JSValueType::NUMBER; }
    bool isString() const { return m_type == JSValueType::STRING; }
    bool isObject() const { return m_type == JSValueType::OBJECT; }
    bool isFunction() const { return m_type == JSValueType::FUNCTION; }
    
    // Type conversions
    bool toBoolean() const;
    double toNumber() const;
    std::string toString() const;
    std::shared_ptr<JSObject> toObject() const;
    std::shared_ptr<JSFunction> toFunction() const;
    
private:
    JSValueType m_type;
    
    // Value storage
    union {
        bool m_boolean;
        double m_number;
    };
    std::string m_string;
    std::shared_ptr<JSObject> m_object;
    std::shared_ptr<JSFunction> m_function;
};

// JavaScript Object
class JSObject {
public:
    JSObject();
    virtual ~JSObject();
    
    // Property access
    JSValue get(const std::string& key) const;
    void set(const std::string& key, const JSValue& value);
    bool has(const std::string& key) const;
    void remove(const std::string& key);
    
    // Get all property names
    std::vector<std::string> keys() const;
    
private:
    std::map<std::string, JSValue> m_properties;
};

// JavaScript Function
class JSFunction {
public:
    using NativeFunction = std::function<JSValue(const std::vector<JSValue>&, JSValue)>;
    
    JSFunction(NativeFunction func);
    virtual ~JSFunction();
    
    // Call the function
    JSValue call(const std::vector<JSValue>& args, JSValue thisValue = JSValue());
    
private:
    NativeFunction m_function;
};

// JavaScript Array (simplified - can inherit from JSObject)
class JSArray : public JSObject {
public:
    JSArray();
    virtual ~JSArray();
    
    // Array-specific methods
    size_t length() const;
    void push(const JSValue& value);
    JSValue pop();
    JSValue at(size_t index) const;
    void setAt(size_t index, const JSValue& value);
};

// JavaScript Interpreter (placeholder)
class JSInterpreter {
public:
    JSInterpreter();
    ~JSInterpreter();
    
    // Execute JavaScript code
    JSValue execute(const std::string& code);
};

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