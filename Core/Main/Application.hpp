#pragma once

#include <SDL.h>
#include <memory>
#include <vector>
#include <string>
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
    void InitializeImGui() const;
    void ProcessEvents() noexcept;
    void Update() noexcept;
    void RenderFrame() noexcept;
    void RenderUI() noexcept;
    void RenderPropertiesPanel() noexcept;
    void RefreshWorkspaceFiles();

    static void InitSDL();
    void CreateWindow();

    bool m_running = true;
    SDL_Window* m_window = nullptr;
    Renderer m_renderer;
    std::unique_ptr<EventHandler> m_eventHandler;
    std::unique_ptr<DiagramData> m_diagramData;

    std::vector<std::string> m_workspaceFiles;
    bool m_showProperties = true;
    bool m_showDemo = false;
};
