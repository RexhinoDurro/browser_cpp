#include "js_engine.h"
#include <iostream>
#include <sstream>

namespace browser {
namespace custom_js {

JSEngine::JSEngine() {
}

JSEngine::~JSEngine() {
}

bool JSEngine::initialize() {
    // Create interpreter
    m_interpreter = std::make_shared<JSInterpreter>();
    
    // Create global window object
    auto windowObj = std::make_shared<JSObject>();
    
    // Add standard global objects
    windowObj->set("undefined", JSValue());
    windowObj->set("NaN", JSValue(std::numeric_limits<double>::quiet_NaN()));
    windowObj->set("Infinity", JSValue(std::numeric_limits<double>::infinity()));
    
    // Add window self-reference
    windowObj->set("window", JSValue(windowObj));
    windowObj->set("self", JSValue(windowObj));
    windowObj->set("global", JSValue(windowObj));
    
    // Add window to global environment
    m_interpreter->defineGlobalVariable("window", JSValue(windowObj));
    
    // Add global Math object
    auto mathObj = std::make_shared<JSObject>();
    mathObj->set("PI", JSValue(3.14159265358979323846));
    mathObj->set("E", JSValue(2.7182818284590452354));
    
    // Math functions
    mathObj->set("abs", JSValue(std::make_shared<JSFunction>([](const std::vector<JSValue>& args, JSValue) {
        if (args.empty()) return JSValue(std::numeric_limits<double>::quiet_NaN());
        return JSValue(std::abs(args[0].toNumber()));
    })));
    
    mathObj->set("sqrt", JSValue(std::make_shared<JSFunction>([](const std::vector<JSValue>& args, JSValue) {
        if (args.empty()) return JSValue(std::numeric_limits<double>::quiet_NaN());
        return JSValue(std::sqrt(args[0].toNumber()));
    })));
    
    mathObj->set("min", JSValue(std::make_shared<JSFunction>([](const std::vector<JSValue>& args, JSValue) {
        if (args.empty()) return JSValue(std::numeric_limits<double>::infinity());
        double result = args[0].toNumber();
        for (size_t i = 1; i < args.size(); i++) {
            result = std::min(result, args[i].toNumber());
        }
        return JSValue(result);
    })));
    
    mathObj->set("max", JSValue(std::make_shared<JSFunction>([](const std::vector<JSValue>& args, JSValue) {
        if (args.empty()) return JSValue(-std::numeric_limits<double>::infinity());
        double result = args[0].toNumber();
        for (size_t i = 1; i < args.size(); i++) {
            result = std::max(result, args[i].toNumber());
        }
        return JSValue(result);
    })));
    
    mathObj->set("random", JSValue(std::make_shared<JSFunction>([](const std::vector<JSValue>& args, JSValue) {
        return JSValue(static_cast<double>(std::rand()) / RAND_MAX);
    })));
    
    // Add Math to global environment
    m_interpreter->defineGlobalVariable("Math", JSValue(mathObj));
    
    // Set up Array constructor
    auto arrayConstructor = std::make_shared<JSFunction>([](const std::vector<JSValue>& args, JSValue) {
        return JSValue(std::make_shared<JSArray>(args));
    });
    
    // Set up Object constructor
    auto objectConstructor = std::make_shared<JSFunction>([](const std::vector<JSValue>& args, JSValue) {
        return JSValue(std::make_shared<JSObject>());
    });
    
    // Add constructors to global environment
    m_interpreter->defineGlobalVariable("Array", JSValue(arrayConstructor));
    m_interpreter->defineGlobalVariable("Object", JSValue(objectConstructor));
    
    // Add standard global functions
    addGlobalFunction("parseInt", [](const std::vector<JSValue>& args, JSValue) {
        if (args.empty()) return JSValue(std::numeric_limits<double>::quiet_NaN());
        std::string str = args[0].toString();
        int radix = args.size() > 1 ? static_cast<int>(args[1].toNumber()) : 10;
        
        try {
            return JSValue(static_cast<double>(std::stoi(str, nullptr, radix)));
        } catch (...) {
            return JSValue(std::numeric_limits<double>::quiet_NaN());
        }
    });
    
    addGlobalFunction("parseFloat", [](const std::vector<JSValue>& args, JSValue) {
        if (args.empty()) return JSValue(std::numeric_limits<double>::quiet_NaN());
        std::string str = args[0].toString();
        
        try {
            return JSValue(std::stod(str));
        } catch (...) {
            return JSValue(std::numeric_limits<double>::quiet_NaN());
        }
    });
    
    addGlobalFunction("isNaN", [](const std::vector<JSValue>& args, JSValue) {
        if (args.empty()) return JSValue(true);
        double num = args[0].toNumber();
        return JSValue(std::isnan(num));
    });
    
    addGlobalFunction("isFinite", [](const std::vector<JSValue>& args, JSValue) {
        if (args.empty()) return JSValue(false);
        double num = args[0].toNumber();
        return JSValue(std::isfinite(num));
    });
    
    return true;
}

bool JSEngine::executeScript(const std::string& script, std::string& result, std::string& error) {
    try {
        JSValue returnValue = m_interpreter->execute(script);
        result = returnValue.toString();
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

void JSEngine::addGlobalFunction(const std::string& name, JSFunction::NativeFunction func) {
    m_interpreter->defineGlobalFunction(name, func);
}

void JSEngine::addGlobalObject(const std::string& name, std::shared_ptr<JSObject> object) {
    m_interpreter->defineGlobalVariable(name, JSValue(object));
}

void JSEngine::setGlobalVariable(const std::string& name, const JSValue& value) {
    m_interpreter->defineGlobalVariable(name, value);
}

JSValue JSEngine::getGlobalVariable(const std::string& name, const JSValue& defaultValue) {
    try {
        return m_interpreter->global()->get(name);
    } catch (...) {
        return defaultValue;
    }
}

void JSEngine::setDocumentObject(std::shared_ptr<JSObject> documentObject) {
    try {
        JSValue windowValue = m_interpreter->global()->get("window");
        if (windowValue.isObject()) {
            std::shared_ptr<JSObject> window = windowValue.toObject();
            window->set("document", JSValue(documentObject));
        }
    } catch (...) {
        // Ignore errors
    }
}

} // namespace custom_js
} // namespace browser