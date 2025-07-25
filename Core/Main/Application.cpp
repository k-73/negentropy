#include "Application.hpp"
#include <stdexcept>

Application::Application() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        throw std::runtime_error("SDL_Init error: " + std::string(SDL_GetError()));

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    
    int mouseX, mouseY;
    SDL_GetGlobalMouseState(&mouseX, &mouseY);
    
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

    m_window = SDL_CreateWindow("Diagram Editor",
                                SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), 
                                SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), 1280, 720,
                                SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!m_window) {
        SDL_Quit();
        throw std::runtime_error("SDL_CreateWindow error: " + std::string(SDL_GetError()));
    }

    if (!m_renderer.Initialize(m_window)) {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        throw std::runtime_error("Failed to initialize renderer");
    }
    
    m_renderer.Clear();
    m_renderer.DrawGrid(m_diagramData.GetCamera());
    m_renderer.DrawBlocks(m_diagramData.GetBlocks(), m_diagramData.GetCamera());
    m_renderer.Present();
    
    SDL_Delay(16);
    SDL_ShowWindow(m_window);
}

Application::~Application() {
    m_running = false;
    m_renderer.Cleanup();
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
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

