// js_engine.cpp - Proper structure
#include "js_engine.h"
#include "js_value.h"  // Make sure this is included
#include <iostream>
#include <sstream>

namespace browser {
namespace custom_js {

// JSValue Implementation
JSValue::JSValue() : m_type(JSValueType::UNDEFINED) {
    // Initialize as undefined
}

JSValue::JSValue(bool value) : m_type(JSValueType::BOOLEAN), m_boolean(value) {
}

JSValue::JSValue(double value) : m_type(JSValueType::NUMBER), m_number(value) {
}

JSValue::JSValue(const std::string& value) : m_type(JSValueType::STRING), m_string(value) {
}

JSValue::JSValue(std::shared_ptr<JSObject> object) : m_type(JSValueType::OBJECT), m_object(object) {
}

JSValue::JSValue(std::shared_ptr<JSFunction> function) : m_type(JSValueType::FUNCTION), m_function(function) {
}

// Type conversions
bool JSValue::toBoolean() const {
    switch (m_type) {
        case JSValueType::UNDEFINED:
        case JSValueType::NULL_TYPE:
            return false;
        case JSValueType::BOOLEAN:
            return m_boolean;
        case JSValueType::NUMBER:
            return m_number != 0.0 && !std::isnan(m_number);
        case JSValueType::STRING:
            return !m_string.empty();
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
            return m_boolean ? 1.0 : 0.0;
        case JSValueType::NUMBER:
            return m_number;
        case JSValueType::STRING:
            try {
                return std::stod(m_string);
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
            return m_boolean ? "true" : "false";
        case JSValueType::NUMBER:
            return std::to_string(m_number);
        case JSValueType::STRING:
            return m_string;
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
        return m_object;
    }
    return nullptr;
}

std::shared_ptr<JSFunction> JSValue::toFunction() const {
    if (m_type == JSValueType::FUNCTION) {
        return m_function;
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

void JSObject::remove(const std::string& key) {
    m_properties.erase(key);
}

std::vector<std::string> JSObject::keys() const {
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