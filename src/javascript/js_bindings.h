#ifndef BROWSER_JS_BINDINGS_H
#define BROWSER_JS_BINDINGS_H

#include "../html/dom_tree.h"

// Forward declare QuickJS structs
struct JSContext;
struct JSValue;

namespace browser {
namespace javascript {

// Class to handle DOM bindings for JavaScript
class DOMBindings {
public:
    // Initialize bindings
    static void initialize(JSContext* ctx);
    
    // Bind document object
    static JSValue bindDocument(JSContext* ctx, html::Document* document);
    
    // Bind element object
    static JSValue bindElement(JSContext* ctx, html::Element* element);
    
    // Helper to get HTML element from JS value
    static html::Element* getElementFromJSValue(JSContext* ctx, JSValue val);

private:
    // Binding helpers
    static void bindDocumentMethods(JSContext* ctx, JSValue obj, html::Document* document);
    static void bindElementMethods(JSContext* ctx, JSValue obj, html::Element* element);
    static void bindWindowMethods(JSContext* ctx, JSValue obj);
    
    // Native function implementations for document methods
    static JSValue js_document_getElementById(JSContext* ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_document_querySelector(JSContext* ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_document_createElement(JSContext* ctx, JSValue this_val, int argc, JSValue* argv);
    
    // Native function implementations for element methods
    static JSValue js_element_getAttribute(JSContext* ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_element_setAttribute(JSContext* ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_element_addEventListener(JSContext* ctx, JSValue this_val, int argc, JSValue* argv);
    
    // Native function implementations for window methods
    static JSValue js_window_setTimeout(JSContext* ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_window_setInterval(JSContext* ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_window_alert(JSContext* ctx, JSValue this_val, int argc, JSValue* argv);
};

} // namespace javascript
} // namespace browser

#endif // BROWSER_JS_BINDINGS_H