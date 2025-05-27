#include "dom_tree.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stack>
#include <queue>

namespace browser {
namespace html {

//-----------------------------------------------------------------------------
// Node Implementation
//-----------------------------------------------------------------------------

Node::Node(NodeType type)
    : m_nodeType(type)
    , m_nodeName("")
    , m_nodeValue("")
    , m_ownerDocument(nullptr)
    , m_parentNode(nullptr)
    , m_previousSibling(nullptr)
    , m_nextSibling(nullptr)
{
}

Node::~Node() {
    // Clear child nodes
    m_childNodes.clear();
}

Element* Node::parentElement() const {
    if (m_parentNode && m_parentNode->nodeType() == NodeType::ELEMENT_NODE) {
        return static_cast<Element*>(m_parentNode);
    }
    return nullptr;
}

std::shared_ptr<Node> Node::appendChild(std::shared_ptr<Node> newChild) {
    if (!newChild) {
        return nullptr;
    }
    
    // Remove from previous parent if any
    if (newChild->m_parentNode) {
        newChild->m_parentNode->removeChild(newChild);
    }
    
    // Set the new parent
    newChild->m_parentNode = this;
    
    // Set the owner document
    if (m_ownerDocument && newChild->m_ownerDocument != m_ownerDocument) {
        newChild->m_ownerDocument = m_ownerDocument;
    }
    
    // Add to child nodes
    if (!m_childNodes.empty()) {
        newChild->m_previousSibling = m_childNodes.back().get();
        m_childNodes.back()->m_nextSibling = newChild.get();
    }
    
    newChild->m_nextSibling = nullptr;
    m_childNodes.push_back(newChild);
    
    // Update ID mappings if this is an element with an ID
    if (newChild->nodeType() == NodeType::ELEMENT_NODE) {
        Element* element = static_cast<Element*>(newChild.get());
        if (element->hasAttribute("id") && m_ownerDocument) {
            m_ownerDocument->registerElementId(element->id(), element);
        }
    }
    
    return newChild;
}

std::shared_ptr<Node> Node::insertBefore(std::shared_ptr<Node> newChild, std::shared_ptr<Node> refChild) {
    if (!newChild) {
        return nullptr;
    }
    
    // If refChild is null, append to the end
    if (!refChild) {
        return appendChild(newChild);
    }
    
    // Check if refChild is a child of this node
    auto it = std::find_if(m_childNodes.begin(), m_childNodes.end(),
                          [&refChild](const std::shared_ptr<Node>& child) {
                              return child.get() == refChild.get();
                          });
    
    if (it == m_childNodes.end()) {
        // refChild not found
        return nullptr;
    }
    
    // Remove from previous parent if any
    if (newChild->m_parentNode) {
        newChild->m_parentNode->removeChild(newChild);
    }
    
    // Set the new parent
    newChild->m_parentNode = this;
    
    // Set the owner document
    if (m_ownerDocument && newChild->m_ownerDocument != m_ownerDocument) {
        newChild->m_ownerDocument = m_ownerDocument;
    }
    
    // Insert before refChild
    m_childNodes.insert(it, newChild);
    
    // Update sibling pointers
    updateSiblingPointers();
    
    // Update ID mappings if this is an element with an ID
    if (newChild->nodeType() == NodeType::ELEMENT_NODE) {
        Element* element = static_cast<Element*>(newChild.get());
        if (element->hasAttribute("id") && m_ownerDocument) {
            m_ownerDocument->registerElementId(element->id(), element);
        }
    }
    
    return newChild;
}

std::shared_ptr<Node> Node::removeChild(std::shared_ptr<Node> child) {
    if (!child) {
        return nullptr;
    }
    
    // Find the child
    auto it = std::find_if(m_childNodes.begin(), m_childNodes.end(),
                          [&child](const std::shared_ptr<Node>& node) {
                              return node.get() == child.get();
                          });
    
    if (it == m_childNodes.end()) {
        // Child not found
        return nullptr;
    }
    
    // Update ID mappings if this is an element with an ID
    if (child->nodeType() == NodeType::ELEMENT_NODE) {
        Element* element = static_cast<Element*>(child.get());
        if (element->hasAttribute("id") && m_ownerDocument) {
            m_ownerDocument->unregisterElementId(element->id());
        }
    }
    
    // Clear parent and owner document
    child->m_parentNode = nullptr;
    
    // Remove from child nodes
    m_childNodes.erase(it);
    
    // Update sibling pointers
    updateSiblingPointers();
    
    return child;
}

std::shared_ptr<Node> Node::replaceChild(std::shared_ptr<Node> newChild, std::shared_ptr<Node> oldChild) {
    if (!newChild || !oldChild) {
        return nullptr;
    }
    
    // Find the old child
    auto it = std::find_if(m_childNodes.begin(), m_childNodes.end(),
                          [&oldChild](const std::shared_ptr<Node>& node) {
                              return node.get() == oldChild.get();
                          });
    
    if (it == m_childNodes.end()) {
        // Old child not found
        return nullptr;
    }
    
    // Remove from previous parent if any
    if (newChild->m_parentNode) {
        newChild->m_parentNode->removeChild(newChild);
    }
    
    // Update ID mappings for old child
    if (oldChild->nodeType() == NodeType::ELEMENT_NODE) {
        Element* element = static_cast<Element*>(oldChild.get());
        if (element->hasAttribute("id") && m_ownerDocument) {
            m_ownerDocument->unregisterElementId(element->id());
        }
    }
    
    // Set the new parent and owner document for new child
    newChild->m_parentNode = this;
    if (m_ownerDocument && newChild->m_ownerDocument != m_ownerDocument) {
        newChild->m_ownerDocument = m_ownerDocument;
    }
    
    // Replace the child
    *it = newChild;
    
    // Update sibling pointers
    updateSiblingPointers();
    
    // Update ID mappings for new child
    if (newChild->nodeType() == NodeType::ELEMENT_NODE) {
        Element* element = static_cast<Element*>(newChild.get());
        if (element->hasAttribute("id") && m_ownerDocument) {
            m_ownerDocument->registerElementId(element->id(), element);
        }
    }
    
    // Clear parent for old child
    oldChild->m_parentNode = nullptr;
    oldChild->m_previousSibling = nullptr;
    oldChild->m_nextSibling = nullptr;
    
    return oldChild;
}

void Node::updateSiblingPointers() {
    Node* prev = nullptr;
    
    for (auto& child : m_childNodes) {
        child->m_previousSibling = prev;
        if (prev) {
            prev->m_nextSibling = child.get();
        }
        prev = child.get();
    }
    
    if (prev) {
        prev->m_nextSibling = nullptr;
    }
}

//-----------------------------------------------------------------------------
// Element Implementation
//-----------------------------------------------------------------------------

Element::Element(const std::string& tagName)
    : Node(NodeType::ELEMENT_NODE)
    , m_tagName(tagName)
{
    m_nodeName = tagName;
}

Element::~Element() {
}

std::string Element::id() const {
    return getAttribute("id");
}

std::string Element::className() const {
    return getAttribute("class");
}

bool Element::hasAttribute(const std::string& name) const {
    return m_attributes.find(name) != m_attributes.end();
}

std::string Element::getAttribute(const std::string& name) const {
    auto it = m_attributes.find(name);
    if (it != m_attributes.end()) {
        return it->second;
    }
    return "";
}

void Element::setAttribute(const std::string& name, const std::string& value) {
    // Check if this is an ID change
    if (name == "id" && m_ownerDocument) {
        // Unregister old ID if any
        if (hasAttribute("id")) {
            m_ownerDocument->unregisterElementId(getAttribute("id"));
        }
        
        // Register new ID
        m_ownerDocument->registerElementId(value, this);
    }
    
    m_attributes[name] = value;
}

void Element::removeAttribute(const std::string& name) {
    // Unregister ID if removing id attribute
    if (name == "id" && hasAttribute("id") && m_ownerDocument) {
        m_ownerDocument->unregisterElementId(getAttribute("id"));
    }
    
    m_attributes.erase(name);
}

std::vector<Element*> Element::getElementsByTagName(const std::string& tagName) const {
    std::vector<Element*> elements;
    
    // Use BFS to traverse the tree
    std::queue<Node*> queue;
    for (const auto& child : m_childNodes) {
        queue.push(child.get());
    }
    
    while (!queue.empty()) {
        Node* node = queue.front();
        queue.pop();
        
        if (node->nodeType() == NodeType::ELEMENT_NODE) {
            Element* element = static_cast<Element*>(node);
            
            // Check if tag name matches (case insensitive)
            std::string elementTag = element->tagName();
            std::string targetTag = tagName;
            std::transform(elementTag.begin(), elementTag.end(), elementTag.begin(), 
                [](unsigned char c) { return static_cast<char>(::tolower(c)); });
            std::transform(targetTag.begin(), targetTag.end(), targetTag.begin(), 
                [](unsigned char c) { return static_cast<char>(::tolower(c)); });
            
            if (targetTag == "*" || elementTag == targetTag) {
                elements.push_back(element);
            }
            
            // Add children to queue
            for (const auto& child : element->childNodes()) {
                queue.push(child.get());
            }
        }
    }
    
    return elements;
}

std::vector<Element*> Element::getElementsByClassName(const std::string& className) const {
    std::vector<Element*> elements;
    
    // Use BFS to traverse the tree
    std::queue<Node*> queue;
    for (const auto& child : m_childNodes) {
        queue.push(child.get());
    }
    
    while (!queue.empty()) {
        Node* node = queue.front();
        queue.pop();
        
        if (node->nodeType() == NodeType::ELEMENT_NODE) {
            Element* element = static_cast<Element*>(node);
            
            // Check if class name matches
            std::string classes = element->className();
            std::istringstream iss(classes);
            std::string cls;
            while (std::getline(iss, cls, ' ')) {
                if (cls == className) {
                    elements.push_back(element);
                    break;
                }
            }
            
            // Add children to queue
            for (const auto& child : element->childNodes()) {
                queue.push(child.get());
            }
        }
    }
    
    return elements;
}

Element* Element::querySelector(const std::string& selector) const {
    // Simplified selector implementation (only supports tag, class, and ID selectors)
    // For real browser, would use a full CSS selector engine
    
    // Check if it's an ID selector (#id)
    if (selector.size() > 1 && selector[0] == '#') {
        std::string id = selector.substr(1);
        if (m_ownerDocument) {
            return m_ownerDocument->getElementById(id);
        }
        
        // Fallback to manual search
        std::queue<Node*> queue;
        for (const auto& child : m_childNodes) {
            queue.push(child.get());
        }
        
        while (!queue.empty()) {
            Node* node = queue.front();
            queue.pop();
            
            if (node->nodeType() == NodeType::ELEMENT_NODE) {
                Element* element = static_cast<Element*>(node);
                if (element->id() == id) {
                    return element;
                }
                
                // Add children to queue
                for (const auto& child : element->childNodes()) {
                    queue.push(child.get());
                }
            }
        }
    }
    
    // Check if it's a class selector (.class)
    if (selector.size() > 1 && selector[0] == '.') {
        std::string className = selector.substr(1);
        std::vector<Element*> elements = getElementsByClassName(className);
        if (!elements.empty()) {
            return elements[0];
        }
        return nullptr;
    }
    
    // Assume it's a tag selector
    std::vector<Element*> elements = getElementsByTagName(selector);
    if (!elements.empty()) {
        return elements[0];
    }
    
    return nullptr;
}

std::vector<Element*> Element::querySelectorAll(const std::string& selector) const {
    // Simplified selector implementation (only supports tag, class, and ID selectors)
    
    // Check if it's an ID selector (#id)
    if (selector.size() > 1 && selector[0] == '#') {
        std::string id = selector.substr(1);
        Element* element = querySelector(selector);
        if (element) {
            return {element};
        }
        return {};
    }
    
    // Check if it's a class selector (.class)
    if (selector.size() > 1 && selector[0] == '.') {
        std::string className = selector.substr(1);
        return getElementsByClassName(className);
    }
    
    // Assume it's a tag selector
    return getElementsByTagName(selector);
}

Element* Element::firstElementChild() const {
    for (const auto& child : m_childNodes) {
        if (child->nodeType() == NodeType::ELEMENT_NODE) {
            return static_cast<Element*>(child.get());
        }
    }
    return nullptr;
}

Element* Element::lastElementChild() const {
    for (auto it = m_childNodes.rbegin(); it != m_childNodes.rend(); ++it) {
        if ((*it)->nodeType() == NodeType::ELEMENT_NODE) {
            return static_cast<Element*>(it->get());
        }
    }
    return nullptr;
}

Element* Element::previousElementSibling() const {
    Node* node = m_previousSibling;
    while (node && node->nodeType() != NodeType::ELEMENT_NODE) {
        node = node->m_previousSibling;
    }
    return node ? static_cast<Element*>(node) : nullptr;
}

Element* Element::nextElementSibling() const {
    Node* node = m_nextSibling;
    while (node && node->nodeType() != NodeType::ELEMENT_NODE) {
        node = node->m_nextSibling;
    }
    return node ? static_cast<Element*>(node) : nullptr;
}

std::string Element::innerHTML() const {
    std::ostringstream oss;
    
    for (const auto& child : m_childNodes) {
        oss << child->toString();
    }
    
    return oss.str();
}

void Element::setInnerHTML(const std::string& html) {
    // Clear existing children
    while (!m_childNodes.empty()) {
        removeChild(m_childNodes[0]);
    }
    
    // Parse the HTML and add new children
    // Note: In a real browser, this would use the HTML parser
    // Here we would call the HTMLParser to parse the fragment
    // This is left as an exercise for the browser implementation
}

std::string Element::textContent() const {
    std::ostringstream oss;
    
    // Recursively get text content
    for (const auto& child : m_childNodes) {
        if (child->nodeType() == NodeType::TEXT_NODE) {
            oss << child->nodeValue();
        } else if (child->nodeType() == NodeType::ELEMENT_NODE) {
            oss << static_cast<Element*>(child.get())->textContent();
        }
    }
    
    return oss.str();
}

void Element::setTextContent(const std::string& text) {
    // Clear existing children
    while (!m_childNodes.empty()) {
        removeChild(m_childNodes[0]);
    }
    
    // Add a new text node
    if (!text.empty() && m_ownerDocument) {
        appendChild(m_ownerDocument->createTextNode(text));
    }
}

std::shared_ptr<Node> Element::cloneNode(bool deep) const {
    auto clone = std::make_shared<Element>(m_tagName);
    
    // Copy attributes
    for (const auto& attr : m_attributes) {
        clone->setAttribute(attr.first, attr.second);
    }
    
    // Clone children if deep
    if (deep) {
        for (const auto& child : m_childNodes) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    
    return clone;
}

std::string Element::toString() const {
    std::ostringstream oss;
    
    // Opening tag
    oss << "<" << m_tagName;
    
    // Attributes
    for (const auto& attr : m_attributes) {
        oss << " " << attr.first << "=\"" << attr.second << "\"";
    }
    
    // Self-closing or content
    bool selfClosing = m_tagName == "area" || m_tagName == "base" || m_tagName == "br" || 
                       m_tagName == "col" || m_tagName == "embed" || m_tagName == "hr" || 
                       m_tagName == "img" || m_tagName == "input" || m_tagName == "link" || 
                       m_tagName == "meta" || m_tagName == "param" || m_tagName == "source" || 
                       m_tagName == "track" || m_tagName == "wbr";
    
    if (selfClosing && m_childNodes.empty()) {
        oss << " />";
    } else {
        oss << ">";
        
        // Children
        for (const auto& child : m_childNodes) {
            oss << child->toString();
        }
        
        // Closing tag
        oss << "</" << m_tagName << ">";
    }
    
    return oss.str();
}

//-----------------------------------------------------------------------------
// Text Implementation
//-----------------------------------------------------------------------------

Text::Text(const std::string& data)
    : Node(NodeType::TEXT_NODE)
{
    m_nodeName = "#text";
    m_nodeValue = data;
}

Text::~Text() {
}

std::string Text::substringData(size_t offset, size_t count) const {
    if (offset > m_nodeValue.length()) {
        return "";
    }
    
    return m_nodeValue.substr(offset, count);
}

void Text::appendData(const std::string& data) {
    m_nodeValue += data;
}

void Text::insertData(size_t offset, const std::string& data) {
    if (offset > m_nodeValue.length()) {
        offset = m_nodeValue.length();
    }
    
    m_nodeValue.insert(offset, data);
}

void Text::deleteData(size_t offset, size_t count) {
    if (offset > m_nodeValue.length()) {
        return;
    }
    
    m_nodeValue.erase(offset, count);
}

void Text::replaceData(size_t offset, size_t count, const std::string& data) {
    if (offset > m_nodeValue.length()) {
        return;
    }
    
    m_nodeValue.replace(offset, count, data);
}

std::shared_ptr<Node> Text::cloneNode(bool /*deep*/) const {
    return std::make_shared<Text>(m_nodeValue);
}

std::string Text::toString() const {
    // Escape special characters for HTML output
    std::string escaped = m_nodeValue;
    size_t pos = 0;
    
    // Replace & with &amp;
    while ((pos = escaped.find("&", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "&amp;");
        pos += 5;
    }
    
    // Replace < with &lt;
    pos = 0;
    while ((pos = escaped.find("<", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "&lt;");
        pos += 4;
    }
    
    // Replace > with &gt;
    pos = 0;
    while ((pos = escaped.find(">", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "&gt;");
        pos += 4;
    }
    
    return escaped;
}

//-----------------------------------------------------------------------------
// Comment Implementation
//-----------------------------------------------------------------------------

Comment::Comment(const std::string& data)
    : Node(NodeType::COMMENT_NODE)
{
    m_nodeName = "#comment";
    m_nodeValue = data;
}

Comment::~Comment() {
}

std::shared_ptr<Node> Comment::cloneNode(bool deep) const {
    return std::make_shared<Comment>(m_nodeValue);
}

std::string Comment::toString() const {
    return "<!--" + m_nodeValue + "-->";
}

//-----------------------------------------------------------------------------
// DocumentType Implementation
//-----------------------------------------------------------------------------

DocumentType::DocumentType(const std::string& name, 
                         const std::string& publicId, 
                         const std::string& systemId)
    : Node(NodeType::DOCUMENT_TYPE_NODE)
    , m_name(name)
    , m_publicId(publicId)
    , m_systemId(systemId)
{
    m_nodeName = name;
}

DocumentType::~DocumentType() {
}

std::shared_ptr<Node> DocumentType::cloneNode(bool deep) const {
    return std::make_shared<DocumentType>(m_name, m_publicId, m_systemId);
}

std::string DocumentType::toString() const {
    std::ostringstream oss;
    oss << "<!DOCTYPE " << m_name;
    
    if (!m_publicId.empty() || !m_systemId.empty()) {
        if (!m_publicId.empty()) {
            oss << " PUBLIC \"" << m_publicId << "\"";
            if (!m_systemId.empty()) {
                oss << " \"" << m_systemId << "\"";
            }
        } else if (!m_systemId.empty()) {
            oss << " SYSTEM \"" << m_systemId << "\"";
        }
    }
    
    oss << ">";
    return oss.str();
}

//-----------------------------------------------------------------------------
// Document Implementation
//-----------------------------------------------------------------------------

Document::Document()
    : Node(NodeType::DOCUMENT_NODE)
{
    m_nodeName = "#document";
    m_ownerDocument = this;
}

Document::~Document() {
}

DocumentType* Document::doctype() const {
    for (const auto& child : m_childNodes) {
        if (child->nodeType() == NodeType::DOCUMENT_TYPE_NODE) {
            return static_cast<DocumentType*>(child.get());
        }
    }
    return nullptr;
}

Element* Document::documentElement() const {
    for (const auto& child : m_childNodes) {
        if (child->nodeType() == NodeType::ELEMENT_NODE) {
            return static_cast<Element*>(child.get());
        }
    }
    return nullptr;
}

std::string Document::title() const {
    Element* root = documentElement();
    if (!root) {
        return "";
    }
    
    Element* head = nullptr;
    for (const auto& child : root->childNodes()) {
        if (child->nodeType() == NodeType::ELEMENT_NODE && 
            static_cast<Element*>(child.get())->tagName() == "head") {
            head = static_cast<Element*>(child.get());
            break;
        }
    }
    
    if (!head) {
        return "";
    }
    
    for (const auto& child : head->childNodes()) {
        if (child->nodeType() == NodeType::ELEMENT_NODE && 
            static_cast<Element*>(child.get())->tagName() == "title") {
            return static_cast<Element*>(child.get())->textContent();
        }
    }
    
    return "";
}

void Document::setTitle(const std::string& title) {
    Element* root = documentElement();
    if (!root) {
        return;
    }
    
    Element* head = nullptr;
    for (const auto& child : root->childNodes()) {
        if (child->nodeType() == NodeType::ELEMENT_NODE && 
            static_cast<Element*>(child.get())->tagName() == "head") {
            head = static_cast<Element*>(child.get());
            break;
        }
    }
    
    if (!head) {
        head = createElement("head").get();
        root->appendChild(std::shared_ptr<Element>(static_cast<Element*>(head->cloneNode(false).get())));
    }
    
    Element* titleElement = nullptr;
    for (const auto& child : head->childNodes()) {
        if (child->nodeType() == NodeType::ELEMENT_NODE && 
            static_cast<Element*>(child.get())->tagName() == "title") {
            titleElement = static_cast<Element*>(child.get());
            break;
        }
    }
    
    if (!titleElement) {
        titleElement = createElement("title").get();
        head->appendChild(std::shared_ptr<Element>(static_cast<Element*>(titleElement->cloneNode(false).get())));
    }
    
    titleElement->setTextContent(title);
}

std::shared_ptr<Element> Document::createElement(const std::string& tagName) {
    auto element = std::make_shared<Element>(tagName);
    element->m_ownerDocument = this;
    return element;
}

std::shared_ptr<Text> Document::createTextNode(const std::string& data) {
    auto text = std::make_shared<Text>(data);
    text->m_ownerDocument = this;
    return text;
}

std::shared_ptr<Comment> Document::createComment(const std::string& data) {
    auto comment = std::make_shared<Comment>(data);
    comment->m_ownerDocument = this;
    return comment;
}

std::shared_ptr<DocumentType> Document::createDocumentType(const std::string& name, 
                                                         const std::string& publicId, 
                                                         const std::string& systemId) {
    auto doctype = std::make_shared<DocumentType>(name, publicId, systemId);
    doctype->m_ownerDocument = this;
    return doctype;
}

Element* Document::getElementById(const std::string& id) const {
    auto it = m_elementsById.find(id);
    if (it != m_elementsById.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<Element*> Document::getElementsByTagName(const std::string& tagName) const {
    std::vector<Element*> elements;
    
    Element* root = documentElement();
    if (root) {
        // Check if the document element matches
        std::string rootTag = root->tagName();
        std::string targetTag = tagName;
        std::transform(rootTag.begin(), rootTag.end(), rootTag.begin(), ::tolower);
        std::transform(targetTag.begin(), targetTag.end(), targetTag.begin(), ::tolower);
        
        if (targetTag == "*" || rootTag == targetTag) {
            elements.push_back(root);
        }
        
        // Get elements from the document element
        std::vector<Element*> childElements = root->getElementsByTagName(tagName);
        elements.insert(elements.end(), childElements.begin(), childElements.end());
    }
    
    return elements;
}

std::vector<Element*> Document::getElementsByClassName(const std::string& className) const {
    std::vector<Element*> elements;
    
    Element* root = documentElement();
    if (root) {
        // Check if the document element matches
        std::string classes = root->className();
        std::istringstream iss(classes);
        std::string cls;
        bool matches = false;
        
        while (std::getline(iss, cls, ' ')) {
            if (cls == className) {
                matches = true;
                break;
            }
        }
        
        if (matches) {
            elements.push_back(root);
        }
        
        // Get elements from the document element
        std::vector<Element*> childElements = root->getElementsByClassName(className);
        elements.insert(elements.end(), childElements.begin(), childElements.end());
    }
    
    return elements;
}

Element* Document::querySelector(const std::string& selector) const {
    Element* root = documentElement();
    if (!root) {
        return nullptr;
    }
    
    // Check if it's an ID selector
    if (selector.size() > 1 && selector[0] == '#') {
        std::string id = selector.substr(1);
        return getElementById(id);
    }
    
    // Check if the document element matches
    if (selector[0] == '.') {
        std::string className = selector.substr(1);
        std::string classes = root->className();
        std::istringstream iss(classes);
        std::string cls;
        
        while (std::getline(iss, cls, ' ')) {
            if (cls == className) {
                return root;
            }
        }
    } else if (selector == root->tagName() || selector == "*") {
        return root;
    }
    
    // Try to find in children
    return root->querySelector(selector);
}

std::vector<Element*> Document::querySelectorAll(const std::string& selector) const {
    std::vector<Element*> elements;
    
    Element* root = documentElement();
    if (!root) {
        return elements;
    }
    
    // Check if it's an ID selector
    if (selector.size() > 1 && selector[0] == '#') {
        std::string id = selector.substr(1);
        Element* element = getElementById(id);
        if (element) {
            elements.push_back(element);
        }
        return elements;
    }
    
    // Check if the document element matches
    bool rootMatches = false;
    
    if (selector[0] == '.') {
        std::string className = selector.substr(1);
        std::string classes = root->className();
        std::istringstream iss(classes);
        std::string cls;
        
        while (std::getline(iss, cls, ' ')) {
            if (cls == className) {
                rootMatches = true;
                break;
            }
        }
    } else if (selector == root->tagName() || selector == "*") {
        rootMatches = true;
    }
    
    if (rootMatches) {
        elements.push_back(root);
    }
    
    // Add elements from children
    std::vector<Element*> childElements = root->querySelectorAll(selector);
    elements.insert(elements.end(), childElements.begin(), childElements.end());
    
    return elements;
}

std::shared_ptr<Node> Document::cloneNode(bool deep) const {
    auto clone = std::make_shared<Document>();
    
    // Clone children if deep
    if (deep) {
        for (const auto& child : m_childNodes) {
            clone->appendChild(child->cloneNode(true));
        }
    }
    
    return clone;
}

std::string Document::toString() const {
    std::ostringstream oss;
    
    for (const auto& child : m_childNodes) {
        oss << child->toString();
    }
    
    return oss.str();
}

std::vector<std::string> Document::findStylesheetLinks() const {
    std::vector<std::string> links;
    
    // Find all link elements with rel="stylesheet"
    std::vector<Element*> linkElements = getElementsByTagName("link");
    
    for (Element* link : linkElements) {
        if (link->getAttribute("rel") == "stylesheet" && !link->getAttribute("href").empty()) {
            links.push_back(link->getAttribute("href"));
        }
    }
    
    return links;
}

std::vector<std::string> Document::findInlineStyles() const {
    std::vector<std::string> styles;
    
    // Find all style elements
    std::vector<Element*> styleElements = getElementsByTagName("style");
    
    for (Element* style : styleElements) {
        styles.push_back(style->textContent());
    }
    
    return styles;
}

void Document::registerElementId(const std::string& id, Element* element) {
    if (!id.empty()) {
        m_elementsById[id] = element;
    }
}

void Document::unregisterElementId(const std::string& id) {
    if (!id.empty()) {
        m_elementsById.erase(id);
    }
}

//-----------------------------------------------------------------------------
// DOMTree Implementation
//-----------------------------------------------------------------------------

DOMTree::DOMTree() {
    initialize();
}

DOMTree::~DOMTree() {
}

void DOMTree::initialize() {
    m_document = std::make_shared<Document>();
}

void DOMTree::setContent(const std::string& /*html*/) {
    // Parse and set content
    // This would be done by the HTML parser
    // Left as an exercise for the browser implementation
}

std::string DOMTree::toHTML() const {
    if (!m_document) {
        return "";
    }
    
    return m_document->toString();
}

std::vector<std::string> DOMTree::findStylesheetLinks() const {
    if (!m_document) {
        return {};
    }
    
    return m_document->findStylesheetLinks();
}

std::vector<std::string> DOMTree::findInlineStyles() const {
    if (!m_document) {
        return {};
    }
    
    return m_document->findInlineStyles();
}

std::vector<std::string> DOMTree::findScriptSources() const {
    std::vector<std::string> sources;
    
    if (!m_document) {
        return sources;
    }
    
    // Find all script elements with src attribute
    std::vector<Element*> scriptElements = m_document->getElementsByTagName("script");
    
    for (Element* script : scriptElements) {
        if (!script->getAttribute("src").empty()) {
            sources.push_back(script->getAttribute("src"));
        }
    }
    
    return sources;
}

std::vector<std::string> DOMTree::findImageSources() const {
    std::vector<std::string> sources;
    
    if (!m_document) {
        return sources;
    }
    
    // Find all img elements
    std::vector<Element*> imgElements = m_document->getElementsByTagName("img");
    
    for (Element* img : imgElements) {
        if (!img->getAttribute("src").empty()) {
            sources.push_back(img->getAttribute("src"));
        }
    }
    
    return sources;
}

void DOMTree::traverse(const std::function<void(Node*)>& callback) const {
    if (!m_document || !callback) {
        return;
    }
    
    traverseNode(m_document.get(), callback);
}

void DOMTree::traverseNode(Node* node, const std::function<void(Node*)>& callback) const {
    if (!node) {
        return;
    }
    
    // Call the callback for this node
    callback(node);
    
    // Traverse children
    for (const auto& child : node->childNodes()) {
        traverseNode(child.get(), callback);
    }
}

} // namespace html
} // namespace browser