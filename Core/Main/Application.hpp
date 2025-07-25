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
    void InitializeImGui();
    void ProcessEvents();
    void Update();
    void RenderFrame();
    void RenderUI();
    void RenderPropertiesPanel();

    bool m_running = true;
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_sdlRenderer = nullptr;
    
    bool m_showProperties = true;
    bool m_showDemo = false;

    Renderer m_renderer;
    EventHandler m_eventHandler;
    DiagramData m_diagramData;
};
