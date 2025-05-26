// js_engine.cpp - Fixed implementation
#include "js_engine.h"
#include "js_value.h"  // Include the actual JSValue definitions
#include "js_interpreter.h"
#include <iostream>
#include <sstream>
#include <limits>   // For std::numeric_limits
#include <cmath>    // For std::isnan, std::isinf

namespace browser {
namespace custom_js {

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
        // Execute the script using the interpreter
        JSValue resultValue = m_interpreter->execute(script);
        result = resultValue.toString();
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
    
    // Also define in interpreter's global environment
    if (m_interpreter) {
        m_interpreter->defineGlobalVariable(name, value);
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
    
    // Add isNaN
    m_globalObject->set("isNaN", JSValue(
        std::make_shared<JSFunction>(
            [](const std::vector<JSValue>& args, JSValue thisValue) {
                if (args.empty()) return JSValue(true);
                double num = args[0].toNumber();
                return JSValue(std::isnan(num));
            }
        )
    ));
    
    // Add isFinite
    m_globalObject->set("isFinite", JSValue(
        std::make_shared<JSFunction>(
            [](const std::vector<JSValue>& args, JSValue thisValue) {
                if (args.empty()) return JSValue(false);
                double num = args[0].toNumber();
                return JSValue(!std::isnan(num) && !std::isinf(num));
            }
        )
    ));
    
    // Define these built-in functions in the interpreter as well
    if (m_interpreter) {
        m_interpreter->defineGlobalVariable("parseInt", m_globalObject->get("parseInt"));
        m_interpreter->defineGlobalVariable("parseFloat", m_globalObject->get("parseFloat"));
        m_interpreter->defineGlobalVariable("isNaN", m_globalObject->get("isNaN"));
        m_interpreter->defineGlobalVariable("isFinite", m_globalObject->get("isFinite"));
    }
}

} // namespace custom_js
} // namespace browser