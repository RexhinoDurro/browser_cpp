// src/rendering/render_target.h - Base render target class
#ifndef BROWSER_RENDERING_RENDER_TARGET_H
#define BROWSER_RENDERING_RENDER_TARGET_H

namespace browser {
namespace rendering {

// Forward declarations
class RenderingContext;

// Render target types
enum class RenderTargetType {
    CONSOLE,
    BITMAP
};

// Base render target interface
class RenderTarget {
public:
    virtual ~RenderTarget() = default;
    
    virtual RenderingContext* context() = 0;
    virtual RenderTargetType type() const = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
};

// Base rendering context interface
class RenderingContext {
public:
    virtual ~RenderingContext() = default;
    // Add common rendering context methods here as needed
};

} // namespace rendering
} // namespace browser

#endif // BROWSER_RENDERING_RENDER_TARGET_H

// src/rendering/custom_renderer.h - Custom renderer classes
#ifndef BROWSER_RENDERING_CUSTOM_RENDERER_H
#define BROWSER_RENDERING_CUSTOM_RENDERER_H

#include <string>
#include <vector>
#include <stack>

namespace browser {
namespace rendering {

// Color class
class Color {
public:
    unsigned char r, g, b;
    float a;
    
    Color() : r(0), g(0), b(0), a(1.0f) {}
    Color(unsigned char red, unsigned char green, unsigned char blue, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
};

// Paint types
enum class PaintType {
    COLOR,
    LINEAR_GRADIENT,
    RADIAL_GRADIENT
};

// Paint class
class Paint {
public:
    PaintType type;
    Color color;
    
    Paint() : type(PaintType::COLOR) {}
    void setColor(const Color& c) { color = c; type = PaintType::COLOR; }
};

// Font class
class Font {
public:
    std::string name;
    float size;
    
    Font() : name("Arial"), size(12.0f) {}
    Font(const std::string& fontName, float fontSize) : name(fontName), size(fontSize) {}
    
    float sizeValue() const { return size; }
};

// Transform matrix
struct Transform {
    float a, b, c, d, e, f;
    
    Transform() : a(1), b(0), c(0), d(1), e(0), f(0) {}
    
    void apply(float& x, float& y) const {
        float tx = x;
        float ty = y;
        x = a * tx + c * ty + e;
        y = b * tx + d * ty + f;
    }
};

// Render state
struct RenderState {
    Transform transform;
    Paint fillPaint;
    Paint strokePaint;
    float strokeWidth;
    Font font;
    bool scissoring;
    float scissorX, scissorY, scissorWidth, scissorHeight;
    
    RenderState() : strokeWidth(1.0f), scissoring(false), 
                   scissorX(0), scissorY(0), scissorWidth(0), scissorHeight(0) {}
};

// Custom render context
class CustomRenderContext {
public:
    CustomRenderContext();
    virtual ~CustomRenderContext();
    
    // State management
    void save();
    void restore();
    
    // Path operations
    void beginPath();
    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void rect(float x, float y, float w, float h);
    void roundedRect(float x, float y, float w, float h, float r);
    void circle(float cx, float cy, float r);
    void closePath();
    
    // Drawing operations
    void fill();
    void stroke();
    
    // Paint and style
    void setFillPaint(const Paint& paint);
    void setStrokePaint(const Paint& paint);
    void setStrokeWidth(float width);
    
    // Transform operations
    void translate(float x, float y);
    void rotate(float angle);
    void scale(float x, float y);
    
    // Text operations
    void setFont(const Font& font);
    float text(float x, float y, const std::string& string);
    void textBounds(float x, float y, const std::string& string, float bounds[4]);
    
    // Clipping
    void scissor(float x, float y, float w, float h);
    void resetScissor();
    
private:
    std::stack<RenderState> m_stateStack;
    RenderState m_currentState;
    std::vector<std::string> m_pathCommands;
};

} // namespace rendering
} // namespace browser

#endif // BROWSER_RENDERING_CUSTOM_RENDERER_H

// src/html/dom_tree.h - Basic DOM tree definitions
#ifndef BROWSER_HTML_DOM_TREE_H
#define BROWSER_HTML_DOM_TREE_H

#include <string>
#include <vector>
#include <memory>

namespace browser {
namespace html {

// Forward declarations
class Node;
class Element;
class Document;

// DOM tree container
class DOMTree {
public:
    DOMTree() = default;
    
    Document* document() const { return m_document.get(); }
    void setDocument(std::shared_ptr<Document> doc) { m_document = doc; }
    
private:
    std::shared_ptr<Document> m_document;
};

// Basic node types for compilation
enum class NodeType {
    ELEMENT_NODE = 1,
    TEXT_NODE = 3,
    DOCUMENT_NODE = 9
};

class Node {
public:
    virtual ~Node() = default;
    virtual NodeType nodeType() const = 0;
    virtual std::string nodeValue() const { return ""; }
};

class Element : public Node {
public:
    virtual NodeType nodeType() const override { return NodeType::ELEMENT_NODE; }
    virtual std::string tagName() const = 0;
    virtual std::string getAttribute(const std::string& name) const = 0;
    virtual bool hasAttribute(const std::string& name) const = 0;
    virtual void setAttribute(const std::string& name, const std::string& value) = 0;
    virtual std::string textContent() const = 0;
    virtual std::vector<Node*> childNodes() const = 0;
};

class Document : public Node {
public:
    virtual NodeType nodeType() const override { return NodeType::DOCUMENT_NODE; }
    virtual Element* getElementById(const std::string& id) const = 0;
    virtual std::vector<Element*> getElementsByTagName(const std::string& tagName) const = 0;
    virtual std::vector<Element*> getElementsByClassName(const std::string& className) const = 0;
    virtual std::shared_ptr<Element> createElement(const std::string& tagName) = 0;
    virtual std::string title() const = 0;
};

} // namespace html
} // namespace browser

#endif // BROWSER_HTML_DOM_TREE_H