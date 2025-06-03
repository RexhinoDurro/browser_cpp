#ifndef BROWSER_UI_CUSTOM_CONTROLS_H
#define BROWSER_UI_CUSTOM_CONTROLS_H

#include <string>
#include <functional>
#include <memory>
#include <vector>

// Forward declare necessary classes instead of including headers
namespace browser {
    namespace rendering {
        class Color;
        class CustomRenderContext;
        class Paint;
        class Font;
    }
    
    namespace ui {
        class BrowserWindow;
    }
}

// Include only what we need for the base UI classes
#include "window.h"

namespace browser {
namespace ui {

// Base control class
class Control {
public:
    Control(int x, int y, int width, int height);
    virtual ~Control();
    
    // Position and size
    void setPosition(int x, int y);
    void getPosition(int& x, int& y) const;
    
    void setSize(int width, int height);
    void getSize(int& width, int& height) const;
    
    // Visibility
    void setVisible(bool visible);
    bool isVisible() const;
    
    // Enabled state
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    // Focus state
    void setFocus(bool focus);
    bool hasFocus() const;
    
    // Drawing and event handling
    virtual void draw(rendering::CustomRenderContext* ctx) = 0;
    virtual bool handleMouseMove(int x, int y);
    virtual bool handleMouseButton(int button, int action, int mods, int x, int y);
    virtual bool handleKeyInput(int key, int scancode, int action, int mods);
    
    // Hit testing
    bool contains(int x, int y) const;
    
protected:
    int m_x, m_y;
    int m_width, m_height;
    bool m_visible;
    bool m_enabled;
    bool m_hover;
    bool m_focused;
};

// Button control
class Button : public Control {
public:
    Button(int x, int y, int width, int height, const std::string& text);
    virtual ~Button();
    
    // Button text
    void setText(const std::string& text);
    std::string getText() const;
    
    // Click event handler
    void setClickHandler(std::function<void()> handler);
    
    // Drawing and event handling
    virtual void draw(rendering::CustomRenderContext* ctx) override;
    virtual bool handleMouseButton(int button, int action, int mods, int x, int y) override;
    
private:
    std::string m_text;
    std::function<void()> m_clickHandler;
};

// Text input control
class TextInput : public Control {
public:
    TextInput(int x, int y, int width, int height);
    virtual ~TextInput();
    
    // Text content
    void setText(const std::string& text);
    std::string getText() const;
    
    // Set placeholder text
    void setPlaceholder(const std::string& placeholder);
    
    // Submit event handler (for Enter key)
    void setSubmitHandler(std::function<void(const std::string&)> handler);
    
    // Text change handler
    void setTextChangeHandler(std::function<void(const std::string&)> handler);
    
    // Drawing and event handling
    virtual void draw(rendering::CustomRenderContext* ctx) override;
    virtual bool handleKeyInput(int key, int scancode, int action, int mods) override;
    virtual bool handleMouseButton(int button, int action, int mods, int x, int y) override;
    
private:
    std::string m_text;
    std::string m_placeholder;
    size_t m_cursorPos;
    size_t m_selectionStart;
    bool m_selecting;
    
    std::function<void(const std::string&)> m_submitHandler;
    std::function<void(const std::string&)> m_textChangeHandler;
    
    // Helper methods
    void moveCursor(int direction, bool selecting);
    void deletePreviousChar();
    void deleteNextChar();
    void insertText(const std::string& text);
};

// Progress bar control
class ProgressBar : public Control {
public:
    ProgressBar(int x, int y, int width, int height);
    virtual ~ProgressBar();
    
    // Progress value (0.0 - 1.0)
    void setValue(float value);
    float getValue() const;
    
    // Set indeterminate state
    void setIndeterminate(bool indeterminate);
    bool isIndeterminate() const;
    
    // Drawing
    virtual void draw(rendering::CustomRenderContext* ctx) override;
    
private:
    float m_value;
    bool m_indeterminate;
    float m_animationOffset;
};

// Toolbar container
class Toolbar : public Control {
public:
    Toolbar(int x, int y, int width, int height);
    virtual ~Toolbar();
    
    // Add controls
    void addControl(std::shared_ptr<Control> control);
    
    // Drawing and event handling
    virtual void draw(rendering::CustomRenderContext* ctx) override;
    virtual bool handleMouseMove(int x, int y) override;
    virtual bool handleMouseButton(int button, int action, int mods, int x, int y) override;
    virtual bool handleKeyInput(int key, int scancode, int action, int mods) override;
    
private:
    std::vector<std::shared_ptr<Control>> m_controls;
};

// Browser UI controls container
class BrowserControls {
public:
    BrowserControls(BrowserWindow* window);
    ~BrowserControls();
    
    // Initialize controls
    bool initialize();
    
    // Draw controls
    void draw(rendering::CustomRenderContext* ctx);
    
    // Process input events
    bool handleMouseMove(int x, int y);
    bool handleMouseButton(int button, int action, int mods, int x, int y);
    bool handleKeyInput(int key, int scancode, int action, int mods);
    bool handleResize(int width, int height);
    
    // Get address bar text
    std::string getAddressBarText() const;
    
    // Set address bar text
    void setAddressBarText(const std::string& text);
    
    // Show/hide progress bar
    void setLoading(bool loading);
    
private:
    BrowserWindow* m_window;
    rendering::CustomRenderContext* m_renderContext;
    
    // UI layout
    int m_toolbarHeight;
    
    // UI controls
    std::shared_ptr<Toolbar> m_toolbar;
    std::shared_ptr<Button> m_backButton;
    std::shared_ptr<Button> m_forwardButton;
    std::shared_ptr<Button> m_reloadButton;
    std::shared_ptr<Button> m_stopButton;
    std::shared_ptr<Button> m_homeButton;      // Add this
    std::shared_ptr<Button> m_goButton;     
    std::shared_ptr<TextInput> m_addressBar;
    std::shared_ptr<ProgressBar> m_progressBar;
    
    // Initialize renderer
    bool initializeRenderer();
    void updateNavigationState();  
    // Layout controls
    void layoutControls();
};

} // namespace ui
} // namespace browser

#endif // BROWSER_UI_CUSTOM_CONTROLS_H