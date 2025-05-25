// src/rendering/custom_renderer.cpp
#include "custom_renderer.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <math.h>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace browser {
namespace rendering {

//-----------------------------------------------------------------------------
// Path Implementation
//-----------------------------------------------------------------------------

void Path::moveTo(float x, float y) {
    Command cmd;
    cmd.type = CommandType::MOVE_TO;
    cmd.points[0] = x;
    cmd.points[1] = y;
    m_commands.push_back(cmd);
    
    m_lastX = x;
    m_lastY = y;
    m_startX = x;
    m_startY = y;
}

void Path::lineTo(float x, float y) {
    Command cmd;
    cmd.type = CommandType::LINE_TO;
    cmd.points[0] = x;
    cmd.points[1] = y;
    m_commands.push_back(cmd);
    
    m_lastX = x;
    m_lastY = y;
}

void Path::bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y) {
    Command cmd;
    cmd.type = CommandType::BEZIER_TO;
    cmd.points[0] = c1x;
    cmd.points[1] = c1y;
    cmd.points[2] = c2x;
    cmd.points[3] = c2y;
    cmd.points[4] = x;
    cmd.points[5] = y;
    m_commands.push_back(cmd);
    
    m_lastX = x;
    m_lastY = y;
}

void Path::quadTo(float cx, float cy, float x, float y) {
    Command cmd;
    cmd.type = CommandType::QUAD_TO;
    cmd.points[0] = cx;
    cmd.points[1] = cy;
    cmd.points[2] = x;
    cmd.points[3] = y;
    m_commands.push_back(cmd);
    
    m_lastX = x;
    m_lastY = y;
}

void Path::arcTo(float x1, float y1, float x2, float y2, float radius) {
    Command cmd;
    cmd.type = CommandType::ARC_TO;
    cmd.points[0] = x1;
    cmd.points[1] = y1;
    cmd.points[2] = x2;
    cmd.points[3] = y2;
    cmd.points[4] = radius;
    m_commands.push_back(cmd);
    
    // ArcTo is complex - in a real implementation we'd calculate the end point
    // For now we'll just use x2,y2 as an approximation
    m_lastX = x2;
    m_lastY = y2;
}

void Path::closePath() {
    Command cmd;
    cmd.type = CommandType::CLOSE;
    m_commands.push_back(cmd);
    
    // Return to the start of the path
    m_lastX = m_startX;
    m_lastY = m_startY;
}

void Path::rect(float x, float y, float w, float h) {
    moveTo(x, y);
    lineTo(x + w, y);
    lineTo(x + w, y + h);
    lineTo(x, y + h);
    closePath();
}

void Path::roundedRect(float x, float y, float w, float h, float r) {
    // Clamp radius to half of smallest dimension
    r = std::min(r, std::min(w, h) * 0.5f);
    
    moveTo(x + r, y);
    lineTo(x + w - r, y);
    arcTo(x + w, y, x + w, y + r, r);
    lineTo(x + w, y + h - r);
    arcTo(x + w, y + h, x + w - r, y + h, r);
    lineTo(x + r, y + h);
    arcTo(x, y + h, x, y + h - r, r);
    lineTo(x, y + r);
    arcTo(x, y, x + r, y, r);
}

void Path::ellipse(float cx, float cy, float rx, float ry) {
    // Approximate ellipse with bezier curves
    float kappa = 0.5522848f; // 4 * (sqrt(2) - 1) / 3
    float ox = rx * kappa;    // Control point offset x
    float oy = ry * kappa;    // Control point offset y
    
    moveTo(cx - rx, cy);
    bezierTo(cx - rx, cy - oy, cx - ox, cy - ry, cx, cy - ry);
    bezierTo(cx + ox, cy - ry, cx + rx, cy - oy, cx + rx, cy);
    bezierTo(cx + rx, cy + oy, cx + ox, cy + ry, cx, cy + ry);
    bezierTo(cx - ox, cy + ry, cx - rx, cy + oy, cx - rx, cy);
    closePath();
}

void Path::circle(float cx, float cy, float r) {
    ellipse(cx, cy, r, r);
}

void Path::clear() {
    m_commands.clear();
    m_lastX = m_lastY = m_startX = m_startY = 0.0f;
}

bool Path::isEmpty() const {
    return m_commands.empty();
}

void Path::getBounds(float& x, float& y, float& w, float& h) const {
    if (isEmpty()) {
        x = y = w = h = 0.0f;
        return;
    }
    
    // Initialize with first point
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    
    // Track current position
    float curX = 0.0f, curY = 0.0f;
    float startX = 0.0f, startY = 0.0f;
    
    for (const auto& cmd : m_commands) {
        switch (cmd.type) {
            case CommandType::MOVE_TO:
                curX = cmd.points[0];
                curY = cmd.points[1];
                startX = curX;
                startY = curY;
                minX = std::min(minX, curX);
                minY = std::min(minY, curY);
                maxX = std::max(maxX, curX);
                maxY = std::max(maxY, curY);
                break;
                
            case CommandType::LINE_TO:
                curX = cmd.points[0];
                curY = cmd.points[1];
                minX = std::min(minX, curX);
                minY = std::min(minY, curY);
                maxX = std::max(maxX, curX);
                maxY = std::max(maxY, curY);
                break;
                
            case CommandType::BEZIER_TO:
                // Include control points in bounds
                minX = std::min(minX, cmd.points[0]);
                minY = std::min(minY, cmd.points[1]);
                maxX = std::max(maxX, cmd.points[0]);
                maxY = std::max(maxY, cmd.points[1]);
                
                minX = std::min(minX, cmd.points[2]);
                minY = std::min(minY, cmd.points[3]);
                maxX = std::max(maxX, cmd.points[2]);
                maxY = std::max(maxY, cmd.points[3]);
                
                curX = cmd.points[4];
                curY = cmd.points[5];
                minX = std::min(minX, curX);
                minY = std::min(minY, curY);
                maxX = std::max(maxX, curX);
                maxY = std::max(maxY, curY);
                break;
                
            case CommandType::QUAD_TO:
                // Include control point in bounds
                minX = std::min(minX, cmd.points[0]);
                minY = std::min(minY, cmd.points[1]);
                maxX = std::max(maxX, cmd.points[0]);
                maxY = std::max(maxY, cmd.points[1]);
                
                curX = cmd.points[2];
                curY = cmd.points[3];
                minX = std::min(minX, curX);
                minY = std::min(minY, curY);
                maxX = std::max(maxX, curX);
                maxY = std::max(maxY, curY);
                break;
                
            case CommandType::ARC_TO:
                // This is an approximation - in a real implementation we'd
                // calculate the actual bounds of the arc
                minX = std::min(minX, cmd.points[0]);
                minY = std::min(minY, cmd.points[1]);
                maxX = std::max(maxX, cmd.points[0]);
                maxY = std::max(maxY, cmd.points[1]);
                
                minX = std::min(minX, cmd.points[2]);
                minY = std::min(minY, cmd.points[3]);
                maxX = std::max(maxX, cmd.points[2]);
                maxY = std::max(maxY, cmd.points[3]);
                
                // Account for the radius
                minX = std::min(minX, curX - cmd.points[4]);
                minY = std::min(minY, curY - cmd.points[4]);
                maxX = std::max(maxX, curX + cmd.points[4]);
                maxY = std::max(maxY, curY + cmd.points[4]);
                
                curX = cmd.points[2];
                curY = cmd.points[3];
                break;
                
            case CommandType::CLOSE:
                curX = startX;
                curY = startY;
                break;
        }
    }
    
    x = minX;
    y = minY;
    w = maxX - minX;
    h = maxY - minY;
}

//-----------------------------------------------------------------------------
// Paint Implementation
//-----------------------------------------------------------------------------

Paint Paint::linearGradient(float sx, float sy, float ex, float ey, 
                          const Color& startColor, const Color& endColor) {
    Paint paint;
    paint.type = PaintType::LINEAR_GRADIENT;
    paint.startX = sx;
    paint.startY = sy;
    paint.endX = ex;
    paint.endY = ey;
    
    // Add gradient stops
    GradientStop startStop = {0.0f, startColor};
    GradientStop endStop = {1.0f, endColor};
    paint.stops.push_back(startStop);
    paint.stops.push_back(endStop);
    
    return paint;
}

Paint Paint::radialGradient(float cx, float cy, float inr, float outr,
                          const Color& innerColor, const Color& outerColor) {
    Paint paint;
    paint.type = PaintType::RADIAL_GRADIENT;
    paint.startX = cx;
    paint.startY = cy;
    paint.innerRadius = inr;
    paint.outerRadius = outr;
    
    // Add gradient stops
    GradientStop innerStop = {0.0f, innerColor};
    GradientStop outerStop = {1.0f, outerColor};
    paint.stops.push_back(innerStop);
    paint.stops.push_back(outerStop);
    
    return paint;
}

Paint Paint::imagePattern(const Image& image, float x, float y, float width, float height,
                        float angle, float alpha) {
    Paint paint;
    paint.type = PaintType::IMAGE_PATTERN;
    paint.image = std::make_shared<Image>(image);
    paint.patternX = x;
    paint.patternY = y;
    paint.patternWidth = width;
    paint.patternHeight = height;
    paint.patternAngle = angle;
    paint.patternAlpha = alpha;
    
    return paint;
}

//-----------------------------------------------------------------------------
// Image Implementation
//-----------------------------------------------------------------------------

bool Image::load(const std::string& filename) {
    // A real implementation would load an image from disk
    // For simplicity, we'll create a placeholder image
    m_width = 100;
    m_height = 100;
    m_data.resize(m_width * m_height * 4);
    
    // Fill with a checkerboard pattern
    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            int idx = (y * m_width + x) * 4;
            bool isLight = ((x / 10) + (y / 10)) % 2 == 0;
            
            m_data[idx + 0] = isLight ? 200 : 100; // R
            m_data[idx + 1] = isLight ? 200 : 100; // G
            m_data[idx + 2] = isLight ? 200 : 100; // B
            m_data[idx + 3] = 255; // A
        }
    }
    
    return true;
}

bool Image::save(const std::string& filename) const {
    // A real implementation would save the image to disk
    std::cout << "Saving image to " << filename << std::endl;
    return true;
}

//-----------------------------------------------------------------------------
// CustomRenderContext Implementation
//-----------------------------------------------------------------------------
void browser::rendering::Paint::setColor(const Color& c) {
    type = PaintType::COLOR;
    color = c;
}

CustomRenderContext::CustomRenderContext()
    : m_nextImageId(1)
    , m_width(0)
    , m_height(0)
    , m_devicePixelRatio(1.0f)
    , m_inFrame(false)
{
    // Initialize with default state
    m_stateStack.push_back(m_currentState);
}

CustomRenderContext::~CustomRenderContext() {
    // Clean up images
    m_images.clear();
}

void CustomRenderContext::save() {
    m_stateStack.push_back(m_currentState);
}

void CustomRenderContext::restore() {
    if (m_stateStack.size() > 1) {
        m_currentState = m_stateStack.back();
        m_stateStack.pop_back();
    }
}

void CustomRenderContext::reset() {
    m_stateStack.clear();
    m_currentState = State();
    m_stateStack.push_back(m_currentState);
}

void CustomRenderContext::translate(float x, float y) {
    Transform t;
    t.e = x;
    t.f = y;
    m_currentState.transform = m_currentState.transform.multiply(t);
}

void CustomRenderContext::rotate(float angle) {
    float s = std::sin(angle);
    float c = std::cos(angle);
    Transform t(c, s, -s, c, 0, 0);
    m_currentState.transform = m_currentState.transform.multiply(t);
}

void CustomRenderContext::scale(float x, float y) {
    Transform t(x, 0, 0, y, 0, 0);
    m_currentState.transform = m_currentState.transform.multiply(t);
}

void CustomRenderContext::transform(float a, float b, float c, float d, float e, float f) {
    Transform t(a, b, c, d, e, f);
    m_currentState.transform = m_currentState.transform.multiply(t);
}

void CustomRenderContext::resetTransform() {
    m_currentState.transform.identity();
}

void CustomRenderContext::beginPath() {
    m_currentPath.clear();
}

void CustomRenderContext::closePath() {
    m_currentPath.closePath();
}

void CustomRenderContext::moveTo(float x, float y) {
    m_currentState.transform.apply(x, y);
    m_currentPath.moveTo(x, y);
}

void CustomRenderContext::lineTo(float x, float y) {
    m_currentState.transform.apply(x, y);
    m_currentPath.lineTo(x, y);
}

void CustomRenderContext::bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y) {
    m_currentState.transform.apply(c1x, c1y);
    m_currentState.transform.apply(c2x, c2y);
    m_currentState.transform.apply(x, y);
    m_currentPath.bezierTo(c1x, c1y, c2x, c2y, x, y);
}

void CustomRenderContext::quadTo(float cx, float cy, float x, float y) {
    m_currentState.transform.apply(cx, cy);
    m_currentState.transform.apply(x, y);
    m_currentPath.quadTo(cx, cy, x, y);
}

void CustomRenderContext::arcTo(float x1, float y1, float x2, float y2, float radius) {
    m_currentState.transform.apply(x1, y1);
    m_currentState.transform.apply(x2, y2);
    m_currentPath.arcTo(x1, y1, x2, y2, radius);
}

void CustomRenderContext::arc(float cx, float cy, float r, float a0, float a1, int dir) {
    // Transform center and radius
    m_currentState.transform.apply(cx, cy);
    
    // Calculate arc start point
    float x0 = cx + r * std::cos(a0);
    float y0 = cy + r * std::sin(a0);
    
    // Move to start of arc
    m_currentPath.moveTo(x0, y0);
    
    // Approximate arc with bezier curves
    float theta = a1 - a0;
    int segmentEstimate = static_cast<int>(std::abs(theta) * 36.0f / (2.0f * M_PI));
    int segments = std::max(1, std::min(36, segmentEstimate));
    
    float step = theta / segments;
    float angle = a0;
    
    for (int i = 0; i < segments; i++) {
        float nextAngle = angle + step;
        float x1 = cx + r * std::cos(nextAngle);
        float y1 = cy + r * std::sin(nextAngle);
        
        // Calculate control points for the bezier curve
        float t = std::tan(step * 0.5f);
        float x2 = x0 - t * (y0 - cy);
        float y2 = y0 + t * (x0 - cx);
        float x3 = x1 + t * (y1 - cy);
        float y3 = y1 - t * (x1 - cx);
        
        m_currentPath.bezierTo(x2, y2, x3, y3, x1, y1);
        
        x0 = x1;
        y0 = y1;
        angle = nextAngle;
    }
}

void CustomRenderContext::rect(float x, float y, float w, float h) {
    // Transform corners
    float x0 = x;
    float y0 = y;
    m_currentState.transform.apply(x0, y0);
    
    float x1 = x + w;
    float y1 = y;
    m_currentState.transform.apply(x1, y1);
    
    float x2 = x + w;
    float y2 = y + h;
    m_currentState.transform.apply(x2, y2);
    
    float x3 = x;
    float y3 = y + h;
    m_currentState.transform.apply(x3, y3);
    
    // Add rectangle to path
    m_currentPath.moveTo(x0, y0);
    m_currentPath.lineTo(x1, y1);
    m_currentPath.lineTo(x2, y2);
    m_currentPath.lineTo(x3, y3);
    m_currentPath.closePath();
}

void CustomRenderContext::roundedRect(float x, float y, float w, float h, float r) {
    // A real implementation would transform each point
    // For simplicity, we'll just transform the whole rectangle
    float transformedX = x;
    float transformedY = y;
    m_currentState.transform.apply(transformedX, transformedY);
    
    m_currentPath.roundedRect(transformedX, transformedY, w, h, r);
}

void CustomRenderContext::ellipse(float cx, float cy, float rx, float ry) {
    // Transform center
    m_currentState.transform.apply(cx, cy);
    
    // Transform radii (approximation)
    float tx = rx;
    float ty = 0;
    m_currentState.transform.apply(tx, ty);
    rx = std::sqrt(tx * tx + ty * ty);
    
    tx = 0;
    ty = ry;
    m_currentState.transform.apply(tx, ty);
    ry = std::sqrt(tx * tx + ty * ty);
    
    m_currentPath.ellipse(cx, cy, rx, ry);
}

void CustomRenderContext::circle(float cx, float cy, float r) {
    ellipse(cx, cy, r, r);
}

void CustomRenderContext::fill() {
    // Store the current path for rendering
    m_paths.push_back(m_currentPath);
    
    // In a real implementation, we would actually render the path here
    // using the current fill paint
}

void CustomRenderContext::stroke() {
    // Store the current path for rendering
    m_paths.push_back(m_currentPath);
    
    // In a real implementation, we would actually render the path here
    // using the current stroke paint and stroke width
}

void CustomRenderContext::setFillPaint(const Paint& paint) {
    m_currentState.fillPaint = paint;
}

void CustomRenderContext::setStrokePaint(const Paint& paint) {
    m_currentState.strokePaint = paint;
}

void CustomRenderContext::setStrokeWidth(float width) {
    m_currentState.strokeWidth = width;
}

void CustomRenderContext::setFont(const Font& font) {
    m_currentState.font = font;
}

float CustomRenderContext::text(float x, float y, const std::string& string) {
    // Transform text position
    m_currentState.transform.apply(x, y);
    
    // In a real implementation, we would render the text
    // For now, just return an estimate of text width
    return string.length() * m_currentState.font.size() * 0.6f;
}

float CustomRenderContext::textBounds(float x, float y, const std::string& string, float* bounds) {
    // Transform text position
    m_currentState.transform.apply(x, y);
    
    // Calculate approximate text bounds
    float width = string.length() * m_currentState.font.size() * 0.6f;
    float height = m_currentState.font.size();
    
    if (bounds) {
        bounds[0] = x;
        bounds[1] = y - height * 0.75f; // Approximate ascender
        bounds[2] = width;
        bounds[3] = height;
    }
    
    return width;
}

void CustomRenderContext::textMetrics(float* ascender, float* descender, float* lineHeight) {
    float size = m_currentState.font.size();
    
    if (ascender) *ascender = size * 0.75f;
    if (descender) *descender = size * 0.25f;
    if (lineHeight) *lineHeight = size * 1.25f;
}

int CustomRenderContext::createImage(const std::string& filename) {
    auto image = std::make_shared<Image>();
    
    if (!image->load(filename)) {
        return 0; // Failed to load
    }
    
    int id = m_nextImageId++;
    m_images[id] = image;
    
    return id;
}

int CustomRenderContext::createImageFromMemory(const unsigned char* data, int ndata) {
    // A real implementation would parse the image data
    // For simplicity, we'll just create a placeholder
    auto image = std::make_shared<Image>(100, 100);
    
    int id = m_nextImageId++;
    m_images[id] = image;
    
    return id;
}

int CustomRenderContext::createImageRGBA(int w, int h, const unsigned char* data) {
    auto image = std::make_shared<Image>(w, h);
    
    if (data) {
        // Copy image data
        std::memcpy(image->data(), data, w * h * 4);
    }
    
    int id = m_nextImageId++;
    m_images[id] = image;
    
    return id;
}

void CustomRenderContext::updateImage(int image, const unsigned char* data) {
    auto it = m_images.find(image);
    
    if (it != m_images.end() && data) {
        // Copy image data
        std::memcpy(it->second->data(), data, 
                   it->second->width() * it->second->height() * 4);
    }
}

void CustomRenderContext::imageSize(int image, int* w, int* h) {
    auto it = m_images.find(image);
    
    if (it != m_images.end()) {
        if (w) *w = it->second->width();
        if (h) *h = it->second->height();
    } else {
        if (w) *w = 0;
        if (h) *h = 0;
    }
}

void CustomRenderContext::deleteImage(int image) {
    m_images.erase(image);
}

void CustomRenderContext::scissor(float x, float y, float w, float h) {
    // Transform scissor rectangle
    float x0 = x;
    float y0 = y;
    m_currentState.transform.apply(x0, y0);
    
    float x1 = x + w;
    float y1 = y + h;
    m_currentState.transform.apply(x1, y1);
    
    // Set scissor rect
    m_currentState.scissorX = std::min(x0, x1);
    m_currentState.scissorY = std::min(y0, y1);
    m_currentState.scissorWidth = std::abs(x1 - x0);
    m_currentState.scissorHeight = std::abs(y1 - y0);
    m_currentState.scissoring = true;
}

void CustomRenderContext::resetScissor() {
    m_currentState.scissoring = false;
}

void CustomRenderContext::beginFrame(int windowWidth, int windowHeight, float devicePixelRatio) {
    m_width = windowWidth;
    m_height = windowHeight;
    m_devicePixelRatio = devicePixelRatio;
    m_inFrame = true;
    
    // Clear path data
    m_paths.clear();
    m_currentPath.clear();
    
    // Reset state
    reset();
}

void CustomRenderContext::cancelFrame() {
    m_inFrame = false;
}

void CustomRenderContext::endFrame() {
    m_inFrame = false;
}

} // namespace rendering
} // namespace browser