#ifndef BROWSER_LAYOUT_ENGINE_H
#define BROWSER_LAYOUT_ENGINE_H

#include "box_model.h"
#include "../html/dom_tree.h"
#include "../css/style_resolver.h"
#include <memory>
#include <vector>
#include <map>

namespace browser {
namespace layout {

// Layout engine class
class LayoutEngine {
public:
    LayoutEngine();
    ~LayoutEngine();
    
    // Initialize the layout engine
    bool initialize();
    
    // Perform layout for a document
    bool layoutDocument(html::Document* document, css::StyleResolver* styleResolver,
                        float viewportWidth, float viewportHeight);
    
    // Get the layout tree root
    std::shared_ptr<Box> layoutRoot() const { return m_layoutRoot; }
    
    // Get the box associated with a DOM node
    Box* getBoxForNode(html::Node* node) const;
    
    // Helper method to print the layout tree (for debugging)
    void printLayoutTree(std::ostream& stream) const;
    
private:
    // Build the layout tree from the DOM tree
    std::shared_ptr<Box> buildLayoutTree(html::Node* node, css::StyleResolver* styleResolver, Box* parent);
    
    // Calculate layout for the entire tree
    void calculateLayout(float viewportWidth, float viewportHeight);
    
    // Helper method to recursively print layout tree
    void printLayoutTreeRecursive(std::ostream& stream, Box* box, int depth) const;
    
    // Layout tree root
    std::shared_ptr<Box> m_layoutRoot;
    
    // Map of DOM nodes to their corresponding boxes
    std::map<html::Node*, Box*> m_nodeToBoxMap;
};

} // namespace layout
} // namespace browser

#endif // BROWSER_LAYOUT_ENGINE_H