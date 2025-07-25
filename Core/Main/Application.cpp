#include "Application.hpp"
#include <stdexcept>
#include <iostream>

Application::Application() {
    // Initialize SDL with proper error handling
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        throw std::runtime_error("SDL_Init error: " + std::string(SDL_GetError()));
    }

    // Set SDL hints for better rendering
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"); // Linear filtering
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1"); // Enable VSync
    
    // Get mouse position to determine which monitor to use
    int mouseX, mouseY;
    SDL_GetGlobalMouseState(&mouseX, &mouseY);
    
    // Find which display contains the mouse cursor with bounds checking
    int displayIndex = 0;
    int numDisplays = SDL_GetNumVideoDisplays();
    if (numDisplays > 0) {
        for (int i = 0; i < numDisplays; i++) {
            SDL_Rect displayBounds;
            if (SDL_GetDisplayBounds(i, &displayBounds) == 0) {
                if (mouseX >= displayBounds.x && mouseX < displayBounds.x + displayBounds.w &&
                    mouseY >= displayBounds.y && mouseY < displayBounds.y + displayBounds.h) {
                    displayIndex = i;
                    break;
                }
            }
        }
    }

    // Create window as hidden first to prevent flash
    m_window = SDL_CreateWindow("Diagram Editor",
                                SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), 
                                SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), 1280, 720,
                                SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        SDL_Quit();
        throw std::runtime_error("SDL_CreateWindow error: " + std::string(SDL_GetError()));
    }

    // Initialize renderer with proper error checking
    if (!m_renderer.Initialize(m_window)) {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        throw std::runtime_error("Failed to initialize renderer");
    }
    
    // Clear and render first frame with proper synchronization
    m_renderer.Clear();
    m_renderer.DrawGrid(m_diagramData.GetCamera());
    m_renderer.DrawBlocks(m_diagramData.GetBlocks(), m_diagramData.GetCamera());
    m_renderer.Present();
    
    // Add small delay to ensure rendering is complete
    SDL_Delay(16); // ~60fps frame time
    
    // Now show the window smoothly
    SDL_ShowWindow(m_window);
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

