// src/rendering/custom_renderer.h
#ifndef BROWSER_CUSTOM_RENDERER_H
#define BROWSER_CUSTOM_RENDERER_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <sstream>
#include "../css/style_resolver.h" // Added for access to css::Value

namespace browser {
namespace rendering {

// Forward declarations
class Path;
class Font;
class Image;

// Color class (RGBA)
class Color {
public:
    Color() : r(0), g(0), b(0), a(1.0f) {}
    Color(unsigned char r, unsigned char g, unsigned char b, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {}
    
    static Color fromRGBA(unsigned int rgba) {
        return Color(
            (rgba >> 24) & 0xFF,
            (rgba >> 16) & 0xFF,
            (rgba >> 8) & 0xFF,
            (rgba & 0xFF) / 255.0f
        );
    }
    
    static Color fromRGB(unsigned int rgb) {
        return Color(
            (rgb >> 16) & 0xFF,
            (rgb >> 8) & 0xFF,
            rgb & 0xFF
        );
    }
    
    // Parse a CSS color value
    static Color fromCssColor(const css::Value& value) {
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
    
    unsigned int toRGBA() const {
        return ((unsigned int)r << 24) | 
               ((unsigned int)g << 16) | 
               ((unsigned int)b << 8) | 
               (unsigned int)(a * 255.0f);
    }
    
    unsigned char r, g, b;
    float a;
};

// Paint type
enum class PaintType {
    NONE,
    COLOR,
    LINEAR_GRADIENT,
    RADIAL_GRADIENT,
    IMAGE_PATTERN
};

// Paint class (for fills and strokes)
class Paint {
public:
    Paint() : type(PaintType::NONE) {}
    Paint(const Color& color) : type(PaintType::COLOR), color(color) {}
    
    // Create linear gradient paint
    static Paint linearGradient(float sx, float sy, float ex, float ey, 
                              const Color& startColor, const Color& endColor);
    
    // Create radial gradient paint
    static Paint radialGradient(float cx, float cy, float inr, float outr,
                              const Color& innerColor, const Color& outerColor);
    
    // Create image pattern paint
    static Paint imagePattern(const Image& image, float x, float y, float width, float height,
                            float angle, float alpha);
    
    // Setter for color (added to support easier construction)
    void setColor(const Color& c);
    
    PaintType type;
    Color color;
    
    // Gradient colors and stops
    struct GradientStop {
        float pos;
        Color color;
    };
    std::vector<GradientStop> stops;
    
    // Gradient parameters
    float startX, startY;
    float endX, endY;
    float innerRadius, outerRadius;
    
    // Image pattern parameters
    std::shared_ptr<Image> image;
    float patternX, patternY;
    float patternWidth, patternHeight;
    float patternAngle;
    float patternAlpha;
};

// Path class for vector shapes
class Path {
public:
    Path() {}
    ~Path() {}
    
    // Path commands
    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y);
    void quadTo(float cx, float cy, float x, float y);
    void arcTo(float x1, float y1, float x2, float y2, float radius);
    void closePath();
    
    // Path creation helpers
    void rect(float x, float y, float w, float h);
    void roundedRect(float x, float y, float w, float h, float r);
    void ellipse(float cx, float cy, float rx, float ry);
    void circle(float cx, float cy, float r);
    
    // Path operations
    void clear();
    bool isEmpty() const;
    void getBounds(float& x, float& y, float& w, float& h) const;
    
    // Path data access
    enum class CommandType {
        MOVE_TO,
        LINE_TO,
        BEZIER_TO,
        QUAD_TO,
        ARC_TO,
        CLOSE
    };
    
    struct Command {
        CommandType type;
        float points[6]; // x,y or c1x,c1y,c2x,c2y,x,y or cx,cy,x,y or x1,y1,x2,y2,r
    };
    
    const std::vector<Command>& commands() const { return m_commands; }
    
private:
    std::vector<Command> m_commands;
    float m_lastX = 0.0f, m_lastY = 0.0f; // Last position
    float m_startX = 0.0f, m_startY = 0.0f; // Start of current subpath
};

// Font class
class Font {
public:
    Font() : m_size(12.0f) {}
    Font(const std::string& name, float size) : m_name(name), m_size(size) {}
    
    const std::string& name() const { return m_name; }
    float size() const { return m_size; }
    
    void setName(const std::string& name) { m_name = name; }
    void setSize(float size) { m_size = size; }
    
private:
    std::string m_name;
    float m_size;
};

// Image class
class Image {
public:
    Image() : m_width(0), m_height(0) {}
    Image(int width, int height) : m_width(width), m_height(height) {
        m_data.resize(width * height * 4, 0);
    }
    
    int width() const { return m_width; }
    int height() const { return m_height; }
    
    const unsigned char* data() const { return m_data.data(); }
    unsigned char* data() { return m_data.data(); }
    
    bool load(const std::string& filename);
    bool save(const std::string& filename) const;
    
private:
    int m_width, m_height;
    std::vector<unsigned char> m_data; // RGBA data
};

// Custom renderer context (replacement for NVGcontext)
class CustomRenderContext {
public:
    CustomRenderContext();
    ~CustomRenderContext();
    
    // State management
    void save();
    void restore();
    void reset();
    
    // Transform operations
    void translate(float x, float y);
    void rotate(float angle);
    void scale(float x, float y);
    void transform(float a, float b, float c, float d, float e, float f);
    void resetTransform();
    
    // Path operations
    void beginPath();
    void closePath();
    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void bezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y);
    void quadTo(float cx, float cy, float x, float y);
    void arcTo(float x1, float y1, float x2, float y2, float radius);
    void arc(float cx, float cy, float r, float a0, float a1, int dir);
    void rect(float x, float y, float w, float h);
    void roundedRect(float x, float y, float w, float h, float r);
    void ellipse(float cx, float cy, float rx, float ry);
    void circle(float cx, float cy, float r);
    void fill();
    void stroke();
    
    // Paint operations
    void setFillPaint(const Paint& paint);
    void setStrokePaint(const Paint& paint);
    void setStrokeWidth(float width);
    
    // Text operations
    void setFont(const Font& font);
    float text(float x, float y, const std::string& string);
    float textBounds(float x, float y, const std::string& string, float* bounds);
    void textMetrics(float* ascender, float* descender, float* lineHeight);
    
    // Image operations
    int createImage(const std::string& filename);
    int createImageFromMemory(const unsigned char* data, int ndata);
    int createImageRGBA(int w, int h, const unsigned char* data);
    void updateImage(int image, const unsigned char* data);
    void imageSize(int image, int* w, int* h);
    void deleteImage(int image);
    
    // Scissoring
    void scissor(float x, float y, float w, float h);
    void resetScissor();
    
    // Rendering to target
    void beginFrame(int windowWidth, int windowHeight, float devicePixelRatio);
    void cancelFrame();
    void endFrame();
    
    // Access to internal state
    const std::vector<Path>& paths() const { return m_paths; }
    const Path& currentPath() const { return m_currentPath; }
    
    // Get window dimensions
    int getWindowWidth() const { return m_width; }
    int getWindowHeight() const { return m_height; }
    
private:
    // Transform matrix (3x3)
    struct Transform {
        float a, b, c, d, e, f;
        
        Transform() : a(1.0f), b(0.0f), c(0.0f), d(1.0f), e(0.0f), f(0.0f) {}
        Transform(float a, float b, float c, float d, float e, float f)
            : a(a), b(b), c(c), d(d), e(e), f(f) {}
            
        void identity() {
            a = 1.0f; b = 0.0f;
            c = 0.0f; d = 1.0f;
            e = 0.0f; f = 0.0f;
        }
        
        Transform multiply(const Transform& other) const {
            return Transform(
                a * other.a + c * other.b,
                b * other.a + d * other.b,
                a * other.c + c * other.d,
                b * other.c + d * other.d,
                a * other.e + c * other.f + e,
                b * other.e + d * other.f + f
            );
        }
        
        void apply(float& x, float& y) const {
            float tx = x;
            float ty = y;
            x = a * tx + c * ty + e;
            y = b * tx + d * ty + f;
        }
    };
    
    // Drawing state
    struct State {
        Transform transform;
        Paint fillPaint;
        Paint strokePaint;
        float strokeWidth;
        Font font;
        float scissorX, scissorY, scissorWidth, scissorHeight;
        bool scissoring;
        
        State() : strokeWidth(1.0f), scissorX(0), scissorY(0), 
                 scissorWidth(0), scissorHeight(0), scissoring(false) {}
    };
    
    // State stack
    std::vector<State> m_stateStack;
    State m_currentState;
    
    // Path data
    Path m_currentPath;
    std::vector<Path> m_paths;
    
    // Image storage
    std::map<int, std::shared_ptr<Image>> m_images;
    int m_nextImageId;
    
    // Frame data
    int m_width, m_height;
    float m_devicePixelRatio;
    bool m_inFrame;
};

} // namespace rendering
} // namespace browser

#endif // BROWSER_CUSTOM_RENDERER_H