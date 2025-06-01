// test_ui_rendering.cpp - Test file for UI and rendering components
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

// Include necessary headers
#include "ui/window.h"
#include "ui/browser_window.h"
#include "ui/custom_canvas.h"
#include "ui/custom_controls.h"
#include "rendering/custom_renderer.h"
#include "rendering/custom_render_target.h"
#include "rendering/renderer.h"
#include "rendering/paint_system.h"
#include "layout/box_model.h"
#include "css/style_resolver.h"
#include "html/dom_tree.h"

using namespace browser;
using namespace browser::ui;
using namespace browser::rendering;
using namespace browser::layout;

// Test 1: Basic Window Creation
bool testBasicWindow() {
    std::cout << "\n=== Test 1: Basic Window Creation ===" << std::endl;
    
    WindowConfig config;
    config.title = "Test Window - Close to continue";
    config.width = 800;
    config.height = 600;
    
    auto window = createPlatformWindow(config);
    if (!window) {
        std::cerr << "Failed to create platform window" << std::endl;
        return false;
    }
    
    if (!window->create()) {
        std::cerr << "Failed to initialize window" << std::endl;
        return false;
    }
    
    std::cout << "Window created successfully" << std::endl;
    
    // Show window
    window->show();
    
    std::cout << "Close the window to continue to the next test..." << std::endl;
    
    // Process events until window is closed
    while (window->processEvents()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    std::cout << "Test 1 PASSED" << std::endl;
    return true;
}

// Test 2: Canvas Drawing
bool testCanvasDrawing() {
    std::cout << "\n=== Test 2: Canvas Drawing ===" << std::endl;
    
    WindowConfig config;
    config.title = "Canvas Drawing Test - Close to continue";
    config.width = 800;
    config.height = 600;
    
    auto window = createPlatformWindow(config);
    if (!window || !window->create()) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }
    
    window->show();
    
    std::cout << "Drawing shapes on canvas..." << std::endl;
    std::cout << "Close the window to continue to the next test..." << std::endl;
    
    // Draw on canvas
    auto drawTest = [&window]() {
        window->beginPaint();
        Canvas* canvas = window->getCanvas();
        
        if (!canvas) {
            std::cerr << "Canvas is null!" << std::endl;
            return false;
        }
        
        // Clear background
        canvas->clear(Canvas::rgb(255, 255, 255));
        
        // Draw some test shapes
        canvas->drawRect(50, 50, 200, 100, Canvas::rgb(255, 0, 0), true);
        canvas->drawRect(300, 50, 200, 100, Canvas::rgb(0, 255, 0), false, 3);
        canvas->drawEllipse(50, 200, 200, 100, Canvas::rgb(0, 0, 255), true);
        canvas->drawLine(50, 350, 250, 450, Canvas::rgb(255, 0, 255), 5);
        canvas->drawText("Canvas Drawing Test", 300, 250, Canvas::rgb(0, 0, 0), "Arial", 24);
        
        window->endPaint();
        return true;
    };
    
    // Initial draw
    if (!drawTest()) {
        window->close();
        return false;
    }
    
    // Process events until window is closed
    while (window->processEvents()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    std::cout << "Test 2 PASSED" << std::endl;
    return true;
}

// Test 3: Custom Renderer
bool testCustomRenderer() {
    std::cout << "\n=== Test 3: Custom Renderer ===" << std::endl;
    
    // Create custom render context
    auto ctx = std::make_shared<CustomRenderContext>();
    
    // Begin frame
    ctx->beginFrame(800, 600, 1.0f);
    
    // Draw some shapes
    ctx->beginPath();
    ctx->rect(100, 100, 200, 150);
    Paint fillPaint;
    fillPaint.setColor(Color(255, 128, 0)); // Orange
    ctx->setFillPaint(fillPaint);
    ctx->fill();
    
    ctx->beginPath();
    ctx->circle(400, 175, 75);
    Paint strokePaint;
    strokePaint.setColor(Color(0, 128, 255)); // Blue
    ctx->setStrokePaint(strokePaint);
    ctx->setStrokeWidth(3.0f);
    ctx->stroke();
    
    // Draw text
    ctx->setFont(Font("Arial", 20.0f));
    Paint textPaint;
    textPaint.setColor(Color(0, 0, 0));
    ctx->setFillPaint(textPaint);
    ctx->text(100, 300, "Custom Renderer Test");
    
    ctx->endFrame();
    
    std::cout << "Custom renderer test completed" << std::endl;
    std::cout << "Test 3 PASSED" << std::endl;
    return true;
}

// Test 4: Custom Canvas with Window
bool testCustomCanvas() {
    std::cout << "\n=== Test 4: Custom Canvas with Window ===" << std::endl;
    
    WindowConfig config;
    config.title = "Custom Canvas Test - Close to continue";
    config.width = 800;
    config.height = 600;
    
    auto window = createPlatformWindow(config);
    if (!window || !window->create()) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }
    
    // Create custom canvas
    auto customCanvas = std::make_shared<CustomCanvas>(config.width, config.height);
    if (!customCanvas->initialize()) {
        std::cerr << "Failed to initialize custom canvas" << std::endl;
        window->close();
        return false;
    }
    
    window->show();
    
    std::cout << "Drawing with custom canvas..." << std::endl;
    std::cout << "Close the window to continue to the next test..." << std::endl;
    
    // Draw using custom canvas
    auto drawCustom = [&window, &customCanvas]() {
        window->beginPaint();
        
        customCanvas->beginFrame();
        
        // Clear background
        customCanvas->clear(Canvas::rgb(240, 240, 240));
        
        // Draw shapes
        customCanvas->drawRect(50, 50, 300, 200, Canvas::rgb(100, 150, 255), true);
        customCanvas->drawEllipse(400, 50, 300, 200, Canvas::rgb(255, 150, 100), false, 3);
        customCanvas->drawText("Custom Canvas Test", 250, 300, Canvas::rgb(0, 0, 0), "Arial", 28);
        
        customCanvas->endFrame();
        
        window->endPaint();
    };
    
    drawCustom();
    
    // Process events until window is closed
    while (window->processEvents()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    std::cout << "Test 4 PASSED" << std::endl;
    return true;
}

// Test 5: Browser Controls
bool testBrowserControls() {
    std::cout << "\n=== Test 5: Browser Controls ===" << std::endl;
    
    WindowConfig config;
    config.title = "Browser Controls Test - Close to continue";
    config.width = 1024;
    config.height = 768;
    
    auto window = createPlatformWindow(config);
    if (!window || !window->create()) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }
    
    // Create browser controls (without full browser)
    BrowserControls controls(nullptr);
    
    // Create custom render context
    auto ctx = std::make_shared<CustomRenderContext>();
    
    window->show();
    
    std::cout << "Displaying browser controls mockup..." << std::endl;
    std::cout << "Close the window to continue to the next test..." << std::endl;
    
    // Draw controls
    auto drawControls = [&window, &controls, &ctx]() {
        window->beginPaint();
        Canvas* canvas = window->getCanvas();
        
        if (canvas) {
            canvas->clear(Canvas::rgb(255, 255, 255));
            
            // Draw toolbar background
            canvas->drawRect(0, 0, 1024, 40, Canvas::rgb(240, 240, 240), true);
            
            // Draw some mock buttons
            canvas->drawRect(5, 5, 30, 30, Canvas::rgb(200, 200, 200), true);
            canvas->drawText("←", 15, 20, Canvas::rgb(0, 0, 0), "Arial", 16);
            
            canvas->drawRect(40, 5, 30, 30, Canvas::rgb(200, 200, 200), true);
            canvas->drawText("→", 50, 20, Canvas::rgb(0, 0, 0), "Arial", 16);
            
            canvas->drawRect(75, 5, 30, 30, Canvas::rgb(200, 200, 200), true);
            canvas->drawText("↻", 85, 20, Canvas::rgb(0, 0, 0), "Arial", 16);
            
            // Draw address bar
            canvas->drawRect(110, 5, 900, 30, Canvas::rgb(255, 255, 255), true);
            canvas->drawRect(110, 5, 900, 30, Canvas::rgb(180, 180, 180), false);
            canvas->drawText("https://example.com", 115, 20, Canvas::rgb(0, 0, 0), "Arial", 14);
        }
        
        window->endPaint();
    };
    
    drawControls();
    
    // Process events until window is closed
    while (window->processEvents()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    std::cout << "Test 5 PASSED" << std::endl;
    return true;
}

// Test 6: Paint System
bool testPaintSystem() {
    std::cout << "\n=== Test 6: Paint System ===" << std::endl;
    
    // Create a simple layout box
    css::ComputedStyle style;
    style.setProperty("background-color", css::Value("red"));
    style.setProperty("width", css::Value("200px"));
    style.setProperty("height", css::Value("100px"));
    style.setProperty("margin-left", css::Value("50px"));
    style.setProperty("margin-top", css::Value("50px"));
    
    auto box = std::make_shared<BlockBox>(nullptr, style);
    box->layout(800); // Available width
    
    // Create paint system
    auto paintSystem = std::make_shared<PaintSystem>();
    if (!paintSystem->initialize()) {
        std::cerr << "Failed to initialize paint system" << std::endl;
        return false;
    }
    
    // Create paint context
    PaintContext context = paintSystem->createContext(box.get());
    
    // Paint the box
    paintSystem->paintBox(box.get(), context);
    
    // Get display list
    const DisplayList& displayList = context.displayList();
    
    std::cout << "Display list has " << displayList.items().size() << " items" << std::endl;
    
    // Create render target
    auto renderTarget = std::make_shared<CustomRenderTarget>(800, 600);
    
    // Create renderer
    Renderer renderer;
    renderer.initialize();
    renderer.setPaintSystem(paintSystem);
    
    // Render the box
    renderer.render(box.get(), renderTarget.get());
    
    std::cout << "Paint system test completed" << std::endl;
    std::cout << "Test 6 PASSED" << std::endl;
    return true;
}

// Test 7: Full Rendering Pipeline
bool testFullRenderingPipeline() {
    std::cout << "\n=== Test 7: Full Rendering Pipeline ===" << std::endl;
    
    WindowConfig config;
    config.title = "Full Rendering Pipeline Test - Close to continue";
    config.width = 800;
    config.height = 600;
    
    auto window = createPlatformWindow(config);
    if (!window || !window->create()) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }
    
    // Create a simple DOM tree
    auto doc = std::make_shared<html::Document>();
    auto html = doc->createElement("html");
    auto body = doc->createElement("body");
    auto div = doc->createElement("div");
    div->setAttribute("style", "background-color: blue; width: 300px; height: 200px; margin: 50px;");
    
    doc->appendChild(html);
    html->appendChild(body);
    body->appendChild(div);
    
    // Create style resolver
    css::StyleResolver styleResolver;
    styleResolver.setDocument(doc.get());
    styleResolver.resolveStyles();
    
    // Create layout engine
    layout::LayoutEngine layoutEngine;
    layoutEngine.initialize();
    layoutEngine.layoutDocument(doc.get(), &styleResolver, 800, 600);
    
    // Create renderer
    auto renderer = std::make_shared<Renderer>();
    renderer->initialize();
    
    // Create paint system
    auto paintSystem = std::make_shared<PaintSystem>();
    paintSystem->initialize();
    renderer->setPaintSystem(paintSystem);
    
    window->show();
    
    std::cout << "Rendering a blue box using the full pipeline..." << std::endl;
    std::cout << "Close the window to continue..." << std::endl;
    
    // Render
    auto render = [&]() {
        window->beginPaint();
        Canvas* canvas = window->getCanvas();
        
        if (canvas && layoutEngine.layoutRoot()) {
            // Clear background
            canvas->clear(Canvas::rgb(255, 255, 255));
            
            // Create paint context
            PaintContext context = paintSystem->createContext(layoutEngine.layoutRoot().get());
            paintSystem->paintBox(layoutEngine.layoutRoot().get(), context);
            
            // Render display list to canvas
            const DisplayList& displayList = context.displayList();
            
            for (const auto& item : displayList.items()) {
                if (!item) continue;
                
                if (item->type() == DisplayItemType::BACKGROUND) {
                    auto bgItem = static_cast<BackgroundDisplayItem*>(item.get());
                    canvas->drawRect(
                        static_cast<int>(bgItem->rect().x),
                        static_cast<int>(bgItem->rect().y),
                        static_cast<int>(bgItem->rect().width),
                        static_cast<int>(bgItem->rect().height),
                        Canvas::rgb(bgItem->color().r, bgItem->color().g, bgItem->color().b),
                        true
                    );
                }
            }
            
            // Also show ASCII representation
            std::string ascii = renderer->renderToASCII(layoutEngine.layoutRoot().get(), 80, 24);
            std::cout << "ASCII Render:\n" << ascii << std::endl;
        }
        
        window->endPaint();
    };
    
    render();
    
    // Process events until window is closed
    while (window->processEvents()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    std::cout << "Test 7 PASSED" << std::endl;
    return true;
}

// Main test runner
int main(int argc, char* argv[]) {
    std::cout << "Browser UI and Rendering Test Suite" << std::endl;
    std::cout << "===================================" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    // Run tests
    struct Test {
        const char* name;
        bool (*func)();
    };
    
    Test tests[] = {
        {"Basic Window Creation", testBasicWindow},
        {"Canvas Drawing", testCanvasDrawing},
        {"Custom Renderer", testCustomRenderer},
        {"Custom Canvas", testCustomCanvas},
        {"Browser Controls", testBrowserControls},
        {"Paint System", testPaintSystem},
        {"Full Rendering Pipeline", testFullRenderingPipeline}
    };
    
    // Check if running specific test
    int testToRun = -1;
    if (argc > 1) {
        testToRun = std::atoi(argv[1]) - 1;
        if (testToRun >= 0 && testToRun < sizeof(tests)/sizeof(tests[0])) {
            std::cout << "\nRunning only test " << (testToRun + 1) << ": " << tests[testToRun].name << std::endl;
        } else {
            std::cout << "\nInvalid test number. Running all tests." << std::endl;
            testToRun = -1;
        }
    }
    
    for (int i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
        if (testToRun >= 0 && i != testToRun) {
            continue; // Skip tests we're not running
        }
        
        const auto& test = tests[i];
        try {
            std::cout << "\nRunning: " << test.name << std::endl;
            if (test.func()) {
                passed++;
            } else {
                failed++;
                std::cerr << "FAILED: " << test.name << std::endl;
            }
        } catch (const std::exception& e) {
            failed++;
            std::cerr << "EXCEPTION in " << test.name << ": " << e.what() << std::endl;
        } catch (...) {
            failed++;
            std::cerr << "UNKNOWN EXCEPTION in " << test.name << std::endl;
        }
    }
    
    // Summary
    std::cout << "\n\nTest Summary:" << std::endl;
    std::cout << "=============" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << "Total:  " << (passed + failed) << std::endl;
    
    if (failed == 0) {
        std::cout << "\nAll tests PASSED!" << std::endl;
    } else {
        std::cout << "\nSome tests FAILED!" << std::endl;
    }
    
    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
    
    return failed == 0 ? 0 : 1;
}