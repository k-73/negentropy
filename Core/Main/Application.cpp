#include "Application.hpp"
#include <stdexcept>
#include <iostream>

Application::Application() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error("SDL_Init error: " + std::string(SDL_GetError()));
    }

    m_window = SDL_CreateWindow("Diagram Editor",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
                                SDL_WINDOW_SHOWN);
    if (!m_window) {
        throw std::runtime_error("SDL_CreateWindow error: " + std::string(SDL_GetError()));
    }

    m_renderer.Initialize(m_window);
}

Application::~Application() {
    m_renderer.Cleanup();
    if (m_window) {
        SDL_DestroyWindow(m_window);
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

