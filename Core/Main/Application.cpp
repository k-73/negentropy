#include "Application.hpp"
#include <stdexcept>
#include <iostream>
#include <spdlog/spdlog.h>
#include <chrono>

Application::Application() {
    spdlog::info("=== Application startup begins ===");
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Initialize SDL with proper error handling
    spdlog::info("Initializing SDL...");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        spdlog::error("SDL_Init failed: {}", SDL_GetError());
        throw std::runtime_error("SDL_Init error: " + std::string(SDL_GetError()));
    }
    spdlog::info("SDL initialized successfully");

    // Set SDL hints for better rendering and performance
    spdlog::info("Setting SDL hints...");
    if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0")) { // Use nearest neighbor for speed
        spdlog::warn("Failed to set render scale quality hint");
    }
    if (!SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0")) { // Disable VSync during init for speed
        spdlog::warn("Failed to set VSync hint");
    }
    if (!SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl")) { // Force OpenGL
        spdlog::warn("Failed to set render driver hint");
    }
    if (!SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0")) {
        spdlog::warn("Failed to set compositor bypass hint");
    }
    
    // Set OpenGL attributes for faster context creation
    spdlog::info("Setting OpenGL attributes...");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0); // No depth buffer needed
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0); // No stencil buffer needed
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    
    // Get mouse position to determine which monitor to use
    int mouseX, mouseY;
    SDL_GetGlobalMouseState(&mouseX, &mouseY);
    spdlog::info("Mouse position: ({}, {})", mouseX, mouseY);
    
    // Find which display contains the mouse cursor with bounds checking
    int displayIndex = 0;
    int numDisplays = SDL_GetNumVideoDisplays();
    spdlog::info("Number of displays: {}", numDisplays);
    
    if (numDisplays > 0) {
        for (int i = 0; i < numDisplays; i++) {
            SDL_Rect displayBounds;
            if (SDL_GetDisplayBounds(i, &displayBounds) == 0) {
                spdlog::info("Display {} bounds: x={}, y={}, w={}, h={}", 
                           i, displayBounds.x, displayBounds.y, displayBounds.w, displayBounds.h);
                if (mouseX >= displayBounds.x && mouseX < displayBounds.x + displayBounds.w &&
                    mouseY >= displayBounds.y && mouseY < displayBounds.y + displayBounds.h) {
                    displayIndex = i;
                    spdlog::info("Mouse found on display {}", i);
                    break;
                }
            } else {
                spdlog::warn("Failed to get bounds for display {}: {}", i, SDL_GetError());
            }
        }
    }
    spdlog::info("Using display index: {}", displayIndex);

    // Create window with minimized flag first to prevent flash completely
    spdlog::info("Creating window (minimized)...");
    auto window_start = std::chrono::high_resolution_clock::now();
    m_window = SDL_CreateWindow("Diagram Editor",
                                SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), 
                                SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), 1280, 720,
                                SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!m_window) {
        spdlog::error("SDL_CreateWindow failed: {}", SDL_GetError());
        SDL_Quit();
        throw std::runtime_error("SDL_CreateWindow error: " + std::string(SDL_GetError()));
    }
    auto window_end = std::chrono::high_resolution_clock::now();
    auto window_duration = std::chrono::duration_cast<std::chrono::milliseconds>(window_end - window_start);
    spdlog::info("Window created in {}ms", window_duration.count());

    // Initialize renderer with proper error checking
    spdlog::info("Initializing renderer...");
    auto renderer_start = std::chrono::high_resolution_clock::now();
    if (!m_renderer.Initialize(m_window)) {
        spdlog::error("Renderer initialization failed");
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        throw std::runtime_error("Failed to initialize renderer");
    }
    auto renderer_end = std::chrono::high_resolution_clock::now();
    auto renderer_duration = std::chrono::duration_cast<std::chrono::milliseconds>(renderer_end - renderer_start);
    spdlog::info("Renderer initialized in {}ms", renderer_duration.count());
    
    // Clear and render first frame with proper synchronization
    spdlog::info("Rendering initial frame...");
    auto render_start = std::chrono::high_resolution_clock::now();
    m_renderer.Clear();
    m_renderer.DrawGrid(m_diagramData.GetCamera());
    m_renderer.DrawBlocks(m_diagramData.GetBlocks(), m_diagramData.GetCamera());
    m_renderer.Present();
    auto render_end = std::chrono::high_resolution_clock::now();
    auto render_duration = std::chrono::duration_cast<std::chrono::milliseconds>(render_end - render_start);
    spdlog::info("Initial frame rendered in {}ms", render_duration.count());
    
    // Add small delay to ensure rendering is complete
    spdlog::info("Waiting for rendering to complete...");
    SDL_Delay(16); // ~60fps frame time
    
    // Now show the window smoothly
    spdlog::info("Showing window...");
    auto show_start = std::chrono::high_resolution_clock::now();
    SDL_ShowWindow(m_window);
    auto show_end = std::chrono::high_resolution_clock::now();
    auto show_duration = std::chrono::duration_cast<std::chrono::milliseconds>(show_end - show_start);
    spdlog::info("Window shown in {}ms", show_duration.count());
    
    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - start_time);
    spdlog::info("=== Application startup completed in {}ms ===", total_duration.count());
}

Application::~Application() {
    // Graceful shutdown sequence
    m_running = false;
    
    // Clean up renderer first
    m_renderer.Cleanup();
    
    // Clean up window
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    // Final SDL cleanup
    SDL_Quit();
}

void Application::Run() {
    while (m_running) {
        m_eventHandler.ProcessEvents(m_diagramData.GetCamera(), m_diagramData.GetBlocks(), m_running);
        RenderFrame();
    }
}

void Application::RenderFrame() {
    m_renderer.Clear();
    m_renderer.DrawGrid(m_diagramData.GetCamera());
    m_renderer.DrawBlocks(m_diagramData.GetBlocks(), m_diagramData.GetCamera());
    m_renderer.Present();
}

