#pragma once

#include <SDL.h>
#include <vector>
#include <string>
#include "Renderer.hpp"
#include "EventHandler.hpp"
#include "DiagramData.hpp"
#include "Utils/Notification.hpp"

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
    void RenderFrame() noexcept;
    void RenderUI() noexcept;
    void RenderPropertiesPanel() noexcept;
    void RefreshWorkspaceFiles();
    void SaveDiagram() noexcept;
    static void DarkStyle() noexcept;
    static void SetupFont() noexcept;

    static void InitSDL();
    void CreateWindow();

    bool m_running = true;
    SDL_Window* m_window = nullptr;
    Renderer m_renderer;
    DiagramData m_diagramData;

    std::string m_currentFilePath;
    std::vector<std::string> m_workspaceFiles;
    bool m_showProperties = true;
    bool m_showDemo = false;
    bool m_showComponentTree = true;
    bool m_showComponentEditor = true;
    
};
