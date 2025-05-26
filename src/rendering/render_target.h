// src/rendering/render_target.h - Base render target class
#ifndef BROWSER_RENDERING_RENDER_TARGET_H
#define BROWSER_RENDERING_RENDER_TARGET_H

#include <string>

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
    virtual std::string toString() = 0;
};

// Base rendering context interface
class RenderingContext {
public:
    virtual ~RenderingContext() = default;
    
    // Add the toASCII method that was missing
    virtual std::string toASCII(int width, int height) = 0;
};

} // namespace rendering
} // namespace browser

#endif // BROWSER_RENDERING_RENDER_TARGET_H