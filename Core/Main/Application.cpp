#include "Application.hpp"
//
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <spdlog/spdlog.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <algorithm>
#include <filesystem>
#include <stdexcept>
//
#include "../Diagram/TreeRenderer.hpp"
#include "../Utils/IconsFontAwesome5.h"
#include "../Utils/Notification.hpp"
#include "../Utils/Path.hpp"

namespace fs = std::filesystem;

Application::Application() {
	InitSDL();
	CreateWindow();

	if(!renderer.Initialize(window)) {
		SDL_DestroyWindow(window);
		SDL_Quit();
		throw std::runtime_error("Failed to initialize renderer");
	}

	InitializeImGui();

#ifndef __EMSCRIPTEN__
	spdlog::info("Application initialized successfully");
#endif

	try {
		currentFilePath = (Utils::GetWorkspacePath() / "Default.xml").string();
		diagramData.Load(currentFilePath);
		RefreshWorkspaceFiles();
		spdlog::info("Files loaded successfully");
	} catch (const std::exception& e) {
		spdlog::warn("Failed to load files: {}", e.what());
		currentFilePath = "";
	}

	DiagramData::SetInstance(&diagramData);
	SDL_ShowWindow(window);
}

Application::~Application() {
	spdlog::info("Shutting down application...");

	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	if(window) {
		SDL_DestroyWindow(window);
		window = nullptr;
	}

	SDL_Quit();
	spdlog::info("Application shutdown complete");
}

void Application::Run() {
#ifdef __EMSCRIPTEN__
	// Use browser's requestAnimationFrame for smooth 60fps
	emscripten_set_main_loop_arg([](void* arg) {
		static_cast<Application*>(arg)->MainLoop();
	}, this, -1, 1); // -1 = use requestAnimationFrame timing
#else
	while(isRunning) {
		MainLoop();
	}
#endif
}

void Application::MainLoop() {
	if (!isRunning) return;
	
	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	ProcessEvents();
	RenderFrame();
}

void Application::InitializeImGui() const {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

#ifdef __EMSCRIPTEN__
	// Maximum responsiveness settings for WebAssembly
	io.MouseDrawCursor = false; // Use native cursor for better performance
	io.ConfigInputTrickleEventQueue = false; // Process events immediately
	io.ConfigDragClickToInputText = false; // Reduce input latency
	io.ConfigMacOSXBehaviors = false; // Disable Mac-specific delays
	io.ConfigInputTextCursorBlink = false; // Disable cursor blinking for performance
	io.ConfigWindowsResizeFromEdges = false; // Reduce edge detection overhead
#endif

	DarkStyle();
	SetupFont();

	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer.GetSDLRenderer());
	ImGui_ImplSDLRenderer2_Init(renderer.GetSDLRenderer());

	spdlog::info("ImGui initialized");
}

void Application::ProcessEvents() noexcept {
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		ImGui_ImplSDL2_ProcessEvent(&event);

		if(event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
			isRunning = false;
			return;
		}

		if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_s && (event.key.keysym.mod & KMOD_CTRL)) {
			SaveDiagram();
			return;
		}

		if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F1) {
			// Create a new block component as a child of the grid
			auto newBlock = std::make_unique<Diagram::BlockComponent>("New Block", glm::vec2{0.0f, 0.0f}, glm::vec2{100.0f, 50.0f});
			newBlock->id = "block_" + std::to_string(diagramData.GetComponentList().size() + 1);
			
			// Add the block as a child of the grid component
			diagramData.GetGrid().AddChild(std::move(newBlock));
			return;
		}

		bool shouldProcessEvent = true;
		if(event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEWHEEL) {
			shouldProcessEvent = !ImGui::GetIO().WantCaptureMouse;
		} else if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP || event.type == SDL_TEXTINPUT) {
			shouldProcessEvent = !ImGui::GetIO().WantCaptureKeyboard;
		}

		if(shouldProcessEvent) {
			// TODO: Update event handling for new component system
			int windowWidth, windowHeight;
			SDL_GetWindowSize(window, &windowWidth, &windowHeight);
			const glm::vec2 screenSize {static_cast<float>(windowWidth), static_cast<float>(windowHeight)};
			
			// Basic camera event handling
			auto& camera = diagramData.GetCamera();
			if(event.type == SDL_MOUSEWHEEL) {
				int mousePositionX, mousePositionY;
				SDL_GetMouseState(&mousePositionX, &mousePositionY);
				const glm::vec2 mousePosition {static_cast<float>(mousePositionX), static_cast<float>(mousePositionY)};
				camera.ZoomAt(mousePosition, screenSize, event.wheel.y > 0 ? 1.1f : 0.9f);
			}
			
			// Handle component events using the proper hierarchy
			bool eventHandled = false;
			const auto cameraView = camera.GetView();
			
			// Use the DispatchEvent function to handle events through the hierarchy
			// Start with the grid (which contains the blocks)
			eventHandled = Diagram::DispatchEvent(&diagramData.GetGrid(), event, cameraView, screenSize);
			
			// If grid didn't handle it, try orphaned components from the main list
			if(!eventHandled) {
				auto& componentList = diagramData.GetComponentList();
				for(auto& component : componentList) {
					if(!component->GetParent()) { // Only check orphaned components
						if(auto* eventHandler = dynamic_cast<Diagram::EventHandler*>(component.get())) {
							if(eventHandler->HandleEvent(event, cameraView, screenSize)) {
								eventHandled = true;
								break;
							}
						}
					}
				}
			}
			
			// Finally, try camera if nothing else handled it
			if(!eventHandled) {
				if(auto* cameraHandler = dynamic_cast<Diagram::EventHandler*>(&camera)) {
					eventHandled = cameraHandler->HandleEvent(event, cameraView, screenSize);
				}
			}
		}
	}
}

void Application::RenderFrame() noexcept {
	RenderUI();

	renderer.Clear();
	
	// Render grid first
	int windowWidth, windowHeight;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);
	const glm::vec2 screenSize {static_cast<float>(windowWidth), static_cast<float>(windowHeight)};
	
	const auto& cameraView = diagramData.GetCamera().GetView();
	
	// Use RenderOfficial to render grid and its children properly
	diagramData.GetGrid().RenderOfficial(renderer.GetSDLRenderer(), cameraView, screenSize);
	
	// Render any orphaned components from the main list
	renderer.DrawComponents(diagramData.GetComponentList(), cameraView);

	ImGui::Render();
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer.GetSDLRenderer());

	renderer.Present();
}

void Application::RenderUI() noexcept {
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 3.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 3.0f));

	if(ImGui::BeginMainMenuBar()) {
		if(ImGui::BeginMenu((ICON_FA_FILE "  File"))) {
			if(ImGui::BeginMenu((ICON_FA_FOLDER_OPEN "  Load"))) {
				for(const auto& workspaceFile: workspaceFiles) {
					std::string menuItem = std::string(ICON_FA_FILE_ALT) + "  " + workspaceFile;
					if(ImGui::MenuItem(menuItem.c_str())) {
						currentFilePath = (Utils::GetWorkspacePath() / workspaceFile).string();
						diagramData.Load(currentFilePath);
					}
				}
				ImGui::EndMenu();
			}
			if(ImGui::MenuItem((ICON_FA_SAVE "  Save"), "Ctrl+S")) {
				SaveDiagram();
			}
			ImGui::Separator();
			if(ImGui::MenuItem((ICON_FA_TIMES "  Exit"), "Alt+F4")) {
				isRunning = false;
			}
			ImGui::EndMenu();
		}

		if(ImGui::BeginMenu((ICON_FA_EDIT "  Edit"))) {
			if(ImGui::MenuItem((ICON_FA_UNDO "  Undo"), "Ctrl+Z")) {}
			if(ImGui::MenuItem((ICON_FA_REDO "  Redo"), "Ctrl+Y")) {}
			ImGui::EndMenu();
		}

		if(ImGui::BeginMenu((ICON_FA_EYE "  View"))) {
			ImGui::MenuItem((ICON_FA_WRENCH "  Properties"), nullptr, &isShownPropertiesPanel);
			ImGui::MenuItem((ICON_FA_SITEMAP "  Component Tree"), nullptr, &isShownComponentTreePanel);
			ImGui::MenuItem((ICON_FA_EDIT "  Component Editor"), nullptr, &isShownComponentEditorPanel);
			ImGui::MenuItem((ICON_FA_MAGIC "  Demo"), nullptr, &isShownDemoPanel);
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	ImGui::PopStyleVar(2);

	if(isShownPropertiesPanel) {
		RenderPropertiesPanel();
	}

	if(isShownComponentTreePanel) {
		ImGui::Begin("Component Tree", &isShownComponentTreePanel);
		
		// Variable to track component to delete (to avoid modifying tree during iteration)
		Diagram::Component* componentToDelete = nullptr;
		
		// Custom tree rendering for our specific hierarchy
		if(ImGui::BeginTable("TreeTable", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 64.0f);
			
			// Helper function to render a component and its children
			std::function<void(Diagram::Component*, int)> renderComponentRecursive = [&](Diagram::Component* component, int depth) {
				if(!component) return;
				
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				
				// Determine flags and icon
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowItemOverlap;
				if(Diagram::TreeRenderer::GetActiveComponent() == component) flags |= ImGuiTreeNodeFlags_Selected;
				if(component->GetChildren().empty()) flags |= ImGuiTreeNodeFlags_Leaf;
				
				const char* icon = ICON_FA_CUBE;
				if(dynamic_cast<Diagram::CameraComponent*>(component)) icon = ICON_FA_VIDEO;
				else if(dynamic_cast<Diagram::GridComponent*>(component)) icon = ICON_FA_TH;
				else if(dynamic_cast<Diagram::BlockComponent*>(component)) icon = ICON_FA_SQUARE;
				
				ImGui::PushID(component);
				
				const std::string label = " " + std::string(icon) + "  " + component->GetDisplayName();
				bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags);
				
				if(ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
					Diagram::TreeRenderer::SetActiveComponent(component);
					
					// Clear all selections
					auto& componentList = diagramData.GetComponentList();
					for(auto& comp : componentList) {
						comp->Deselect();
					}
					diagramData.GetCamera().Deselect();
					diagramData.GetGrid().Deselect();
					
					component->Select();
				}
				
				ImGui::TableNextColumn();
				
				// Action buttons
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
				
				// Add button for container components
				if(dynamic_cast<Diagram::GridComponent*>(component)) {
					if(ImGui::Button(ICON_FA_PLUS)) {
						auto newBlock = std::make_unique<Diagram::BlockComponent>("New Block", glm::vec2{0.0f, 0.0f}, glm::vec2{100.0f, 50.0f});
						newBlock->id = "block_" + std::to_string(component->GetChildren().size() + 1);
						component->AddChild(std::move(newBlock));
					}
					ImGui::SameLine();
				}
				
				// Delete button for deletable components (not camera or grid)
				if(!dynamic_cast<Diagram::CameraComponent*>(component) && 
				   !dynamic_cast<Diagram::GridComponent*>(component)) {
					if(ImGui::Button(ICON_FA_TRASH)) {
						// Mark for deletion - we'll handle it after the tree rendering
						componentToDelete = component;
					}
					ImGui::SameLine();
				}
				
				// Settings button
				if(ImGui::Button(ICON_FA_COG)) {
					Diagram::TreeRenderer::AddToEditor(component);
				}
				
				ImGui::PopStyleVar();
				
				// Render children
				if(isOpen) {
					for(const auto& child : component->GetChildren()) {
						renderComponentRecursive(child.get(), depth + 1);
					}
					ImGui::TreePop();
				}
				
				ImGui::PopID();
			};
			
			// Render camera as root component
			renderComponentRecursive(&diagramData.GetCamera(), 0);
			
			// Render grid as root component (this will show its children - the blocks)
			renderComponentRecursive(&diagramData.GetGrid(), 0);
			
			// Show any orphaned components from the main list (shouldn't be any now)
			auto& componentList = diagramData.GetComponentList();
			for(const auto& component : componentList) {
				if(!component->GetParent()) { // Only show orphaned components
					renderComponentRecursive(component.get(), 0);
				}
			}
			
			ImGui::EndTable();
		}
		
		// Add a button to migrate orphaned blocks to grid
		if(ImGui::Button("Fix Hierarchy - Move Orphaned Blocks to Grid")) {
			auto& componentList = diagramData.GetComponentList();
			std::vector<std::unique_ptr<Diagram::Component>> blocksToMove;
			
			// Find orphaned BlockComponents
			for(auto it = componentList.begin(); it != componentList.end();) {
				if(auto* blockComp = dynamic_cast<Diagram::BlockComponent*>(it->get())) {
					if(!blockComp->GetParent()) {
						blocksToMove.push_back(std::move(*it));
						it = componentList.erase(it);
						continue;
					}
				}
				++it;
			}
			
			// Move them to grid
			for(auto& block : blocksToMove) {
				diagramData.GetGrid().AddChild(std::move(block));
			}
		}
		
		// Handle component deletion after tree rendering to avoid modifying during iteration
		if(componentToDelete) {
			// First, clear any selections that might reference this component
			if(componentToDelete->IsSelected()) {
				componentToDelete->Deselect();
			}
			
			// Clear it from TreeRenderer if it's the active component
			if(Diagram::TreeRenderer::GetActiveComponent() == componentToDelete) {
				Diagram::TreeRenderer::SetActiveComponent(nullptr);
			}
			
			// Clear all selections to ensure no dangling references
			auto& componentList = diagramData.GetComponentList();
			for(auto& comp : componentList) {
				if(comp.get() != componentToDelete) {
					comp->Deselect();
				}
			}
			diagramData.GetCamera().Deselect();
			diagramData.GetGrid().Deselect();
			
			// Now safely remove the component
			if(componentToDelete->GetParent()) {
				// Remove from parent - this will destroy the component
				auto removedComponent = componentToDelete->GetParent()->RemoveChild(componentToDelete);
				// removedComponent will be destroyed when it goes out of scope
			} else {
				// Remove from main component list if it's orphaned
				componentList.erase(
					std::remove_if(componentList.begin(), componentList.end(),
						[componentToDelete](const std::unique_ptr<Diagram::Component>& comp) {
							return comp.get() == componentToDelete;
						}),
					componentList.end()
				);
			}
			
			// componentToDelete is now invalid - don't use it after this point
		}
		
		ImGui::End();
	}

	if(isShownComponentEditorPanel) {
		Diagram::TreeRenderer::RenderComponentEditor();
	}

	if(isShownDemoPanel) {
		ImGui::ShowDemoWindow(&isShownDemoPanel);
	}

	Notify::Render();
}

void Application::RenderPropertiesPanel() noexcept {
	ImGui::Begin("Properties", &isShownPropertiesPanel);

	auto& componentList = diagramData.GetComponentList();
	auto& camera = diagramData.GetCamera();

	size_t componentCount = componentList.size();

	const auto cameraView = camera.GetView();
	ImGui::Text("Camera: (%.1f, %.1f) Zoom: %.2f", camera.position.x, camera.position.y, cameraView.zoom);
	ImGui::Text("Components: %zu", componentCount);

	if(ImGui::Button((ICON_FA_PLUS "  [F1] Add Block"))) {
		// Create a new block component as a child of the grid
		auto newBlock = std::make_unique<Diagram::BlockComponent>("New Block", glm::vec2{0.0f, 0.0f}, glm::vec2{100.0f, 50.0f});
		newBlock->id = "block_" + std::to_string(componentCount + 1);
		
		// Add the block as a child of the grid component
		diagramData.GetGrid().AddChild(std::move(newBlock));
	}

	ImGui::End();
}

void Application::SaveDiagram() noexcept {
	if(!currentFilePath.empty()) {
		diagramData.Save(currentFilePath);
	} else {
		Notify::Error("No file is currently open to save.");
	}
}

void Application::RefreshWorkspaceFiles() {
	workspaceFiles.clear();
	const auto workspacePath = Utils::GetWorkspacePath();
	for(const auto& workspaceEntry: fs::directory_iterator(workspacePath)) {
		if(workspaceEntry.is_regular_file() && workspaceEntry.path().extension() == ".xml") {
			workspaceFiles.push_back(workspaceEntry.path().filename().string());
		}
	}
}

void Application::InitSDL() {
	spdlog::info("Initializing SDL...");
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
		throw std::runtime_error("SDL_Init error: " + std::string(SDL_GetError()));

#ifdef __EMSCRIPTEN__
	// Maximum responsiveness settings for WebAssembly
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // Fast nearest neighbor scaling
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0"); // Disable vsync completely
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2"); // Force OpenGL ES2 for better performance
	SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
	SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "1"); // Enable framebuffer acceleration
	SDL_SetHint(SDL_HINT_RENDER_BATCHING, "0"); // Disable batching for immediate rendering
	SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "0"); // Prevent screensaver
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1"); // Immediate mouse focus
#else
	// Desktop settings
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
	SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
#endif
}

void Application::DarkStyle() noexcept {
	auto& style = ImGui::GetStyle();
	auto& colors = style.Colors;

	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.98f);
	colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.14f, 0.98f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.18f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.19f, 0.19f, 0.21f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.17f, 0.17f, 0.19f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.17f, 0.17f, 0.19f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.47f, 0.84f, 0.20f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.00f, 0.47f, 0.84f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 0.47f, 0.84f, 0.95f);
	colors[ImGuiCol_Tab] = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.17f, 0.17f, 0.19f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.11f, 0.13f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.25f, 0.25f, 0.26f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.47f, 0.84f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 0.80f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);

	// Compact VS Code Style Settings
	style.WindowPadding = ImVec2(5.0f, 5.0f);
	style.FramePadding = ImVec2(6.0f, 3.0f);
	style.CellPadding = ImVec2(3.0f, 2.0f);
	style.ItemSpacing = ImVec2(5.0f, 3.0f);
	style.ItemInnerSpacing = ImVec2(3.0f, 2.0f);
	style.TouchExtraPadding = ImVec2(0.0f, 0.0f);

	style.IndentSpacing = 12.0f;
	style.ScrollbarSize = 12.0f;
	style.GrabMinSize = 8.0f;

	// Borders
	style.WindowBorderSize = 0.0f;
	style.ChildBorderSize = 0.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;
	style.TabBorderSize = 0.0f;

	// Rounding
	style.WindowRounding = 2.0f;
	style.ChildRounding = 4.0f;
	style.FrameRounding = 2.0f;
	style.PopupRounding = 3.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabRounding = 2.0f;
	style.LogSliderDeadzone = 4.0f;
	style.TabRounding = 4.0f;

	// Alignment
	style.WindowTitleAlign = ImVec2(0.00f, 0.50f);
	style.WindowMenuButtonPosition = ImGuiDir_Left;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.50f, 0.50f);
	style.SelectableTextAlign = ImVec2(0.00f, 0.00f);

	// Safe Area Padding
	style.DisplaySafeAreaPadding = ImVec2(3.0f, 3.0f);
}

void Application::SetupFont() noexcept {
	ImGuiIO& io = ImGui::GetIO();

	constexpr float fontSizeBase = 13.0f;
	constexpr float fontSizeIcon = fontSizeBase * 2.0f / 3.0f;
	constexpr float fontSizeDefault = 14.0f;

	bool isFontLoaded = false;

#ifdef __EMSCRIPTEN__
	// For WASM, first try to load from preloaded assets
	std::vector<std::string> wasmFontPaths = {
		"Assets/fonts/LiberationSans-Regular.ttf"
	};
	
	for (const auto& fontPath : wasmFontPaths) {
		if(std::filesystem::exists(fontPath)) {
			ImFontConfig fontConfig;
			fontConfig.OversampleH = 3;
			fontConfig.OversampleV = 2;
			fontConfig.PixelSnapH = true;
			io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSizeBase, &fontConfig);
			isFontLoaded = true;
			spdlog::info("Font loaded from: {}", fontPath);
			break;
		}
	}
#else
	// For native builds, try system fonts
	constexpr const char* fontPathGeneralList[] = {
		"/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
		"/usr/share/fonts/TTF/arial.ttf",
		"/System/Library/Fonts/Helvetica.ttc",
		"/Windows/Fonts/arial.ttf",
		"/Windows/Fonts/segoeui.ttf"};

	for(const char* fontPathGeneral: fontPathGeneralList) {
		if(std::filesystem::exists(fontPathGeneral)) {
			ImFontConfig fontConfig;
			fontConfig.OversampleH = 3;
			fontConfig.OversampleV = 2;
			fontConfig.PixelSnapH = true;
			io.Fonts->AddFontFromFileTTF(fontPathGeneral, fontSizeBase, &fontConfig);
			isFontLoaded = true;
			break;
		}
	}
#endif

	if(!isFontLoaded) {
		ImFontConfig fontConfig;
		fontConfig.SizePixels = fontSizeDefault;
		fontConfig.OversampleH = 3;
		fontConfig.OversampleV = 2;
		fontConfig.PixelSnapH = true;
		io.Fonts->AddFontDefault(&fontConfig);
		spdlog::warn("Using default ImGui font");
	}

	// Merge in icons from Font Awesome
	static const ImWchar iconFontRange[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
	ImFontConfig iconFontConfig;
	iconFontConfig.MergeMode = true;
	iconFontConfig.PixelSnapH = true;
	iconFontConfig.GlyphMinAdvanceX = fontSizeIcon;

	// Try different paths for font loading
	std::vector<std::string> fontPaths = {
		"Assets/fonts/fa-solid-900.ttf",  // WASM preloaded path
		std::string(PROJECT_SOURCE_DIR) + "/Assets/fonts/fa-solid-900.ttf"  // Native path
	};
	
	bool fontAwesomeLoaded = false;
	for (const auto& fontPath : fontPaths) {
		if(std::filesystem::exists(fontPath)) {
			io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSizeIcon, &iconFontConfig, iconFontRange);
			fontAwesomeLoaded = true;
			spdlog::info("Font Awesome loaded from: {}", fontPath);
			break;
		}
	}
	
	if (!fontAwesomeLoaded) {
		spdlog::warn("Font Awesome not found");
	}
}

void Application::CreateWindow() {
	spdlog::info("Creating window...");
	int displayIndexGeneral = 0;
	int mousePositionX, mousePositionY;
	SDL_GetGlobalMouseState(&mousePositionX, &mousePositionY);
	int displayCount = SDL_GetNumVideoDisplays();

	for(int i = 0; i < displayCount; i++) {
		SDL_Rect displayBounds;
		if(SDL_GetDisplayBounds(i, &displayBounds) == 0 &&
		   mousePositionX >= displayBounds.x && mousePositionX < displayBounds.x + displayBounds.w &&
		   mousePositionY >= displayBounds.y && mousePositionY < displayBounds.y + displayBounds.h) {
			displayIndexGeneral = i;
			break;
		}
	}

	window = SDL_CreateWindow("Negentropy - Diagram Editor",
							  SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndexGeneral),
							  SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndexGeneral), 1280, 720,
							  SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if(!window) {
		SDL_Quit();
		throw std::runtime_error("SDL_CreateWindow error: " + std::string(SDL_GetError()));
	}
}
