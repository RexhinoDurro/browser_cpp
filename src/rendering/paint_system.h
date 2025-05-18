#ifndef BROWSER_PAINT_SYSTEM_H
#define BROWSER_PAINT_SYSTEM_H

#include "../layout/layout_engine.h"
#include "renderer.h"
#include <vector>
#include <memory>

namespace browser {
namespace rendering {

// Forward declarations
class DisplayItem;
class DisplayList;
class PaintContext;

// Types of display items
enum class DisplayItemType {
    BACKGROUND,    // Background color/image
    BORDER,        // Border
    TEXT,          // Text content
    IMAGE,         // Image content
    RECT,          // Rectangle (for debugging)
    TRANSFORM,     // Transform state
    CLIP           // Clipping region
};

// Base class for all display items
class DisplayItem {
public:
    DisplayItem(DisplayItemType type);
    virtual ~DisplayItem();
    
    DisplayItemType type() const { return m_type; }
    
    // Paint this item to a rendering context
    virtual void paint(RenderingContext* context) = 0;
    
private:
    DisplayItemType m_type;
};

// Background display item
class BackgroundDisplayItem : public DisplayItem {
public:
    BackgroundDisplayItem(const layout::Rect& rect, const Color& color);
    virtual ~BackgroundDisplayItem();
    
    virtual void paint(RenderingContext* context) override;
    
private:
    layout::Rect m_rect;
    Color m_color;
};

// Border display item
class BorderDisplayItem : public DisplayItem {
public:
    BorderDisplayItem(const layout::Rect& rect, const Color& color, 
                     float top, float right, float bottom, float left);
    virtual ~BorderDisplayItem();
    
    virtual void paint(RenderingContext* context) override;
    
private:
    layout::Rect m_rect;
    Color m_color;
    float m_topWidth;
    float m_rightWidth;
    float m_bottomWidth;
    float m_leftWidth;
};

// Text display item
class TextDisplayItem : public DisplayItem {
public:
    TextDisplayItem(const std::string& text, float x, float y, 
                  const Color& color, const std::string& fontFamily, float fontSize);
    virtual ~TextDisplayItem();
    
    virtual void paint(RenderingContext* context) override;
    
private:
    std::string m_text;
    float m_x;
    float m_y;
    Color m_color;
    std::string m_fontFamily;
    float m_fontSize;
};

// Image display item
class ImageDisplayItem : public DisplayItem {
public:
    ImageDisplayItem(const std::string& url, const layout::Rect& rect);
    virtual ~ImageDisplayItem();
    
    virtual void paint(RenderingContext* context) override;
    
private:
    std::string m_url;
    layout::Rect m_rect;
    // In a real implementation, this would store the image data
};

// Rect display item (for debugging)
class RectDisplayItem : public DisplayItem {
public:
    RectDisplayItem(const layout::Rect& rect, const Color& color, bool fill);
    virtual ~RectDisplayItem();
    
    virtual void paint(RenderingContext* context) override;
    
private:
    layout::Rect m_rect;
    Color m_color;
    bool m_fill;
};

// Transform display item
class TransformDisplayItem : public DisplayItem {
public:
    TransformDisplayItem(float dx, float dy);
    virtual ~TransformDisplayItem();
    
    virtual void paint(RenderingContext* context) override;
    
private:
    float m_dx;
    float m_dy;
};

// Clip display item
class ClipDisplayItem : public DisplayItem {
public:
    ClipDisplayItem(const layout::Rect& rect);
    virtual ~ClipDisplayItem();
    
    virtual void paint(RenderingContext* context) override;
    
private:
    layout::Rect m_rect;
};

// Display list - ordered list of display items
class DisplayList {
public:
    DisplayList();
    ~DisplayList();
    
    // Disallow copying (unique_ptr can't be copied)
    DisplayList(const DisplayList&) = delete;
    DisplayList& operator=(const DisplayList&) = delete;
    
    // Allow moving
    DisplayList(DisplayList&&) = default;
    DisplayList& operator=(DisplayList&&) = default;
    
    // Add items to the list
    void appendItem(std::unique_ptr<DisplayItem> item);
    
    // Get all items
    const std::vector<std::unique_ptr<DisplayItem>>& items() const { return m_items; }
    
    // Paint all items to a rendering context
    void paint(RenderingContext* context) const;
    
    // Clear the list
    void clear();
    
private:
    std::vector<std::unique_ptr<DisplayItem>> m_items;
};

// Paint context
class PaintContext {
public:
    PaintContext();
    ~PaintContext();
    
    // Get the display list
    DisplayList& displayList() { return m_displayList; }
    
    // Add specific display items
    void drawBackground(const layout::Rect& rect, const Color& color);
    void drawBorder(const layout::Rect& rect, const Color& color, 
                   float top, float right, float bottom, float left);
    void drawText(const std::string& text, float x, float y, 
                 const Color& color, const std::string& fontFamily, float fontSize);
    void drawImage(const std::string& url, const layout::Rect& rect);
    void drawRect(const layout::Rect& rect, const Color& color, bool fill);
    void transform(float dx, float dy);
    void clip(const layout::Rect& rect);
    
private:
    DisplayList m_displayList;
};

// Paint system
class PaintSystem {
public:
    PaintSystem();
    ~PaintSystem();
    
    // Initialize the paint system
    bool initialize();
    
    // Create a paint context for a specific box
    PaintContext createContext(layout::Box* box);
    
    // Paint a layout box and its children to a display list
    void paintBox(layout::Box* box, PaintContext& context);
    
    // Paint text content
    void paintText(layout::TextBox* textBox, PaintContext& context);
    
    // Get colors for box parts
    Color getBackgroundColor(layout::Box* box);
    Color getBorderColor(layout::Box* box);
    Color getTextColor(layout::Box* box);
    
    // Paint the display list to a rendering context
    void paintDisplayList(const DisplayList& displayList, RenderingContext* context);
};

} // namespace rendering
} // namespace browser

#endif // BROWSER_PAINT_SYSTEM_H