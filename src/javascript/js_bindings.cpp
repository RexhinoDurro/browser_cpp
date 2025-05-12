#include "js_bindings.h"
#include <iostream>

// Include QuickJS headers
extern "C" {
#include "quickjs.h"
}

namespace browser {
namespace javascript {

// Initialize DOM bindings
void DOMBindings::initialize(JSContext* ctx) {
    // Register class definitions, prototypes, etc.
    // This would be expanded in a full implementation
}

// Bind document object
JSValue DOMBindings::bindDocument(JSContext* ctx, html::Document* document) {
    if (!document) {
        return JS_NULL;
    }
    
    // Create document object
    JSValue documentObj = JS_NewObject(ctx);
    
    // Store a pointer to the document in the object
    // This allows retrieving the document in DOM API functions
    JS_SetOpaque(documentObj, document);
    
    // Add document properties
    JS_SetPropertyStr(ctx, documentObj, "title", 
                    JS_NewString(ctx, document->title().c_str()));
    
    // Bind document methods
    bindDocumentMethods(ctx, documentObj, document);
    
    return documentObj;
}

// Bind element object
JSValue DOMBindings::bindElement(JSContext* ctx, html::Element* element) {
    if (!element) {
        return JS_NULL;
    }
    
    // Create element object
    JSValue elementObj = JS_NewObject(ctx);
    
    // Store a pointer to the element in the object
    JS_SetOpaque(elementObj, element);
    
    // Add element properties
    JS_SetPropertyStr(ctx, elementObj, "tagName", 
                    JS_NewString(ctx, element->tagName().c_str()));
    JS_SetPropertyStr(ctx, elementObj, "id", 
                    JS_NewString(ctx, element->id().c_str()));
    JS_SetPropertyStr(ctx, elementObj, "className", 
                    JS_NewString(ctx, element->className().c_str()));
    JS_SetPropertyStr(ctx, elementObj, "innerHTML", 
                    JS_NewString(ctx, element->innerHTML().c_str()));
    
    // Bind element methods
    bindElementMethods(ctx, elementObj, element);
    
    return elementObj;
}

// Helper to get HTML element from JS value
html::Element* DOMBindings::getElementFromJSValue(JSContext* ctx, JSValue val) {
    return static_cast<html::Element*>(JS_GetOpaque(val, 0));
}

// Bind document methods
void DOMBindings::bindDocumentMethods(JSContext* ctx, JSValue obj, html::Document* document) {
    // Add document methods
    JS_SetPropertyStr(ctx, obj, "getElementById", 
                    JS_NewCFunction(ctx, js_document_getElementById, "getElementById", 1));
    JS_SetPropertyStr(ctx, obj, "querySelector", 
                    JS_NewCFunction(ctx, js_document_querySelector, "querySelector", 1));
    JS_SetPropertyStr(ctx, obj, "createElement", 
                    JS_NewCFunction(ctx, js_document_createElement, "createElement", 1));
}

// Bind element methods
void DOMBindings::bindElementMethods(JSContext* ctx, JSValue obj, html::Element* element) {
    // Add element methods
    JS_SetPropertyStr(ctx, obj, "getAttribute", 
                    JS_NewCFunction(ctx, js_element_getAttribute, "getAttribute", 1));
    JS_SetPropertyStr(ctx, obj, "setAttribute", 
                    JS_NewCFunction(ctx, js_element_setAttribute, "setAttribute", 2));
    JS_SetPropertyStr(ctx, obj, "addEventListener", 
                    JS_NewCFunction(ctx, js_element_addEventListener, "addEventListener", 2));
}

// Bind window methods
void DOMBindings::bindWindowMethods(JSContext* ctx, JSValue obj) {
    // Add window methods
    JS_SetPropertyStr(ctx, obj, "setTimeout", 
                    JS_NewCFunction(ctx, js_window_setTimeout, "setTimeout", 2));
    JS_SetPropertyStr(ctx, obj, "setInterval", 
                    JS_NewCFunction(ctx, js_window_setInterval, "setInterval", 2));
    JS_SetPropertyStr(ctx, obj, "alert", 
                    JS_NewCFunction(ctx, js_window_alert, "alert", 1));
}

// Implementation of document.getElementById
JSValue DOMBindings::js_document_getElementById(JSContext* ctx, JSValue this_val, 
                                             int argc, JSValue* argv) {
    if (argc < 1 || !JS_IsString(argv[0])) {
        return JS_NULL;
    }
    
    // Get document from 'this'
    html::Document* document = static_cast<html::Document*>(JS_GetOpaque(this_val, 0));
    if (!document) {
        return JS_NULL;
    }
    
    // Get element ID
    const char* id = JS_ToCString(ctx, argv[0]);
    if (!id) {
        return JS_NULL;
    }
    
    // Find element
    html::Element* element = document->getElementById(id);
    JS_FreeCString(ctx, id);
    
    if (!element) {
        return JS_NULL;
    }
    
    // Bind and return element
    return bindElement(ctx, element);
}

// Implementation of document.querySelector
JSValue DOMBindings::js_document_querySelector(JSContext* ctx, JSValue this_val, 
                                            int argc, JSValue* argv) {
    if (argc < 1 || !JS_IsString(argv[0])) {
        return JS_NULL;
    }
    
    // Get document from 'this'
    html::Document* document = static_cast<html::Document*>(JS_GetOpaque(this_val, 0));
    if (!document) {
        return JS_NULL;
    }
    
    // Get selector
    const char* selector = JS_ToCString(ctx, argv[0]);
    if (!selector) {
        return JS_NULL;
    }
    
    // Find element
    html::Element* element = document->querySelector(selector);
    JS_FreeCString(ctx, selector);
    
    if (!element) {
        return JS_NULL;
    }
    
    // Bind and return element
    return bindElement(ctx, element);
}

// Implementation of document.createElement
JSValue DOMBindings::js_document_createElement(JSContext* ctx, JSValue this_val, 
                                            int argc, JSValue* argv) {
    if (argc < 1 || !JS_IsString(argv[0])) {
        return JS_ThrowTypeError(ctx, "createElement requires a string argument");
    }
    
    // Get document from 'this'
    html::Document* document = static_cast<html::Document*>(JS_GetOpaque(this_val, 0));
    if (!document) {
        return JS_NULL;
    }
    
    // Get tag name
    const char* tagName = JS_ToCString(ctx, argv[0]);
    if (!tagName) {
        return JS_NULL;
    }
    
    // Create element
    auto element = document->createElement(tagName);
    JS_FreeCString(ctx, tagName);
    
    if (!element) {
        return JS_NULL;
    }
    
    // Bind and return element
    return bindElement(ctx, element.get());
}

// Implementation of element.getAttribute
JSValue DOMBindings::js_element_getAttribute(JSContext* ctx, JSValue this_val, 
                                          int argc, JSValue* argv) {
    if (argc < 1 || !JS_IsString(argv[0])) {
        return JS_NULL;
    }
    
    // Get element from 'this'
    html::Element* element = getElementFromJSValue(ctx, this_val);
    if (!element) {
        return JS_NULL;
    }
    
    // Get attribute name
    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name) {
        return JS_NULL;
    }
    
    // Get attribute value
    std::string value = element->getAttribute(name);
    JS_FreeCString(ctx, name);
    
    return JS_NewString(ctx, value.c_str());
}

// Implementation of element.setAttribute
JSValue DOMBindings::js_element_setAttribute(JSContext* ctx, JSValue this_val, 
                                          int argc, JSValue* argv) {
    if (argc < 2 || !JS_IsString(argv[0]) || !JS_IsString(argv[1])) {
        return JS_ThrowTypeError(ctx, "setAttribute requires name and value strings");
    }
    
    // Get element from 'this'
    html::Element* element = getElementFromJSValue(ctx, this_val);
    if (!element) {
        return JS_UNDEFINED;
    }
    
    // Get name and value
    const char* name = JS_ToCString(ctx, argv[0]);
    const char* value = JS_ToCString(ctx, argv[1]);
    
    if (name && value) {
        element->setAttribute(name, value);
    }
    
    JS_FreeCString(ctx, name);
    JS_FreeCString(ctx, value);
    
    return JS_UNDEFINED;
}

// Implementation of element.addEventListener
JSValue DOMBindings::js_element_addEventListener(JSContext* ctx, JSValue this_val, 
                                              int argc, JSValue* argv) {
    // This is a placeholder - a real implementation would need to handle
    // storing event listeners and callbacks
    return JS_UNDEFINED;
}

// Implementation of window.setTimeout
JSValue DOMBindings::js_window_setTimeout(JSContext* ctx, JSValue this_val, 
                                       int argc, JSValue* argv) {
    // This is a placeholder - a real implementation would need to handle
    // storing callback functions and scheduling their execution
    return JS_NewInt32(ctx, 1);
}

// Implementation of window.setInterval
JSValue DOMBindings::js_window_setInterval(JSContext* ctx, JSValue this_val, 
                                        int argc, JSValue* argv) {
    // This is a placeholder - a real implementation would need to handle
    // storing callback functions and scheduling their repeated execution
    return JS_NewInt32(ctx, 1);
}

// Implementation of window.alert
JSValue DOMBindings::js_window_alert(JSContext* ctx, JSValue this_val, 
                                  int argc, JSValue* argv) {
    if (argc < 1) {
        std::cout << "Alert: undefined" << std::endl;
        return JS_UNDEFINED;
    }
    
    const char* message = JS_ToCString(ctx, argv[0]);
    if (message) {
        std::cout << "Alert: " << message << std::endl;
        JS_FreeCString(ctx, message);
    }
    
    return JS_UNDEFINED;
}

} // namespace javascript
} // namespace browser