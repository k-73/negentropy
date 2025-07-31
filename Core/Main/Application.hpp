#pragma once

#include <SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <string>
#include <vector>

#include "DiagramData.hpp"
#include "EventHandler.hpp"
#include "Renderer.hpp"
#include "Utils/Notification.hpp"

class Application
{
public:
	Application();
	~Application();

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
	Application(Application&&) = delete;
	Application& operator=(Application&&) = delete;

	void Run();
	void MainLoop();

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

	bool isRunning = true;
	SDL_Window* window = nullptr;
	Renderer renderer;
	DiagramData diagramData;

	std::string currentFilePath;
	std::vector<std::string> workspaceFiles;
	bool isShownPropertiesPanel = true;
	bool isShownDemoPanel = false;
	bool isShownComponentTreePanel = true;
	bool isShownComponentEditorPanel = true;
};
