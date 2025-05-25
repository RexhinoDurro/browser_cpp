// js_engine.cpp - Fixed implementation
#include "js_engine.h"
#include "js_value.h"  // Make sure this is included
#include <iostream>
#include <sstream>
#include <limits>  // for std::numeric_limits
#include <cmath>   // for std::isnan

namespace browser {
namespace custom_js {

// JSValue Implementation
JSValue::JSValue() : m_type(JSValueType::UNDEFINED) {
    m_value = nullptr;
}

JSValue::JSValue(std::nullptr_t) : m_type(JSValueType::NULL_TYPE) {
    m_value = nullptr;
}

JSValue::JSValue(bool value) : m_type(JSValueType::BOOLEAN) {
    m_value = value;
}

JSValue::JSValue(double value) : m_type(JSValueType::NUMBER) {
    m_value = value;
}

JSValue::JSValue(const std::string& value) : m_type(JSValueType::STRING) {
    m_value = value;
}

JSValue::JSValue(std::shared_ptr<JSObject> object) : m_type(JSValueType::OBJECT) {
    m_value = object;
}

JSValue::JSValue(std::shared_ptr<JSFunction> function) : m_type(JSValueType::FUNCTION) {
    m_value = function;
}

JSValue::JSValue(std::shared_ptr<JSArray> array) : m_type(JSValueType::ARRAY) {
    m_value = array;
}

// Type conversions
bool JSValue::toBoolean() const {
    switch (m_type) {
        case JSValueType::UNDEFINED:
        case JSValueType::NULL_TYPE:
            return false;
        case JSValueType::BOOLEAN:
            return std::get<bool>(m_value);
        case JSValueType::NUMBER: {
            double num = std::get<double>(m_value);
            return num != 0.0 && !std::isnan(num);
        }
        case JSValueType::STRING:
            return !std::get<std::string>(m_value).empty();
        case JSValueType::OBJECT:
        case JSValueType::FUNCTION:
            return true;
        default:
            return false;
    }
}

double JSValue::toNumber() const {
    switch (m_type) {
        case JSValueType::UNDEFINED:
            return std::numeric_limits<double>::quiet_NaN();
        case JSValueType::NULL_TYPE:
            return 0.0;
        case JSValueType::BOOLEAN:
            return std::get<bool>(m_value) ? 1.0 : 0.0;
        case JSValueType::NUMBER:
            return std::get<double>(m_value);
        case JSValueType::STRING:
            try {
                return std::stod(std::get<std::string>(m_value));
            } catch (...) {
                return std::numeric_limits<double>::quiet_NaN();
            }
        default:
            return std::numeric_limits<double>::quiet_NaN();
    }
}

std::string JSValue::toString() const {
    switch (m_type) {
        case JSValueType::UNDEFINED:
            return "undefined";
        case JSValueType::NULL_TYPE:
            return "null";
        case JSValueType::BOOLEAN:
            return std::get<bool>(m_value) ? "true" : "false";
        case JSValueType::NUMBER:
            return std::to_string(std::get<double>(m_value));
        case JSValueType::STRING:
            return std::get<std::string>(m_value);
        case JSValueType::OBJECT:
            return "[object Object]";
        case JSValueType::FUNCTION:
            return "[object Function]";
        default:
            return "";
    }
}

std::shared_ptr<JSObject> JSValue::toObject() const {
    if (m_type == JSValueType::OBJECT) {
        return std::get<std::shared_ptr<JSObject>>(m_value);
    }
    return nullptr;
}

std::shared_ptr<JSFunction> JSValue::toFunction() const {
    if (m_type == JSValueType::FUNCTION) {
        return std::get<std::shared_ptr<JSFunction>>(m_value);
    }
    return nullptr;
}

std::shared_ptr<JSArray> JSValue::toArray() const {
    if (m_type == JSValueType::ARRAY) {
        return std::get<std::shared_ptr<JSArray>>(m_value);
    }
    return nullptr;
}

// JSObject Implementation
JSObject::JSObject() {
}

JSObject::~JSObject() {
}

JSValue JSObject::get(const std::string& key) const {
    auto it = m_properties.find(key);
    if (it != m_properties.end()) {
        return it->second;
    }
    return JSValue(); // undefined
}

void JSObject::set(const std::string& key, const JSValue& value) {
    m_properties[key] = value;
}

bool JSObject::has(const std::string& key) const {
    return m_properties.find(key) != m_properties.end();
}

bool JSObject::remove(const std::string& key) {
    auto it = m_properties.find(key);
    if (it != m_properties.end()) {
        m_properties.erase(it);
        return true;
    }
    return false;
}

std::vector<std::string> JSObject::getPropertyNames() const {
    std::vector<std::string> result;
    for (const auto& pair : m_properties) {
        result.push_back(pair.first);
    }
    return result;
}

// JSFunction Implementation
JSFunction::JSFunction(NativeFunction func) : m_function(func) {
}

JSFunction::~JSFunction() {
}

JSValue JSFunction::call(const std::vector<JSValue>& args, JSValue thisValue) {
    if (m_function) {
        return m_function(args, thisValue);
    }
    return JSValue(); // undefined
}

// JSArray Implementation
JSArray::JSArray() : JSObject() {
    JSObject::set("length", JSValue(0.0));
}

JSArray::~JSArray() {
}

size_t JSArray::length() const {
    JSValue lengthValue = JSObject::get("length");
    return static_cast<size_t>(lengthValue.toNumber());
}

void JSArray::push(const JSValue& value) {
    size_t currentLength = length();
    JSObject::set(std::to_string(currentLength), value);
    JSObject::set("length", JSValue(static_cast<double>(currentLength + 1)));
}

JSValue JSArray::pop() {
    size_t currentLength = length();
    if (currentLength == 0) {
        return JSValue(); // undefined
    }
    
    size_t lastIndex = currentLength - 1;
    JSValue lastValue = JSObject::get(std::to_string(lastIndex));
    JSObject::remove(std::to_string(lastIndex));
    JSObject::set("length", JSValue(static_cast<double>(lastIndex)));
    
    return lastValue;
}

JSValue JSArray::get(size_t index) const {
    return JSObject::get(std::to_string(index));
}

void JSArray::set(size_t index, const JSValue& value) {
    JSObject::set(std::to_string(index), value);
    
    // Update length if necessary
    size_t currentLength = length();
    if (index >= currentLength) {
        JSObject::set("length", JSValue(static_cast<double>(index + 1)));
    }
}

// JSEngine Implementation
JSEngine::JSEngine() : m_interpreter(nullptr) {
}

JSEngine::~JSEngine() {
}

bool JSEngine::initialize() {
    // Initialize the interpreter
    m_interpreter = std::make_unique<JSInterpreter>();
    
    // Create global object
    m_globalObject = std::make_shared<JSObject>();
    
    // Add built-in functions
    addBuiltinFunctions();
    
    return true;
}

bool JSEngine::executeScript(const std::string& script, std::string& result, std::string& error) {
    if (!m_interpreter) {
        error = "JavaScript engine not initialized";
        return false;
    }
    
    try {
        // In a real implementation, this would:
        // 1. Tokenize the script
        // 2. Parse into AST
        // 3. Execute the AST
        
        // For now, just a placeholder
        result = "Script executed successfully";
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

void JSEngine::defineGlobalVariable(const std::string& name, const JSValue& value) {
    if (m_globalObject) {
        m_globalObject->set(name, value);
    }
}

void JSEngine::addBuiltinFunctions() {
    // Add parseInt
    m_globalObject->set("parseInt", JSValue(
        std::make_shared<JSFunction>(
            [](const std::vector<JSValue>& args, JSValue thisValue) {
                if (args.empty()) return JSValue(std::numeric_limits<double>::quiet_NaN());
                
                std::string str = args[0].toString();
                int base = 10;
                
                if (args.size() > 1) {
                    base = static_cast<int>(args[1].toNumber());
                }
                
                try {
                    return JSValue(static_cast<double>(std::stoi(str, nullptr, base)));
                } catch (...) {
                    return JSValue(std::numeric_limits<double>::quiet_NaN());
                }
            }
        )
    ));
    
    // Add parseFloat
    m_globalObject->set("parseFloat", JSValue(
        std::make_shared<JSFunction>(
            [](const std::vector<JSValue>& args, JSValue thisValue) {
                if (args.empty()) return JSValue(std::numeric_limits<double>::quiet_NaN());
                
                try {
                    return JSValue(std::stod(args[0].toString()));
                } catch (...) {
                    return JSValue(std::numeric_limits<double>::quiet_NaN());
                }
            }
        )
    ));
}

} // namespace custom_js
} // namespace browser