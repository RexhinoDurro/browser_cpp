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
    RenderingContext* baseContext = target->context();
    if (!baseContext) {
        return;
    }
    
    // Try to cast to our custom context type
    auto context = dynamic_cast<CustomRenderingContext*>(baseContext);
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
        // Paint each display item manually since we're using our custom context
        for (const auto& item : displayList.items()) {
            if (!item) continue;
            
            switch (item->type()) {
                case DisplayItemType::BACKGROUND: {
                    auto bgItem = static_cast<BackgroundDisplayItem*>(item.get());
                    context->setFillColor(bgItem->color());
                    context->fillRect(bgItem->rect().x, bgItem->rect().y, 
                                    bgItem->rect().width, bgItem->rect().height);
                    break;
                }
                
                case DisplayItemType::BORDER: {
                    auto borderItem = static_cast<BorderDisplayItem*>(item.get());
                    context->setStrokeColor(borderItem->color());
                    float maxWidth = std::max({borderItem->topWidth(), borderItem->rightWidth(), 
                                             borderItem->bottomWidth(), borderItem->leftWidth()});
                    context->strokeRect(borderItem->rect().x, borderItem->rect().y, 
                                      borderItem->rect().width, borderItem->rect().height, maxWidth);
                    break;
                }
                
                case DisplayItemType::TEXT: {
                    auto textItem = static_cast<TextDisplayItem*>(item.get());
                    context->setTextColor(textItem->color());
                    context->drawText(textItem->text(), textItem->x(), textItem->y(), 
                                    textItem->fontFamily(), textItem->fontSize());
                    break;
                }
                
                case DisplayItemType::IMAGE: {
                    // Draw a placeholder for images
                    auto imageItem = static_cast<ImageDisplayItem*>(item.get());
                    context->setFillColor(Color(200, 200, 200)); // Light gray
                    context->fillRect(imageItem->rect().x, imageItem->rect().y, 
                                    imageItem->rect().width, imageItem->rect().height);
                    
                    context->setStrokeColor(Color(100, 100, 100)); // Dark gray
                    context->strokeRect(imageItem->rect().x, imageItem->rect().y, 
                                      imageItem->rect().width, imageItem->rect().height, 1.0f);
                    
                    // Draw image URL as text
                    context->setTextColor(Color(0, 0, 0));
                    context->drawText("Image: " + imageItem->url(), 
                                    imageItem->rect().x + 5, imageItem->rect().y + 15, 
                                    "sans", 10.0f);
                    break;
                }
                
                case DisplayItemType::RECT: {
                    auto rectItem = static_cast<RectDisplayItem*>(item.get());
                    if (rectItem->filled()) {
                        context->setFillColor(rectItem->color());
                        context->fillRect(rectItem->rect().x, rectItem->rect().y, 
                                        rectItem->rect().width, rectItem->rect().height);
                    } else {
                        context->setStrokeColor(rectItem->color());
                        context->strokeRect(rectItem->rect().x, rectItem->rect().y, 
                                          rectItem->rect().width, rectItem->rect().height, 1.0f);
                    }
                    break;
                }
                
                case DisplayItemType::TRANSFORM: {
                    auto transformItem = static_cast<TransformDisplayItem*>(item.get());
                    context->translate(transformItem->dx(), transformItem->dy());
                    break;
                }
                
                case DisplayItemType::CLIP: {
                    // Clipping not implemented in our simple context
                    break;
                }
            }
        }
    }
}

void Renderer::renderCustomDisplayList(const DisplayList& displayList, CustomRenderContext* ctx) {
    if (!ctx) {
        return;
    }
    
    // Clear the context
    ctx->beginPath();
    ctx->rect(0, 0, ctx->getWindowWidth(), ctx->getWindowHeight());
    Paint whitePaint;
    whitePaint.setColor(Color(255, 255, 255)); // White
    ctx->setFillPaint(whitePaint);
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
                Paint bgPaint;
                bgPaint.setColor(bgItem->color());
                ctx->setFillPaint(bgPaint);
                ctx->fill();
                break;
            }
            
            case DisplayItemType::BORDER: {
                auto borderItem = static_cast<BorderDisplayItem*>(item.get());
                ctx->beginPath();
                ctx->rect(borderItem->rect().x, borderItem->rect().y, 
                         borderItem->rect().width, borderItem->rect().height);
                Paint borderPaint;
                borderPaint.setColor(borderItem->color());
                ctx->setStrokePaint(borderPaint);
                ctx->setStrokeWidth(std::max({borderItem->topWidth(), borderItem->rightWidth(), 
                                            borderItem->bottomWidth(), borderItem->leftWidth()}));
                ctx->stroke();
                break;
            }
            
            case DisplayItemType::TEXT: {
                auto textItem = static_cast<TextDisplayItem*>(item.get());
                ctx->setFont(Font(textItem->fontFamily(), textItem->fontSize()));
                Paint textPaint;
                textPaint.setColor(textItem->color());
                ctx->setFillPaint(textPaint);
                ctx->text(textItem->x(), textItem->y(), textItem->text());
                break;
            }
            
            case DisplayItemType::IMAGE: {
                auto imageItem = static_cast<ImageDisplayItem*>(item.get());
                // Draw a placeholder for images
                ctx->beginPath();
                ctx->rect(imageItem->rect().x, imageItem->rect().y, 
                         imageItem->rect().width, imageItem->rect().height);
                Paint grayPaint;
                grayPaint.setColor(Color(200, 200, 200)); // Light gray
                ctx->setFillPaint(grayPaint);
                ctx->fill();
                
                ctx->beginPath();
                ctx->rect(imageItem->rect().x, imageItem->rect().y, 
                         imageItem->rect().width, imageItem->rect().height);
                Paint darkGrayPaint;
                darkGrayPaint.setColor(Color(100, 100, 100)); // Dark gray
                ctx->setStrokePaint(darkGrayPaint);
                ctx->setStrokeWidth(1.0f);
                ctx->stroke();
                
                // Draw image URL as text
                ctx->setFont(Font("sans", 10.0f));
                Paint blackPaint;
                blackPaint.setColor(Color(0, 0, 0));
                ctx->setFillPaint(blackPaint);
                ctx->text(imageItem->rect().x + 5, imageItem->rect().y + 15, 
                         "Image: " + imageItem->url());
                break;
            }
            
            case DisplayItemType::RECT: {
                auto rectItem = static_cast<RectDisplayItem*>(item.get());
                ctx->beginPath();
                ctx->rect(rectItem->rect().x, rectItem->rect().y, 
                         rectItem->rect().width, rectItem->rect().height);
                Paint rectPaint;
                rectPaint.setColor(rectItem->color());
                if (rectItem->filled()) {
                    ctx->setFillPaint(rectPaint);
                    ctx->fill();
                } else {
                    ctx->setStrokePaint(rectPaint);
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