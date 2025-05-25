// src/ui/window_macos.mm
#import <Cocoa/Cocoa.h>
#include "window_macos.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

// MacOS application delegate
@interface BrowserAppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation BrowserAppDelegate
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyRegular];
    [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
}
@end

// Window delegate
@interface BrowserWindowDelegate : NSObject <NSWindowDelegate> {
    browser::ui::MacOSWindow* m_browserWindow;
}
- (id)initWithBrowserWindow:(browser::ui::MacOSWindow*)browserWindow;
@end

@implementation BrowserWindowDelegate
- (id)initWithBrowserWindow:(browser::ui::MacOSWindow*)browserWindow {
    self = [super init];
    if (self) {
        m_browserWindow = browserWindow;
    }
    return self;
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
    m_browserWindow->handleCloseEvent();
    return NO; // We'll close the window ourselves
}

- (NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize {
    // Convert to content size
    NSRect contentRect = [sender contentRectForFrameRect:NSMakeRect(0, 0, frameSize.width, frameSize.height)];
    m_browserWindow->handleResizeEvent(contentRect.size.width, contentRect.size.height);
    return frameSize;
}

- (void)windowDidResize:(NSNotification*)notification {
    NSWindow* window = [notification object];
    NSRect contentRect = [window contentView].bounds;
    m_browserWindow->handleResizeEvent(contentRect.size.width, contentRect.size.height);
}
@end

// Custom view for drawing
@interface BrowserView : NSView {
    browser::ui::MacOSWindow* m_browserWindow;
    browser::ui::MacOSCanvas* m_canvas;
    NSTrackingArea* m_trackingArea;
}
- (id)initWithFrame:(NSRect)frame browserWindow:(browser::ui::MacOSWindow*)browserWindow canvas:(browser::ui::MacOSCanvas*)canvas;
- (void)updateTrackingAreas;
@end

@implementation BrowserView
- (id)initWithFrame:(NSRect)frame browserWindow:(browser::ui::MacOSWindow*)browserWindow canvas:(browser::ui::MacOSCanvas*)canvas {
    self = [super initWithFrame:frame];
    if (self) {
        m_browserWindow = browserWindow;
        m_canvas = canvas;
        
        // Set up tracking area for mouse events
        m_trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                options:(NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways)
                                                owner:self userInfo:nil];
        [self addTrackingArea:m_trackingArea];
    }
    return self;
}

- (void)dealloc {
    [m_trackingArea release];
    [super dealloc];
}

- (void)updateTrackingAreas {
    [self removeTrackingArea:m_trackingArea];
    [m_trackingArea release];
    
    m_trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                            options:(NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways)
                                            owner:self userInfo:nil];
    [self addTrackingArea:m_trackingArea];
    [super updateTrackingAreas];
}

- (BOOL)isFlipped {
    return YES; // Use top-left origin coordinate system
}

- (void)drawRect:(NSRect)dirtyRect {
    // Get drawing commands from the canvas
    std::vector<browser::ui::MacOSCanvas::DrawCommand> commands = m_canvas->getDrawCommands();
    
    // Create graphics context
    NSGraphicsContext* context = [NSGraphicsContext currentContext];
    [context saveGraphicsState];
    
    // Execute drawing commands
    for (const auto& cmd : commands) {
        switch (cmd.type) {
            case browser::ui::MacOSCanvas::DrawCommand::CLEAR: {
                // Extract color components
                unsigned char r = (cmd.color >> 16) & 0xFF;
                unsigned char g = (cmd.color >> 8) & 0xFF;
                unsigned char b = cmd.color & 0xFF;
                
                // Create NSColor
                NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
                [color set];
                
                // Fill the entire view
                NSRectFill([self bounds]);
                break;
            }
            
            case browser::ui::MacOSCanvas::DrawCommand::LINE: {
                // Extract color components
                unsigned char r = (cmd.color >> 16) & 0xFF;
                unsigned char g = (cmd.color >> 8) & 0xFF;
                unsigned char b = cmd.color & 0xFF;
                
                // Create NSColor
                NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
                
                // Create and configure path
                NSBezierPath* path = [NSBezierPath bezierPath];
                [path setLineWidth:cmd.thickness];
                [path moveToPoint:NSMakePoint(cmd.x1, cmd.y1)];
                [path lineToPoint:NSMakePoint(cmd.x2, cmd.y2)];
                
                // Draw the line
                [color set];
                [path stroke];
                break;
            }
            
            case browser::ui::MacOSCanvas::DrawCommand::RECT: {
                // Extract color components
                unsigned char r = (cmd.color >> 16) & 0xFF;
                unsigned char g = (cmd.color >> 8) & 0xFF;
                unsigned char b = cmd.color & 0xFF;
                
                // Create NSColor
                NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
                
                // Create rect
                NSRect rect = NSMakeRect(cmd.x1, cmd.y1, cmd.width, cmd.height);
                
                // Draw the rectangle
                [color set];
                if (cmd.filled) {
                    NSRectFill(rect);
                } else {
                    NSFrameRect(rect);
                }
                break;
            }
            
            case browser::ui::MacOSCanvas::DrawCommand::ELLIPSE: {
                // Extract color components
                unsigned char r = (cmd.color >> 16) & 0xFF;
                unsigned char g = (cmd.color >> 8) & 0xFF;
                unsigned char b = cmd.color & 0xFF;
                
                // Create NSColor
                NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
                
                // Create rect and path
                NSRect rect = NSMakeRect(cmd.x1, cmd.y1, cmd.width, cmd.height);
                NSBezierPath* path = [NSBezierPath bezierPathWithOvalInRect:rect];
                
                // Draw the ellipse
                [color set];
                if (cmd.filled) {
                    [path fill];
                } else {
                    [path setLineWidth:cmd.thickness];
                    [path stroke];
                }
                break;
            }
            
            case browser::ui::MacOSCanvas::DrawCommand::TEXT: {
                // Extract color components
                unsigned char r = (cmd.color >> 16) & 0xFF;
                unsigned char g = (cmd.color >> 8) & 0xFF;
                unsigned char b = cmd.color & 0xFF;
                
                // Create NSColor
                NSColor* color = [NSColor colorWithCalibratedRed:r/255.0 green:g/255.0 blue:b/255.0 alpha:1.0];
                
                // Create font
                NSString* fontName = [NSString stringWithUTF8String:cmd.fontName.c_str()];
                NSFont* font = [NSFont fontWithName:fontName size:cmd.fontSize];
                if (!font) {
                    font = [NSFont systemFontOfSize:cmd.fontSize];
                }
                
                // Create attributed string
                NSString* string = [NSString stringWithUTF8String:cmd.text.c_str()];
                NSMutableDictionary* attrs = [NSMutableDictionary dictionary];
                [attrs setObject:font forKey:NSFontAttributeName];
                [attrs setObject:color forKey:NSForegroundColorAttributeName];
                NSAttributedString* attrString = [[NSAttributedString alloc] 
                                               initWithString:string 
                                               attributes:attrs];
                
                // Draw the text
                [attrString drawAtPoint:NSMakePoint(cmd.x1, cmd.y1)];
                [attrString release];
                break;
            }
        }
    }
    
    [context restoreGraphicsState];
}

- (void)mouseDown:(NSEvent*)event {
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    m_browserWindow->handleMouseButtonEvent(browser::ui::MouseButton::Left, browser::ui::MouseAction::Press, point.x, point.y);
}

- (void)mouseUp:(NSEvent*)event {
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    m_browserWindow->handleMouseButtonEvent(browser::ui::MouseButton::Left, browser::ui::MouseAction::Release, point.x, point.y);
}

- (void)rightMouseDown:(NSEvent*)event {
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    m_browserWindow->handleMouseButtonEvent(browser::ui::MouseButton::Right, browser::ui::MouseAction::Press, point.x, point.y);
}

- (void)rightMouseUp:(NSEvent*)event {
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    m_browserWindow->handleMouseButtonEvent(browser::ui::MouseButton::Right, browser::ui::MouseAction::Release, point.x, point.y);
}

- (void)otherMouseDown:(NSEvent*)event {
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    m_browserWindow->handleMouseButtonEvent(browser::ui::MouseButton::Middle, browser::ui::MouseAction::Press, point.x, point.y);
}

- (void)otherMouseUp:(NSEvent*)event {
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    m_browserWindow->handleMouseButtonEvent(browser::ui::MouseButton::Middle, browser::ui::MouseAction::Release, point.x, point.y);
}

- (void)mouseMoved:(NSEvent*)event {
    NSPoint point = [self convertPoint:[event locationInWindow] fromView:nil];
    m_browserWindow->handleMouseMoveEvent(point.x, point.y);
}

- (void)mouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void)rightMouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void)otherMouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void)keyDown:(NSEvent*)event {
    m_browserWindow->handleKeyEvent([event keyCode], true);
}

- (void)keyUp:(NSEvent*)event {
    m_browserWindow->handleKeyEvent([event keyCode], false);
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)becomeFirstResponder {
    return YES;
}

- (BOOL)resignFirstResponder {
    return YES;
}
@end

namespace browser {
namespace ui {

//-----------------------------------------------------------------------------
// Initialize Cocoa application
//-----------------------------------------------------------------------------
static bool g_appInitialized = false;

static void initializeApp() {
    if (g_appInitialized) return;
    
    // Create autorelease pool
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    // Initialize Cocoa
    [NSApplication sharedApplication];
    
    // Create app delegate
    BrowserAppDelegate* appDelegate = [[BrowserAppDelegate alloc] init];
    [[NSApplication sharedApplication] setDelegate:appDelegate];
    
    // Activate the application
    [NSApp finishLaunching];
    
    [pool release];
    
    g_appInitialized = true;
}

//-----------------------------------------------------------------------------
// MacOSCanvas Implementation
//-----------------------------------------------------------------------------

MacOSCanvas::MacOSCanvas(int width, int height)
    : Canvas(width, height)
    , m_view(nil)
{
}

MacOSCanvas::~MacOSCanvas() {
    // Clean up font cache
    for (auto& pair : m_fontCache) {
        [pair.second release];
    }
    m_fontCache.clear();
    
    // Clean up color cache
    for (auto& pair : m_colorCache) {
        [pair.second release];
    }
    m_colorCache.clear();
}

void MacOSCanvas::initialize(NSView* view) {
    m_view = view;
}

void MacOSCanvas::clear(unsigned int color) {
    addDrawCommand(DrawCommand::Clear(color));
}

void MacOSCanvas::drawLine(int x1, int y1, int x2, int y2, unsigned int color, int thickness) {
    addDrawCommand(DrawCommand::Line(x1, y1, x2, y2, color, thickness));
}

void MacOSCanvas::drawRect(int x, int y, int width, int height, unsigned int color, bool filled, int thickness) {
    addDrawCommand(DrawCommand::Rect(x, y, width, height, color, filled, thickness));
}

void MacOSCanvas::drawEllipse(int x, int y, int width, int height, unsigned int color, bool filled, int thickness) {
    addDrawCommand(DrawCommand::Ellipse(x, y, width, height, color, filled, thickness));
}

void MacOSCanvas::drawText(const std::string& text, int x, int y, unsigned int color, const std::string& fontName, int fontSize) {
    addDrawCommand(DrawCommand::Text(text, x, y, color, fontName, fontSize));
}

void MacOSCanvas::setNeedsDisplay() {
    if (m_view) {
        [m_view setNeedsDisplay:YES];
    }
}

void MacOSCanvas::addDrawCommand(const DrawCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_drawMutex);
    m_drawCommands.push_back(cmd);
    setNeedsDisplay();
}

std::vector<MacOSCanvas::DrawCommand> MacOSCanvas::getDrawCommands() {
    std::lock_guard<std::mutex> lock(m_drawMutex);
    return m_drawCommands;
}

void MacOSCanvas::clearDrawCommands() {
    std::lock_guard<std::mutex> lock(m_drawMutex);
    m_drawCommands.clear();
}

NSColor* MacOSCanvas::getColor(unsigned int color) {
    // Check if color is already cached
    auto it = m_colorCache.find(color);
    if (it != m_colorCache.end()) {
        return it->second;
    }
    
    // Extract color components
    unsigned char r = (color >> 16) & 0xFF;
    unsigned char g = (color >> 8) & 0xFF;
    unsigned char b = color & 0xFF;
    
    // Create NSColor
    NSColor* nsColor = [[NSColor colorWithCalibratedRed:r/255.0
                                                  green:g/255.0
                                                   blue:b/255.0
                                                  alpha:1.0] retain];
    
    // Add to cache
    m_colorCache[color] = nsColor;
    
    return nsColor;
}

NSFont* MacOSCanvas::getFont(const std::string& fontName, int fontSize) {
    // Create a unique key for the font cache
    std::string key = fontName + "_" + std::to_string(fontSize);
    
    // Check if font is already cached
    auto it = m_fontCache.find(key);
    if (it != m_fontCache.end()) {
        return it->second;
    }
    
    // Map common font names to macOS font names
    std::string macFontName = fontName;
    if (fontName == "Arial") {
        macFontName = "Helvetica";
    } else if (fontName == "Times New Roman") {
        macFontName = "Times";
    } else if (fontName == "Courier New") {
        macFontName = "Courier";
    }
    
    // Create NSFont
    NSString* nsName = [NSString stringWithUTF8String:macFontName.c_str()];
    NSFont* font = [[NSFont fontWithName:nsName size:fontSize] retain];
    
    // If font not found, use system font
    if (!font) {
        font = [[NSFont systemFontOfSize:fontSize] retain];
    }
    
    // Add to cache
    m_fontCache[key] = font;
    
    return font;
}

//-----------------------------------------------------------------------------
// MacOSWindow Implementation
//-----------------------------------------------------------------------------

MacOSWindow::MacOSWindow(const WindowConfig& config)
    : Window(config)
    , m_window(nil)
    , m_view(nil)
    , m_delegate(nil)
    , m_resizing(false)
{
    m_canvas = std::make_shared<MacOSCanvas>(config.width, config.height);
    
    // Initialize the application
    initializeApp();
}

MacOSWindow::~MacOSWindow() {
    // Close window
    close();
}

bool MacOSWindow::create() {
    // Create autorelease pool
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    // Create window
    NSRect contentRect = NSMakeRect(0, 0, m_config.width, m_config.height);
    m_window = [[NSWindow alloc] initWithContentRect:contentRect
                                         styleMask:NSWindowStyleMaskTitled 
                                               | NSWindowStyleMaskClosable 
                                               | NSWindowStyleMaskMiniaturizable 
                                               | (m_config.resizable ? NSWindowStyleMaskResizable : 0)
                                         backing:NSBackingStoreBuffered
                                         defer:NO];
    
    if (!m_window) {
        std::cerr << "Failed to create macOS window" << std::endl;
        [pool release];
        return false;
    }
    
    // Set window properties
    [m_window setTitle:[NSString stringWithUTF8String:m_config.title.c_str()]];
    [m_window setReleasedWhenClosed:NO];
    [m_window center];
    
    // Create window delegate
    m_delegate = [[BrowserWindowDelegate alloc] initWithBrowserWindow:this];
    [m_window setDelegate:m_delegate];
    
    // Create view
    m_view = [[BrowserView alloc] initWithFrame:contentRect
                                 browserWindow:this
                                       canvas:m_canvas.get()];
    [m_window setContentView:m_view];
    
    // Initialize canvas
    m_canvas->initialize(m_view);
    
    [pool release];
    
    return true;
}

void MacOSWindow::show() {
    if (m_window) {
        [m_window makeKeyAndOrderFront:nil];
        
        // If maximized, maximize the window
        if (m_config.maximized) {
            [m_window zoom:nil];
        }
    }
}

void MacOSWindow::hide() {
    if (m_window) {
        [m_window orderOut:nil];
    }
}

void MacOSWindow::close() {
    // Release resources
    if (m_window) {
        [m_window setDelegate:nil];
        [m_window close];
        [m_window release];
        m_window = nil;
    }
    
    if (m_delegate) {
        [m_delegate release];
        m_delegate = nil;
    }
    
    if (m_view) {
        [m_view release];
        m_view = nil;
    }
}

bool MacOSWindow::processEvents() {
    // Create autorelease pool
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    // Process events
    NSEvent* event;
    do {
        event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                   untilDate:[NSDate distantPast]
                                      inMode:NSDefaultRunLoopMode
                                     dequeue:YES];
        if (event) {
            [NSApp sendEvent:event];
        }
    } while (event);
    
    [pool release];
    
    // Return false if window is closed
    return m_window != nil;
}

void* MacOSWindow::getNativeHandle() const {
    return (void*)m_window;
}

void MacOSWindow::beginPaint() {
    // Nothing to do for macOS
}

void MacOSWindow::endPaint() {
    if (m_canvas && m_view) {
        m_canvas->setNeedsDisplay();
    }
}

Canvas* MacOSWindow::getCanvas() {
    return m_canvas.get();
}

void MacOSWindow::setTitle(const std::string& title) {
    Window::setTitle(title);
    
    if (m_window) {
        [m_window setTitle:[NSString stringWithUTF8String:title.c_str()]];
    }
}

void MacOSWindow::setSize(int width, int height) {
    if (m_window) {
        NSRect contentRect = NSMakeRect(0, 0, width, height);
        NSRect frameRect = [m_window frameRectForContentRect:contentRect];
        [m_window setFrame:frameRect display:YES];
    }
    
    // Parent class handles updating m_config
    Window::setSize(width, height);
}

void MacOSWindow::setPosition(int x, int y) {
    if (m_window) {
        // macOS coordinates are bottom-left origin, convert to top-left
        NSRect screenRect = [[NSScreen mainScreen] frame];
        y = screenRect.size.height - y - m_config.height;
        
        [m_window setFrameOrigin:NSMakePoint(x, y)];
    }
}

void MacOSWindow::getPosition(int& x, int& y) const {
    if (m_window) {
        NSRect frame = [m_window frame];
        x = frame.origin.x;
        
        // macOS coordinates are bottom-left origin, convert to top-left
        NSRect screenRect = [[NSScreen mainScreen] frame];
        y = screenRect.size.height - frame.origin.y - frame.size.height;
    } else {
        x = 0;
        y = 0;
    }
}

void MacOSWindow::handleKeyEvent(int keyCode, bool pressed) {
    // Map key code to Key enum
    Key key = mapKeyCode(keyCode);
    
    // Create key action
    KeyAction action = pressed ? KeyAction::Press : KeyAction::Release;
    
    // Notify key event
    notifyKeyEvent(key, action);
}

void MacOSWindow::handleMouseButtonEvent(MouseButton button, MouseAction action, int x, int y) {
    // Notify mouse button event
    notifyMouseButtonEvent(button, action, x, y);
}

void MacOSWindow::handleMouseMoveEvent(int x, int y) {
    // Notify mouse move event
    notifyMouseMoveEvent(x, y);
}

void MacOSWindow::handleResizeEvent(int width, int height) {
    // Update canvas size
    m_canvas->setSize(width, height);
    
    // Update config
    m_config.width = width;
    m_config.height = height;
    
    // Clear draw commands to avoid artifacts
    m_canvas->clearDrawCommands();
    
    // Notify resize event
    notifyResizeEvent(width, height);
}

void MacOSWindow::handleCloseEvent() {
    // Notify close event
    notifyCloseEvent();
}

Key MacOSWindow::mapKeyCode(int keyCode) {
    // Map macOS key codes to Key enum
    // Reference: https://stackoverflow.com/questions/3202629/where-can-i-find-a-list-of-mac-virtual-key-codes
    switch (keyCode) {
        case 51: return Key::Backspace; // Delete
        case 48: return Key::Tab;
        case 36: return Key::Enter; // Return
        case 56: 
        case 60: return Key::Shift; // Left/right shift
        case 59:
        case 62: return Key::Control; // Left/right control
        case 58:
        case 61: return Key::Alt; // Left/right option (alt)
        case 53: return Key::Escape;
        case 49: return Key::Space;
        case 116: return Key::PageUp;
        case 121: return Key::PageDown;
        case 119: return Key::End;
        case 115: return Key::Home;
        case 123: return Key::Left;
        case 126: return Key::Up;
        case 124: return Key::Right;
        case 125: return Key::Down;
        case 114: return Key::Insert; // Help key as Insert
        case 117: return Key::Delete; // Forward delete
        
        // Function keys
        case 122: return Key::F1;
        case 120: return Key::F2;
        case 99: return Key::F3;
        case 118: return Key::F4;
        case 96: return Key::F5;
        case 97: return Key::F6;
        case 98: return Key::F7;
        case 100: return Key::F8;
        case 101: return Key::F9;
        case 109: return Key::F10;
        case 103: return Key::F11;
        case 111: return Key::F12;
        
        // Letters (convert to uppercase)
        case 0: return Key::A;
        case 11: return Key::B;
        case 8: return Key::C;
        case 2: return Key::D;
        case 14: return Key::E;
        case 3: return Key::F;
        case 5: return Key::G;
        case 4: return Key::H;
        case 34: return Key::I;
        case 38: return Key::J;
        case 40: return Key::K;
        case 37: return Key::L;
        case 46: return Key::M;
        case 45: return Key::N;
        case 31: return Key::O;
        case 35: return Key::P;
        case 12: return Key::Q;
        case 15: return Key::R;
        case 1: return Key::S;
        case 17: return Key::T;
        case 32: return Key::U;
        case 9: return Key::V;
        case 13: return Key::W;
        case 7: return Key::X;
        case 16: return Key::Y;
        case 6: return Key::Z;
        
        // Numbers
        case 29: return Key::Num0;
        case 18: return Key::Num1;
        case 19: return Key::Num2;
        case 20: return Key::Num3;
        case 21: return Key::Num4;
        case 23: return Key::Num5;
        case 22: return Key::Num6;
        case 26: return Key::Num7;
        case 28: return Key::Num8;
        case 25: return Key::Num9;
        
        default:
            return static_cast<Key>(0); // Unknown key
    }
}

} // namespace ui
} // namespace browser