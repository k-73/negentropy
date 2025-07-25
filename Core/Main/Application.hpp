#pragma once

#include <SDL.h>
#include <memory>

class Renderer;
class EventHandler;
class DiagramData;

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
    void ProcessEvents() noexcept;
    void Update() noexcept;
    void RenderFrame() noexcept;
    void RenderUI() noexcept;
    void RenderPropertiesPanel() noexcept;

    bool m_running = true;
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_sdlRenderer = nullptr;
    
    bool m_showProperties = true;
    bool m_showDemo = false;

    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<EventHandler> m_eventHandler;
    std::unique_ptr<DiagramData> m_diagramData;
};
