// js_value.h - Represents JavaScript values in our engine
#ifndef CUSTOM_JS_VALUE_H
#define CUSTOM_JS_VALUE_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <variant>

namespace browser {
namespace custom_js {

// Forward declarations
class JSObject;
class JSFunction;
class JSArray;

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

// JavaScript value class
class JSValue {
public:
    // Constructors for different JS types
    JSValue();  // undefined
    JSValue(std::nullptr_t);  // null
    JSValue(bool value);  // boolean
    JSValue(int value);  // number
    JSValue(double value);  // number
    JSValue(const std::string& value);  // string
    JSValue(const char* value);  // string
    JSValue(std::shared_ptr<JSObject> obj);  // object
    JSValue(std::shared_ptr<JSFunction> func);  // function
    JSValue(std::shared_ptr<JSArray> array);  // array

    // Type checking
    JSValueType type() const;
    bool isUndefined() const;
    bool isNull() const;
    bool isBoolean() const;
    bool isNumber() const;
    bool isString() const;
    bool isObject() const;
    bool isFunction() const;
    bool isArray() const;

    // Value access
    bool toBoolean() const;
    double toNumber() const;
    std::string toString() const;
    std::shared_ptr<JSObject> toObject() const;
    std::shared_ptr<JSFunction> toFunction() const;
    std::shared_ptr<JSArray> toArray() const;

    // Operators
    bool operator==(const JSValue& other) const;
    bool operator!=(const JSValue& other) const;
    
private:
    // We use std::variant to store different possible types
    using ValueVariant = std::variant<
        std::nullptr_t,          // null
        bool,                    // boolean
        double,                  // number
        std::string,             // string
        std::shared_ptr<JSObject>,  // object
        std::shared_ptr<JSFunction>, // function
        std::shared_ptr<JSArray>    // array
    >;
    
    ValueVariant m_value;
    JSValueType m_type;
};

// JavaScript object class
class JSObject {
public:
    JSObject();
    virtual ~JSObject();

    // Property access
    JSValue get(const std::string& key) const;
    void set(const std::string& key, const JSValue& value);
    bool has(const std::string& key) const;
    bool remove(const std::string& key);
    
    // Get all property names
    std::vector<std::string> getPropertyNames() const;

    // Prototype chain
    void setPrototype(std::shared_ptr<JSObject> prototype);
    std::shared_ptr<JSObject> getPrototype() const;

protected:
    std::map<std::string, JSValue> m_properties;
    std::shared_ptr<JSObject> m_prototype;
};

// JavaScript function
class JSFunction : public JSObject {
public:
    using NativeFunction = std::function<JSValue(const std::vector<JSValue>&, JSValue)>;
    
    JSFunction(NativeFunction func);
    virtual ~JSFunction();

    // Call the function
    JSValue call(const std::vector<JSValue>& args, JSValue thisValue);

private:
    NativeFunction m_function;
};

// JavaScript array
class JSArray : public JSObject {
public:
    JSArray();
    JSArray(const std::vector<JSValue>& elements);
    virtual ~JSArray();

    // Array methods
    JSValue get(size_t index) const;
    void set(size_t index, const JSValue& value);
    void push(const JSValue& value);
    JSValue pop();
    size_t length() const;
    
private:
    std::vector<JSValue> m_elements;
};

} // namespace custom_js
} // namespace browser

#endif // CUSTOM_JS_VALUE_H