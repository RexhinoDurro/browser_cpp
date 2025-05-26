#ifndef BROWSER_PAINT_SYSTEM_H
#define BROWSER_PAINT_SYSTEM_H

#include "../layout/box_model.h"
#include "render_target.h"
#include "custom_renderer.h"
#include <memory>
#include <vector>
#include <map>

namespace browser {
namespace rendering {

// Display item types
enum class DisplayItemType {
    BACKGROUND,
    BORDER,
    TEXT,
    IMAGE,
    RECT,
    TRANSFORM,
    CLIP,
    LINE
};

// Base display item class
class DisplayItem {
public:
    DisplayItem(DisplayItemType type);
    virtual ~DisplayItem();
    
    // Get item type
    DisplayItemType type() const { return m_type; }
    
    // Paint method (to be overridden by derived classes)
    virtual void paint(RenderingContext* context) = 0;
    
private:
    DisplayItemType m_type;
};

// Background display item
class BackgroundDisplayItem : public DisplayItem {
public:
    BackgroundDisplayItem(const layout::Rect& rect, const Color& color);
    virtual ~BackgroundDisplayItem();
    
    // Getters
    const layout::Rect& rect() const { return m_rect; }
    const Color& color() const { return m_color; }
    
    // Paint method
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
    
    // Getters
    const layout::Rect& rect() const { return m_rect; }
    const Color& color() const { return m_color; }
    float topWidth() const { return m_topWidth; }
    float rightWidth() const { return m_rightWidth; }
    float bottomWidth() const { return m_bottomWidth; }
    float leftWidth() const { return m_leftWidth; }
    
    // Paint method
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
    
    // Getters
    const std::string& text() const { return m_text; }
    float x() const { return m_x; }
    float y() const { return m_y; }
    const Color& color() const { return m_color; }
    const std::string& fontFamily() const { return m_fontFamily; }
    float fontSize() const { return m_fontSize; }
    
    // Paint method
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
    
    // Getters
    const std::string& url() const { return m_url; }
    const layout::Rect& rect() const { return m_rect; }
    
    // Paint method
    virtual void paint(RenderingContext* context) override;
    
private:
    std::string m_url;
    layout::Rect m_rect;
};

// Rectangle display item
class RectDisplayItem : public DisplayItem {
public:
    RectDisplayItem(const layout::Rect& rect, const Color& color, bool fill);
    virtual ~RectDisplayItem();
    
    // Getters
    const layout::Rect& rect() const { return m_rect; }
    const Color& color() const { return m_color; }
    bool filled() const { return m_fill; }
    
    // Paint method
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
    
    // Getters
    float dx() const { return m_dx; }
    float dy() const { return m_dy; }
    
    // Paint method
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
    
    // Getters
    const layout::Rect& rect() const { return m_rect; }
    
    // Paint method
    virtual void paint(RenderingContext* context) override;
    
private:
    layout::Rect m_rect;
};

// Line display item
class LineDisplayItem : public DisplayItem {
public:
    LineDisplayItem(float x1, float y1, float x2, float y2, const Color& color, float thickness);
    virtual ~LineDisplayItem();
    
    // Getters
    float x1() const { return m_x1; }
    float y1() const { return m_y1; }
    float x2() const { return m_x2; }
    float y2() const { return m_y2; }
    const Color& color() const { return m_color; }
    float thickness() const { return m_thickness; }
    
    // Paint method
    virtual void paint(RenderingContext* context) override;
    
private:
    float m_x1, m_y1, m_x2, m_y2;
    Color m_color;
    float m_thickness;
};

// Display list class - a list of display items to be painted
class DisplayList {
public:
    DisplayList();
    ~DisplayList();
    
    // Delete copy constructor and copy assignment operator
    DisplayList(const DisplayList&) = delete;
    DisplayList& operator=(const DisplayList&) = delete;
    
    // Define move constructor and move assignment operator
    DisplayList(DisplayList&& other) noexcept : m_items(std::move(other.m_items)) {}
    DisplayList& operator=(DisplayList&& other) noexcept {
        if (this != &other) {
            m_items = std::move(other.m_items);
        }
        return *this;
    }
    
    // Add an item to the list
    void appendItem(std::unique_ptr<DisplayItem> item);
    
    // Get all items
    const std::vector<std::unique_ptr<DisplayItem>>& items() const { return m_items; }
    
    // Paint all items to a context
    void paint(RenderingContext* context) const;
    
    // Clear the list
    void clear();
    
private:
    std::vector<std::unique_ptr<DisplayItem>> m_items;
};

// Paint context class - provides methods for building a display list
class PaintContext {
public:
    PaintContext();
    ~PaintContext();
    
    // Delete copy constructor and copy assignment operator
    PaintContext(const PaintContext&) = delete;
    PaintContext& operator=(const PaintContext&) = delete;
    
    // Define move constructor and move assignment operator
    PaintContext(PaintContext&& other) noexcept = default;
    PaintContext& operator=(PaintContext&& other) noexcept = default;
    
    // Get the display list
    const DisplayList& displayList() const { return m_displayList; }
    
    // Drawing methods
    void drawBackground(const layout::Rect& rect, const Color& color);
    void drawBorder(const layout::Rect& rect, const Color& color, 
                   float top, float right, float bottom, float left);
    void drawText(const std::string& text, float x, float y, 
                 const Color& color, const std::string& fontFamily, float fontSize);
    void drawImage(const std::string& url, const layout::Rect& rect);
    void drawRect(const layout::Rect& rect, const Color& color, bool fill);
    void drawLine(float x1, float y1, float x2, float y2, const Color& color, float thickness);
    void transform(float dx, float dy);
    void clip(const layout::Rect& rect);
    
private:
    DisplayList m_displayList;
};

// Paint system class - responsible for painting the layout tree
class PaintSystem {
public:
    PaintSystem();
    ~PaintSystem();
    
    // Initialize the paint system
    bool initialize();
    
    // Create a paint context for a box
    PaintContext createContext(layout::Box* box);
    
    // Paint a box and its children to a context
    void paintBox(layout::Box* box, PaintContext& context);
    
    // Paint a text box
    void paintText(layout::TextBox* textBox, PaintContext& context);
    
    // Paint a display list to a rendering context
    void paintDisplayList(const DisplayList& displayList, RenderingContext* context);
    
private:
    // Helper methods
    Color getBackgroundColor(layout::Box* box);
    Color getBorderColor(layout::Box* box);
    Color getTextColor(layout::Box* box);
};

} // namespace rendering
} // namespace browser

#endif // BROWSER_PAINT_SYSTEM_H