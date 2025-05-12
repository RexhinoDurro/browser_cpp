#ifndef BROWSER_DOM_TREE_H
#define BROWSER_DOM_TREE_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>

namespace browser {
namespace html {

// Forward declarations
class Node;
class Element;
class Text;
class Comment;
class DocumentType;
class Document;

// NodeType enum representing different types of DOM nodes
enum class NodeType {
    ELEMENT_NODE = 1,
    TEXT_NODE = 3,
    COMMENT_NODE = 8,
    DOCUMENT_NODE = 9,
    DOCUMENT_TYPE_NODE = 10
};

// Base Node class
class Node {
public:
    Node(NodeType type);
    virtual ~Node();
    
    // Node properties
    NodeType nodeType() const { return m_nodeType; }
    std::string nodeName() const { return m_nodeName; }
    std::string nodeValue() const { return m_nodeValue; }
    void setNodeValue(const std::string& value) { m_nodeValue = value; }
    
    // Node hierarchy
    Document* ownerDocument() const { return m_ownerDocument; }
    Node* parentNode() const { return m_parentNode; }
    Element* parentElement() const;
    bool hasChildNodes() const { return !m_childNodes.empty(); }
    const std::vector<std::shared_ptr<Node>>& childNodes() const { return m_childNodes; }
    Node* firstChild() const { return m_childNodes.empty() ? nullptr : m_childNodes.front().get(); }
    Node* lastChild() const { return m_childNodes.empty() ? nullptr : m_childNodes.back().get(); }
    Node* previousSibling() const { return m_previousSibling; }
    Node* nextSibling() const { return m_nextSibling; }
    
    // DOM operations
    std::shared_ptr<Node> appendChild(std::shared_ptr<Node> newChild);
    std::shared_ptr<Node> insertBefore(std::shared_ptr<Node> newChild, std::shared_ptr<Node> refChild);
    std::shared_ptr<Node> removeChild(std::shared_ptr<Node> child);
    std::shared_ptr<Node> replaceChild(std::shared_ptr<Node> newChild, std::shared_ptr<Node> oldChild);
    
    // Clone this node
    virtual std::shared_ptr<Node> cloneNode(bool deep = false) const = 0;
    
    // Convert to string representation (for debugging)
    virtual std::string toString() const = 0;
    
    // Get element by ID (to be overridden by Document)
    virtual Element* getElementById(const std::string& id) const { return nullptr; }
    
    // Query selectors (to be implemented by Document and Element)
    virtual Element* querySelector(const std::string& selector) const { return nullptr; }
    virtual std::vector<Element*> querySelectorAll(const std::string& selector) const { return {}; }
    
protected:
    NodeType m_nodeType;
    std::string m_nodeName;
    std::string m_nodeValue;
    Document* m_ownerDocument;
    Node* m_parentNode;
    std::vector<std::shared_ptr<Node>> m_childNodes;
    Node* m_previousSibling;
    Node* m_nextSibling;
    
    // Update sibling pointers after child list changes
    void updateSiblingPointers();
    
    friend class Document;
    friend class Element;
    friend class Text;
    friend class Comment;
    friend class HTMLParser;
};

// Element node representing HTML elements
class Element : public Node {
public:
    Element(const std::string& tagName);
    virtual ~Element();
    
    // Element properties
    std::string tagName() const { return m_tagName; }
    std::string id() const;
    std::string className() const;
    
    // Attributes
    bool hasAttributes() const { return !m_attributes.empty(); }
    const std::map<std::string, std::string>& attributes() const { return m_attributes; }
    bool hasAttribute(const std::string& name) const;
    std::string getAttribute(const std::string& name) const;
    void setAttribute(const std::string& name, const std::string& value);
    void removeAttribute(const std::string& name);
    
    // DOM navigation
    std::vector<Element*> getElementsByTagName(const std::string& tagName) const;
    std::vector<Element*> getElementsByClassName(const std::string& className) const;
    
    // CSS selection
    virtual Element* querySelector(const std::string& selector) const override;
    virtual std::vector<Element*> querySelectorAll(const std::string& selector) const override;
    
    // DOM hierarchy
    Element* firstElementChild() const;
    Element* lastElementChild() const;
    Element* previousElementSibling() const;
    Element* nextElementSibling() const;
    
    // Inner content
    std::string innerHTML() const;
    void setInnerHTML(const std::string& html);
    std::string textContent() const;
    void setTextContent(const std::string& text);
    
    // Cloning
    virtual std::shared_ptr<Node> cloneNode(bool deep = false) const override;
    
    // Convert to string (for debugging)
    virtual std::string toString() const override;
    
private:
    std::string m_tagName;
    std::map<std::string, std::string> m_attributes;
};

// Text node for text content
class Text : public Node {
public:
    Text(const std::string& data);
    virtual ~Text();
    
    // Text properties
    std::string data() const { return m_nodeValue; }
    void setData(const std::string& data) { m_nodeValue = data; }
    size_t length() const { return m_nodeValue.length(); }
    
    // Text operations
    std::string substringData(size_t offset, size_t count) const;
    void appendData(const std::string& data);
    void insertData(size_t offset, const std::string& data);
    void deleteData(size_t offset, size_t count);
    void replaceData(size_t offset, size_t count, const std::string& data);
    
    // Cloning
    virtual std::shared_ptr<Node> cloneNode(bool deep = false) const override;
    
    // Convert to string (for debugging)
    virtual std::string toString() const override;
};

// Comment node
class Comment : public Node {
public:
    Comment(const std::string& data);
    virtual ~Comment();
    
    // Comment properties
    std::string data() const { return m_nodeValue; }
    void setData(const std::string& data) { m_nodeValue = data; }
    
    // Cloning
    virtual std::shared_ptr<Node> cloneNode(bool deep = false) const override;
    
    // Convert to string (for debugging)
    virtual std::string toString() const override;
};

// DocumentType node representing <!DOCTYPE> declaration
class DocumentType : public Node {
public:
    DocumentType(const std::string& name, 
                 const std::string& publicId = "", 
                 const std::string& systemId = "");
    virtual ~DocumentType();
    
    // DocumentType properties
    std::string name() const { return m_name; }
    std::string publicId() const { return m_publicId; }
    std::string systemId() const { return m_systemId; }
    
    // Cloning
    virtual std::shared_ptr<Node> cloneNode(bool deep = false) const override;
    
    // Convert to string (for debugging)
    virtual std::string toString() const override;
    
private:
    std::string m_name;
    std::string m_publicId;
    std::string m_systemId;
};

// Document node representing the entire HTML document
class Document : public Node {
public:
    Document();
    virtual ~Document();
    
    // Document properties
    DocumentType* doctype() const;
    Element* documentElement() const;
    std::string title() const;
    void setTitle(const std::string& title);
    
    // DOM creation methods
    std::shared_ptr<Element> createElement(const std::string& tagName);
    std::shared_ptr<Text> createTextNode(const std::string& data);
    std::shared_ptr<Comment> createComment(const std::string& data);
    std::shared_ptr<DocumentType> createDocumentType(const std::string& name, 
                                                    const std::string& publicId = "", 
                                                    const std::string& systemId = "");
    
    // Element access
    virtual Element* getElementById(const std::string& id) const override;
    std::vector<Element*> getElementsByTagName(const std::string& tagName) const;
    std::vector<Element*> getElementsByClassName(const std::string& className) const;
    
    // CSS selection
    virtual Element* querySelector(const std::string& selector) const override;
    virtual std::vector<Element*> querySelectorAll(const std::string& selector) const override;
    
    // Cloning
    virtual std::shared_ptr<Node> cloneNode(bool deep = false) const override;
    
    // Convert to string (for debugging)
    virtual std::string toString() const override;
    
    // Get and find style sheets
    std::vector<std::string> findStylesheetLinks() const;
    std::vector<std::string> findInlineStyles() const;
    
    // Register elements with IDs - made public for HTMLParser access
    void registerElementId(const std::string& id, Element* element);
    void unregisterElementId(const std::string& id);
    
private:
    // ID to element mapping for fast getElementById lookup
    std::map<std::string, Element*> m_elementsById;
    
    friend class Element;
    friend class HTMLParser;
};

// The main DOM tree class
class DOMTree {
public:
    DOMTree();
    ~DOMTree();
    
    // Initialize a new document
    void initialize();
    
    // Access document
    Document* document() const { return m_document.get(); }
    
    // Parse and set document content
    void setContent(const std::string& html);
    
    // Render the DOM tree as HTML
    std::string toHTML() const;
    
    // Find resources in the document
    std::vector<std::string> findStylesheetLinks() const;
    std::vector<std::string> findInlineStyles() const;
    std::vector<std::string> findScriptSources() const;
    std::vector<std::string> findImageSources() const;
    
    // Traverse the DOM tree
    void traverse(const std::function<void(Node*)>& callback) const;
    
private:
    // Recursive traversal helper
    void traverseNode(Node* node, const std::function<void(Node*)>& callback) const;
    
    // Document root
    std::shared_ptr<Document> m_document;
};

} // namespace html
} // namespace browser

#endif // BROWSER_DOM_TREE_H