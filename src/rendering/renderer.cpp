#include "renderer.h"
#include "paint_system.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace browser {
namespace rendering {

//-----------------------------------------------------------------------------
// Color Implementation
//-----------------------------------------------------------------------------

Color Color::fromCssColor(const css::Value& value) {
    std::string colorStr = value.stringValue();
    
    // Handle named colors
    if (colorStr == "black") return Color(0, 0, 0);
    if (colorStr == "white") return Color(255, 255, 255);
    if (colorStr == "red") return Color(255, 0, 0);
    if (colorStr == "green") return Color(0, 128, 0);
    if (colorStr == "blue") return Color(0, 0, 255);
    if (colorStr == "yellow") return Color(255, 255, 0);
    if (colorStr == "purple") return Color(128, 0, 128);
    if (colorStr == "gray" || colorStr == "grey") return Color(128, 128, 128);
    if (colorStr == "transparent") return Color(0, 0, 0, 0);
    
    // Handle hex colors
    if (colorStr.size() >= 7 && colorStr[0] == '#') {
        int r = std::stoi(colorStr.substr(1, 2), nullptr, 16);
        int g = std::stoi(colorStr.substr(3, 2), nullptr, 16);
        int b = std::stoi(colorStr.substr(5, 2), nullptr, 16);
        return Color(r, g, b);
    } else if (colorStr.size() >= 4 && colorStr[0] == '#') {
        int r = std::stoi(colorStr.substr(1, 1) + colorStr.substr(1, 1), nullptr, 16);
        int g = std::stoi(colorStr.substr(2, 1) + colorStr.substr(2, 1), nullptr, 16);
        int b = std::stoi(colorStr.substr(3, 1) + colorStr.substr(3, 1), nullptr, 16);
        return Color(r, g, b);
    }
    
    // Handle rgb/rgba
    if (colorStr.substr(0, 4) == "rgb(") {
        // This is a very simplified RGB parser
        // A real implementation would handle more formats and error conditions
        std::string values = colorStr.substr(4, colorStr.length() - 5);
        std::stringstream ss(values);
        int r, g, b;
        char comma;
        ss >> r >> comma >> g >> comma >> b;
        return Color(r, g, b);
    }
    
    if (colorStr.substr(0, 5) == "rgba(") {
        // This is a very simplified RGBA parser
        std::string values = colorStr.substr(5, colorStr.length() - 6);
        std::stringstream ss(values);
        int r, g, b;
        float a;
        char comma;
        ss >> r >> comma >> g >> comma >> b >> comma >> a;
        return Color(r, g, b, a);
    }
    
    // Default to black
    return Color(0, 0, 0);
}

//-----------------------------------------------------------------------------
// ConsoleRenderingContext Implementation
//-----------------------------------------------------------------------------

ConsoleRenderingContext::ConsoleRenderingContext(int width, int height)
    : m_width(width)
    , m_height(height)
    , m_translateX(0)
    , m_translateY(0)
{
    // Initialize buffer with spaces
    m_buffer.resize(height, std::vector<char>(width, ' '));
    
    // Default colors
    m_fillColor = Color(255, 255, 255); // White
    m_strokeColor = Color(0, 0, 0);     // Black
    m_textColor = Color(0, 0, 0);       // Black
}

ConsoleRenderingContext::~ConsoleRenderingContext() {
}

void ConsoleRenderingContext::setFillColor(const Color& color) {
    m_fillColor = color;
}

void ConsoleRenderingContext::fillRect(float x, float y, float width, float height) {
    // Apply translation
    x += m_translateX;
    y += m_translateY;
    
    // Convert to integers
    int x1 = static_cast<int>(x);
    int y1 = static_cast<int>(y);
    int x2 = static_cast<int>(x + width - 1);
    int y2 = static_cast<int>(y + height - 1);
    
    // Clip to buffer bounds
    x1 = std::max(0, std::min(m_width - 1, x1));
    y1 = std::max(0, std::min(m_height - 1, y1));
    x2 = std::max(0, std::min(m_width - 1, x2));
    y2 = std::max(0, std::min(m_height - 1, y2));
    
    // Fill rectangle in buffer
    char fillChar = ' ';
    
    // Choose character based on color brightness
    float brightness = (m_fillColor.r + m_fillColor.g + m_fillColor.b) / (3.0f * 255.0f);
    if (brightness < 0.25f) {
        fillChar = '#'; // Dark
    } else if (brightness < 0.5f) {
        fillChar = '+'; // Medium
    } else if (brightness < 0.75f) {
        fillChar = '.'; // Light
    } else {
        fillChar = ' '; // White
    }
    
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            if (y >= 0 && y < m_height && x >= 0 && x < m_width) {
                m_buffer[y][x] = fillChar;
            }
        }
    }
}

void ConsoleRenderingContext::setStrokeColor(const Color& color) {
    m_strokeColor = color;
}

void ConsoleRenderingContext::strokeRect(float x, float y, float width, float height, float lineWidth) {
    // Apply translation
    x += m_translateX;
    y += m_translateY;
    
    // Convert to integers
    int x1 = static_cast<int>(x);
    int y1 = static_cast<int>(y);
    int x2 = static_cast<int>(x + width - 1);
    int y2 = static_cast<int>(y + height - 1);
    
    // Clip to buffer bounds
    x1 = std::max(0, std::min(m_width - 1, x1));
    y1 = std::max(0, std::min(m_height - 1, y1));
    x2 = std::max(0, std::min(m_width - 1, x2));
    y2 = std::max(0, std::min(m_height - 1, y2));
    
    // Choose character based on color brightness
    float brightness = (m_strokeColor.r + m_strokeColor.g + m_strokeColor.b) / (3.0f * 255.0f);
    char strokeChar = '+';
    
    if (brightness < 0.25f) {
        strokeChar = '#'; // Dark
    } else if (brightness < 0.5f) {
        strokeChar = '+'; // Medium
    } else if (brightness < 0.75f) {
        strokeChar = '.'; // Light
    } else {
        strokeChar = ' '; // White
    }
    
    // Draw top and bottom borders
    for (int x = x1; x <= x2; x++) {
        if (y1 >= 0 && y1 < m_height && x >= 0 && x < m_width) {
            m_buffer[y1][x] = strokeChar;
        }
        if (y2 >= 0 && y2 < m_height && x >= 0 && x < m_width) {
            m_buffer[y2][x] = strokeChar;
        }
    }
    
    // Draw left and right borders
    for (int y = y1; y <= y2; y++) {
        if (y >= 0 && y < m_height && x1 >= 0 && x1 < m_width) {
            m_buffer[y][x1] = strokeChar;
        }
        if (y >= 0 && y < m_height && x2 >= 0 && x2 < m_width) {
            m_buffer[y][x2] = strokeChar;
        }
    }
}

void ConsoleRenderingContext::setTextColor(const Color& color) {
    m_textColor = color;
}

void ConsoleRenderingContext::drawText(const std::string& text, float x, float y, 
                                     const std::string& fontFamily, float fontSize) {
    // Apply translation
    x += m_translateX;
    y += m_translateY;
    
    // Convert to integers
    int startX = static_cast<int>(x);
    int startY = static_cast<int>(y);
    
    // Ignore fontFamily and fontSize in ASCII rendering
    
    // Clip to buffer bounds
    if (startY < 0 || startY >= m_height) {
        return;
    }
    
    // Write text to buffer
    for (size_t i = 0; i < text.length(); i++) {
        int posX = startX + i;
        if (posX >= 0 && posX < m_width) {
            m_buffer[startY][posX] = text[i];
        }
    }
}

void ConsoleRenderingContext::save() {
    // Save current state
    State state;
    state.fillColor = m_fillColor;
    state.strokeColor = m_strokeColor;
    state.textColor = m_textColor;
    state.translateX = m_translateX;
    state.translateY = m_translateY;
    
    m_stateStack.push_back(state);
}

void ConsoleRenderingContext::restore() {
    // Restore last saved state
    if (!m_stateStack.empty()) {
        State state = m_stateStack.back();
        m_stateStack.pop_back();
        
        m_fillColor = state.fillColor;
        m_strokeColor = state.strokeColor;
        m_textColor = state.textColor;
        m_translateX = state.translateX;
        m_translateY = state.translateY;
    }
}

void ConsoleRenderingContext::translate(float x, float y) {
    m_translateX += x;
    m_translateY += y;
}

std::string ConsoleRenderingContext::toASCII(int width, int height) {
    std::stringstream ss;
    
    // Draw a border around the entire viewport
    ss << '+' << std::string(std::min(width, m_width), '-') << '+' << std::endl;
    
    // Draw each line of the buffer
    for (int y = 0; y < std::min(height, m_height); y++) {
        ss << '|';
        for (int x = 0; x < std::min(width, m_width); x++) {
            ss << m_buffer[y][x];
        }
        ss << '|' << std::endl;
    }
    
    // Draw bottom border
    ss << '+' << std::string(std::min(width, m_width), '-') << '+' << std::endl;
    
    return ss.str();
}

//-----------------------------------------------------------------------------
// ConsoleRenderTarget Implementation
//-----------------------------------------------------------------------------

ConsoleRenderTarget::ConsoleRenderTarget(int width, int height)
    : m_width(width)
    , m_height(height)
    , m_context(width, height)
{
}

ConsoleRenderTarget::~ConsoleRenderTarget() {
}

std::string ConsoleRenderTarget::toString() {
    return m_context.toASCII(m_width, m_height);
}

//-----------------------------------------------------------------------------
// BitmapRenderTarget Implementation
//-----------------------------------------------------------------------------

BitmapRenderTarget::BitmapRenderTarget(int width, int height)
    : m_width(width)
    , m_height(height)
    , m_context(width, height)
{
    // In a real implementation, this would create a bitmap buffer
}

BitmapRenderTarget::~BitmapRenderTarget() {
}

std::string BitmapRenderTarget::toString() {
    // In a real implementation, this would convert the bitmap to a string representation
    return m_context.toASCII(m_width, m_height);
}

//-----------------------------------------------------------------------------
// Renderer Implementation
//-----------------------------------------------------------------------------

Renderer::Renderer()
    : m_paintSystem(nullptr)
{
}

Renderer::~Renderer() {
}

bool Renderer::initialize() {
    // Create the paint system if not already created
    if (!m_paintSystem) {
        m_paintSystem = std::make_shared<PaintSystem>();
        if (!m_paintSystem->initialize()) {
            return false;
        }
    }
    
    return true;
}

std::shared_ptr<RenderTarget> Renderer::createTarget(RenderTargetType type, int width, int height) {
    switch (type) {
        case RenderTargetType::CONSOLE:
            return std::make_shared<ConsoleRenderTarget>(width, height);
        case RenderTargetType::BITMAP:
            return std::make_shared<BitmapRenderTarget>(width, height);
        default:
            // Unsupported type
            return nullptr;
    }
}

void Renderer::render(layout::Box* rootBox, RenderTarget* target) {
    if (!rootBox || !target || !m_paintSystem) {
        return;
    }
    
    // Create a paint context for the root box
    PaintContext context = m_paintSystem->createContext(rootBox);
    
    // Paint the root box and its children to the context
    m_paintSystem->paintBox(rootBox, context);
    
    // Render the display list to the target
    renderDisplayList(context.displayList(), target);
}

void Renderer::renderDisplayList(const DisplayList& displayList, RenderTarget* target) {
    if (!target) {
        return;
    }
    
    // Get the rendering context from the target
    RenderingContext* context = target->context();
    if (!context) {
        return;
    }
    
    // Clear the context
    context->save();
    context->setFillColor(Color(255, 255, 255)); // White
    context->fillRect(0, 0, target->width(), target->height());
    context->restore();
    
    // Render the display list to the context
    if (m_paintSystem) {
        m_paintSystem->paintDisplayList(displayList, context);
    }
}

std::string Renderer::renderToASCII(layout::Box* rootBox, int width, int height) {
    // Create a console render target
    auto target = createTarget(RenderTargetType::CONSOLE, width, height);
    if (!target) {
        return "Failed to create render target";
    }
    
    // Render the layout tree to the target
    render(rootBox, target.get());
    
    // Get the ASCII output
    return target->toString();
}

Color Renderer::getBoxBackgroundColor(layout::Box* box) {
    if (!box) {
        return Color(255, 255, 255); // Default to white
    }
    
    // Get background color from style
    css::Value bgColorValue = box->style().getProperty("background-color");
    return Color::fromCssColor(bgColorValue);
}

Color Renderer::getBoxBorderColor(layout::Box* box) {
    if (!box) {
        return Color(0, 0, 0); // Default to black
    }
    
    // Get border color from style
    css::Value borderColorValue = box->style().getProperty("border-color");
    return Color::fromCssColor(borderColorValue);
}

Color Renderer::getBoxTextColor(layout::Box* box) {
    if (!box) {
        return Color(0, 0, 0); // Default to black
    }
    
    // Get text color from style
    css::Value textColorValue = box->style().getProperty("color");
    return Color::fromCssColor(textColorValue);
}

void Renderer::drawBoxBackground(layout::Box* box, RenderingContext* context) {
    // Get background color
    Color bgColor = getBoxBackgroundColor(box);
    
    // Skip transparent backgrounds
    if (bgColor.a <= 0) {
        return;
    }
    
    // Set fill color
    context->setFillColor(bgColor);
    
    // Get border box for background
    layout::Rect borderBox = box->borderBox();
    
    // Fill rectangle
    context->fillRect(borderBox.x, borderBox.y, borderBox.width, borderBox.height);
}

void Renderer::drawBoxBorder(layout::Box* box, RenderingContext* context) {
    // Skip if no border
    if (box->borderTop() <= 0 && box->borderRight() <= 0 && 
        box->borderBottom() <= 0 && box->borderLeft() <= 0) {
        return;
    }
    
    // Get border color
    Color borderColor = getBoxBorderColor(box);
    
    // Set stroke color
    context->setStrokeColor(borderColor);
    
    // Get border box
    layout::Rect borderBox = box->borderBox();
    
    // Draw border
    context->strokeRect(borderBox.x, borderBox.y, borderBox.width, borderBox.height, 
                      std::max({box->borderTop(), box->borderRight(), 
                               box->borderBottom(), box->borderLeft()}));
}

void Renderer::drawBoxText(layout::Box* box, RenderingContext* context) {
    auto textBox = dynamic_cast<layout::TextBox*>(box);
    if (!textBox || !textBox->textNode()) {
        return;
    }
    
    // Get text color
    Color textColor = getBoxTextColor(box);
    context->setTextColor(textColor);
    
    // Get font properties
    css::Value fontFamilyValue = box->style().getProperty("font-family");
    css::Value fontSizeValue = box->style().getProperty("font-size");
    
    std::string fontFamily = fontFamilyValue.stringValue();
    if (fontFamily.empty()) {
        fontFamily = "Arial";
    }
    
    float fontSize = 16.0f; // Default
    if (fontSizeValue.type() == css::ValueType::LENGTH) {
        fontSize = fontSizeValue.numericValue();
    }
    
    // Get content rect
    layout::Rect contentRect = box->contentRect();
    
    // Draw text
    context->drawText(textBox->textNode()->nodeValue(), 
                    contentRect.x, contentRect.y + fontSize, // Position text baseline at content y + fontSize
                    fontFamily, fontSize);
}

} // namespace rendering
} // namespace browser