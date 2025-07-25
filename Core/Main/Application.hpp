#pragma once

#include <SDL.h>
#include "Renderer.hpp"
#include "EventHandler.hpp"
#include "DiagramData.hpp"

class Application {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    void Run();

private:
    void RenderFrame();

    bool m_running = true;
    SDL_Window* m_window = nullptr;

    Renderer m_renderer;
    EventHandler m_eventHandler;
    DiagramData m_diagramData;
};
