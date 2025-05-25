// src/rendering/modified_renderer.cpp
// This is a modified version of renderer.cpp that uses our custom renderer instead of NanoVG

#include "renderer.h"
#include "paint_system.h"
#include "custom_render_target.h"
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
            // Use the custom renderer
            return std::make_shared<CustomRenderTarget>(width, height);
            
        case RenderTargetType::BITMAP:
            // Use the custom renderer for bitmap too
            return std::make_shared<CustomRenderTarget>(width, height);
            
        default:
            // Unsupported type
            return nullptr;
    }
}

void Renderer::render(layout::Box* rootBox, RenderTarget* target) {
    if (!rootBox || !target || !m_paintSystem) {
        return;
    }
    
    // Check if target is a CustomRenderTarget
    auto customTarget = dynamic_cast<CustomRenderTarget*>(target);
    if (customTarget) {
        // Begin rendering frame
        auto ctx = customTarget->getCustomContext();
        if (ctx) {
            ctx->beginFrame(target->width(), target->height(), 1.0f);
            
            // Create a paint context for the root box
            PaintContext context = m_paintSystem->createContext(rootBox);
            
            // Paint the root box and its children to the context
            m_paintSystem->paintBox(rootBox, context);
            
            // Render the display list to the target using the custom context
            renderCustomDisplayList(context.displayList(), ctx);
            
            // End rendering frame
            ctx->endFrame();
        }
    } else {
        // Create a paint context for the root box
        PaintContext context = m_paintSystem->createContext(rootBox);
        
        // Paint the root box and its children to the context
        m_paintSystem->paintBox(rootBox, context);
        
        // Render the display list to the target
        renderDisplayList(context.displayList(), target);
    }
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

void Renderer::renderCustomDisplayList(const DisplayList& displayList, CustomRenderContext* ctx) {
    if (!ctx) {
        return;
    }
    
    // Clear the context
    ctx->beginPath();
    ctx->rect(0, 0, ctx->getWindowWidth(), ctx->getWindowHeight());
    ctx->setFillPaint(Paint(Color(255, 255, 255))); // White
    ctx->fill();
    
    // Render each display item
    for (const auto& item : displayList.items()) {
        if (!item) continue;
        
        switch (item->type()) {
            case DisplayItemType::BACKGROUND: {
                auto bgItem = static_cast<BackgroundDisplayItem*>(item.get());
                ctx->beginPath();
                ctx->rect(bgItem->rect().x, bgItem->rect().y, 
                          bgItem->rect().width, bgItem->rect().height);
                ctx->setFillPaint(Paint(bgItem->color()));
                ctx->fill();
                break;
            }
            
            case DisplayItemType::BORDER: {
                auto borderItem = static_cast<BorderDisplayItem*>(item.get());
                ctx->beginPath();
                ctx->rect(borderItem->rect().x, borderItem->rect().y, 
                         borderItem->rect().width, borderItem->rect().height);
                ctx->setStrokePaint(Paint(borderItem->color()));
                ctx->setStrokeWidth(std::max({borderItem->topWidth(), borderItem->rightWidth(), 
                                            borderItem->bottomWidth(), borderItem->leftWidth()}));
                ctx->stroke();
                break;
            }
            
            case DisplayItemType::TEXT: {
                auto textItem = static_cast<TextDisplayItem*>(item.get());
                ctx->setFont(Font(textItem->fontFamily(), textItem->fontSize()));
                ctx->setFillPaint(Paint(textItem->color()));
                ctx->text(textItem->x(), textItem->y(), textItem->text());
                break;
            }
            
            case DisplayItemType::IMAGE: {
                auto imageItem = static_cast<ImageDisplayItem*>(item.get());
                // Draw a placeholder for images
                ctx->beginPath();
                ctx->rect(imageItem->rect().x, imageItem->rect().y, 
                         imageItem->rect().width, imageItem->rect().height);
                ctx->setFillPaint(Paint(Color(200, 200, 200))); // Light gray
                ctx->fill();
                
                ctx->beginPath();
                ctx->rect(imageItem->rect().x, imageItem->rect().y, 
                         imageItem->rect().width, imageItem->rect().height);
                ctx->setStrokePaint(Paint(Color(100, 100, 100))); // Dark gray
                ctx->setStrokeWidth(1.0f);
                ctx->stroke();
                
                // Draw image URL as text
                ctx->setFont(Font("sans", 10.0f));
                ctx->setFillPaint(Paint(Color(0, 0, 0)));
                ctx->text(imageItem->rect().x + 5, imageItem->rect().y + 15, 
                         "Image: " + imageItem->url());
                break;
            }
            
            case DisplayItemType::RECT: {
                auto rectItem = static_cast<RectDisplayItem*>(item.get());
                ctx->beginPath();
                ctx->rect(rectItem->rect().x, rectItem->rect().y, 
                         rectItem->rect().width, rectItem->rect().height);
                if (rectItem->filled()) {
                    ctx->setFillPaint(Paint(rectItem->color()));
                    ctx->fill();
                } else {
                    ctx->setStrokePaint(Paint(rectItem->color()));
                    ctx->setStrokeWidth(1.0f);
                    ctx->stroke();
                }
                break;
            }
            
            case DisplayItemType::TRANSFORM: {
                auto transformItem = static_cast<TransformDisplayItem*>(item.get());
                ctx->translate(transformItem->dx(), transformItem->dy());
                break;
            }
            
            case DisplayItemType::CLIP: {
                auto clipItem = static_cast<ClipDisplayItem*>(item.get());
                ctx->scissor(clipItem->rect().x, clipItem->rect().y, 
                            clipItem->rect().width, clipItem->rect().height);
                break;
            }
        }
    }
}

std::string Renderer::renderToASCII(layout::Box* rootBox, int width, int height) {
    // Create a render target
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

} // namespace rendering
} // namespace browser