#include "js_value.h"
#include <iostream>
#include <cmath>

namespace browser {
namespace custom_js {

//-----------------------------------------------------------------------------
// JSValue Implementation
//-----------------------------------------------------------------------------

JSValue::JSValue() : m_type(JSValueType::UNDEFINED) {
    m_value = nullptr;
}

JSValue::JSValue(std::nullptr_t) : m_type(JSValueType::NULL_TYPE) {
    m_value = nullptr;
}

JSValue::JSValue(bool value) : m_type(JSValueType::BOOLEAN) {
    m_value = value;
}

JSValue::JSValue(int value) : m_type(JSValueType::NUMBER) {
    m_value = static_cast<double>(value);
}

JSValue::JSValue(double value) : m_type(JSValueType::NUMBER) {
    m_value = value;
}

JSValue::JSValue(const std::string& value) : m_type(JSValueType::STRING) {
    m_value = value;
}

JSValue::JSValue(const char* value) : m_type(JSValueType::STRING) {
    m_value = std::string(value);
}

JSValue::JSValue(std::shared_ptr<JSObject> obj) : m_type(JSValueType::OBJECT) {
    m_value = obj;
}

JSValue::JSValue(std::shared_ptr<JSFunction> func) : m_type(JSValueType::FUNCTION) {
    m_value = func;
}

JSValue::JSValue(std::shared_ptr<JSArray> array) : m_type(JSValueType::ARRAY) {
    m_value = array;
}

JSValueType JSValue::type() const {
    return m_type;
}

bool JSValue::isUndefined() const {
    return m_type == JSValueType::UNDEFINED;
}

bool JSValue::isNull() const {
    return m_type == JSValueType::NULL_TYPE;
}

bool JSValue::isBoolean() const {
    return m_type == JSValueType::BOOLEAN;
}

bool JSValue::isNumber() const {
    return m_type == JSValueType::NUMBER;
}

bool JSValue::isString() const {
    return m_type == JSValueType::STRING;
}

bool JSValue::isObject() const {
    return m_type == JSValueType::OBJECT;
}

bool JSValue::isFunction() const {
    return m_type == JSValueType::FUNCTION;
}

bool JSValue::isArray() const {
    return m_type == JSValueType::ARRAY;
}

bool JSValue::toBoolean() const {
    switch (m_type) {
        case JSValueType::UNDEFINED:
        case JSValueType::NULL_TYPE:
            return false;
        case JSValueType::BOOLEAN:
            return std::get<bool>(m_value);
        case JSValueType::NUMBER: {
            double num = std::get<double>(m_value);
            return num != 0 && !std::isnan(num);
        }
        case JSValueType::STRING:
            return !std::get<std::string>(m_value).empty();
        case JSValueType::OBJECT:
        case JSValueType::FUNCTION:
        case JSValueType::ARRAY:
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
        case JSValueType::STRING: {
            try {
                return std::stod(std::get<std::string>(m_value));
            } catch (...) {
                return std::numeric_limits<double>::quiet_NaN();
            }
        }
        case JSValueType::OBJECT:
        case JSValueType::FUNCTION:
        case JSValueType::ARRAY:
            return std::numeric_limits<double>::quiet_NaN(); // Should call valueOf() in a full implementation
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
        case JSValueType::NUMBER: {
            double num = std::get<double>(m_value);
            if (std::isnan(num)) return "NaN";
            if (std::isinf(num)) return num > 0 ? "Infinity" : "-Infinity";
            
            // Format number as string
            std::string result = std::to_string(num);
            // Remove trailing zeros
            if (result.find('.') != std::string::npos) {
                result = result.substr(0, result.find_last_not_of('0') + 1);
                if (result.back() == '.') result.pop_back();
            }
            return result;
        }
        case JSValueType::STRING:
            return std::get<std::string>(m_value);
        case JSValueType::OBJECT:
            return "[object Object]";
        case JSValueType::FUNCTION:
            return "[function]";
        case JSValueType::ARRAY:
            return "[object Array]"; // Should use join() in a full implementation
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

bool JSValue::operator==(const JSValue& other) const {
    if (m_type != other.m_type) {
        // Different types, try type conversion
        if (isNumber() && other.isString()) {
            return toNumber() == other.toNumber();
        }
        if (isString() && other.isNumber()) {
            return toNumber() == other.toNumber();
        }
        // For a full implementation, handle more type conversions
        return false;
    }

    // Same type
    switch (m_type) {
        case JSValueType::UNDEFINED:
        case JSValueType::NULL_TYPE:
            return true; // undefined == undefined, null == null
        case JSValueType::BOOLEAN:
            return std::get<bool>(m_value) == std::get<bool>(other.m_value);
        case JSValueType::NUMBER: {
            double a = std::get<double>(m_value);
            double b = std::get<double>(other.m_value);
            if (std::isnan(a) && std::isnan(b)) return true;
            return a == b;
        }
        case JSValueType::STRING:
            return std::get<std::string>(m_value) == std::get<std::string>(other.m_value);
        case JSValueType::OBJECT:
        case JSValueType::FUNCTION:
        case JSValueType::ARRAY:
            // Object equality is by reference
            return std::get<std::shared_ptr<JSObject>>(m_value) == std::get<std::shared_ptr<JSObject>>(other.m_value);
        default:
            return false;
    }
}

bool JSValue::operator!=(const JSValue& other) const {
    return !(*this == other);
}

//-----------------------------------------------------------------------------
// JSObject Implementation
//-----------------------------------------------------------------------------

JSObject::JSObject() {
}

JSObject::~JSObject() {
}

JSValue JSObject::get(const std::string& key) const {
    auto it = m_properties.find(key);
    if (it != m_properties.end()) {
        return it->second;
    }

    // Look up prototype chain
    if (m_prototype) {
        return m_prototype->get(key);
    }

    return JSValue(); // undefined
}

void JSObject::set(const std::string& key, const JSValue& value) {
    m_properties[key] = value;
}

bool JSObject::has(const std::string& key) const {
    if (m_properties.find(key) != m_properties.end()) {
        return true;
    }

    // Look up prototype chain
    if (m_prototype) {
        return m_prototype->has(key);
    }

    return false;
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
    std::vector<std::string> names;
    
    // Get own properties
    for (const auto& pair : m_properties) {
        names.push_back(pair.first);
    }
    
    // Get prototype properties
    if (m_prototype) {
        std::vector<std::string> protoNames = m_prototype->getPropertyNames();
        names.insert(names.end(), protoNames.begin(), protoNames.end());
    }
    
    return names;
}

void JSObject::setPrototype(std::shared_ptr<JSObject> prototype) {
    m_prototype = prototype;
}

std::shared_ptr<JSObject> JSObject::getPrototype() const {
    return m_prototype;
}

//-----------------------------------------------------------------------------
// JSFunction Implementation
//-----------------------------------------------------------------------------

JSFunction::JSFunction(NativeFunction func) : JSObject(), m_function(func) {
}

JSFunction::~JSFunction() {
}

JSValue JSFunction::call(const std::vector<JSValue>& args, JSValue thisValue) {
    return m_function(args, thisValue);
}

//-----------------------------------------------------------------------------
// JSArray Implementation
//-----------------------------------------------------------------------------

JSArray::JSArray() : JSObject() {
    // Set special "length" property
    JSObject::set("length", JSValue(0.0));
}

JSArray::JSArray(const std::vector<JSValue>& elements) : JSObject(), m_elements(elements) {
    // Set length property
    JSObject::set("length", JSValue(static_cast<double>(elements.size())));
    
    // Set indexed properties
    for (size_t i = 0; i < elements.size(); i++) {
        JSObject::set(std::to_string(i), elements[i]);
    }
}

JSArray::~JSArray() {
}

JSValue JSArray::get(size_t index) const {
    if (index < m_elements.size()) {
        return m_elements[index];
    }
    return JSValue(); // undefined
}

void JSArray::set(size_t index, const JSValue& value) {
    if (index >= m_elements.size()) {
        m_elements.resize(index + 1);
    }
    m_elements[index] = value;
    
    // Update length property
    JSObject::set("length", JSValue(static_cast<double>(m_elements.size())));
}

void JSArray::push(const JSValue& value) {
    m_elements.push_back(value);
    
    // Update length property
    JSObject::set("length", JSValue(static_cast<double>(m_elements.size())));
    
    // Set indexed property
    set(m_elements.size() - 1, value);
}

JSValue JSArray::pop() {
    if (m_elements.empty()) {
        return JSValue(); // undefined
    }
    
    JSValue last = m_elements.back();
    m_elements.pop_back();
    
    // Update length property
    JSObject::set("length", JSValue(static_cast<double>(m_elements.size())));
    
    return last;
}

size_t JSArray::length() const {
    return m_elements.size();
}

} // namespace custom_js
} // namespace browser