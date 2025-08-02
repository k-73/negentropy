#include "Application.hpp"
//
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <spdlog/spdlog.h>
#include <SDL2/SDL_ttf.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <algorithm>
#include <filesystem>
#include <stdexcept>
//
#include "../Diagram/TreeRenderer.hpp"
#include "../Diagram/Components/MinimapComponent.hpp"
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

	TTF_Quit();
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

		if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F2) {
			// Toggle minimap visibility
			auto minimaps = diagramData.GetComponentsOfType<Diagram::MinimapComponent>();
			if(!minimaps.empty()) {
				auto* minimap = minimaps[0];
				minimap->data.visible = !minimap->data.visible;
			}
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
			
			// Try minimap next (screen overlay should have priority over camera)
			if(!eventHandled) {
				eventHandled = HandleMinimapEvent(event, cameraView, screenSize);
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

	// Render minimap as screen overlay AFTER all world-space components
	diagramData.GetMinimap().RenderWithContent(renderer.GetSDLRenderer(), cameraView, screenSize, diagramData.GetGrid());

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
		
		// Drag and drop state variables
		static Diagram::Component* draggedComponent = nullptr;
		static Diagram::Component* dropTarget = nullptr;
		static bool isDragging = false;
		static bool showInsertionLine = false;
		static float insertionLineY = 0.0f;
		static bool insertBelow = false;
		
		// Variable to track component to delete (to avoid modifying tree during iteration)
		Diagram::Component* componentToDelete = nullptr;
		
		// Custom tree rendering for our specific hierarchy
		if(ImGui::BeginTable("TreeTable", 3, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 84.0f);
			ImGui::TableSetupColumn("Group", ImGuiTableColumnFlags_WidthFixed, 52.0f);
			
			// Table header with group controls
			ImGui::TableHeadersRow();
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Components");
			ImGui::TableNextColumn();
			ImGui::Text("Actions");
			ImGui::TableNextColumn();
			
			// Count selected components
			int selectedCount = 0;
			std::function<void(Diagram::Component*)> countSelected = [&](Diagram::Component* comp) {
				if(comp->IsSelected()) selectedCount++;
				for(const auto& child : comp->GetChildren()) {
					countSelected(child.get());
				}
			};
			countSelected(&diagramData.GetGrid());
			
			// Group controls in header
			if(selectedCount > 1) {
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
				if(ImGui::Button(ICON_FA_OBJECT_GROUP "##group_selected")) {
					// Find a suitable parent for the group (first selected component's parent or grid)
					Diagram::Component* groupParent = &diagramData.GetGrid();
					std::vector<Diagram::Component*> selectedComponents;
					
					std::function<void(Diagram::Component*)> collectSelected = [&](Diagram::Component* comp) {
						if(comp->IsSelected() && comp != &diagramData.GetCamera() && comp != &diagramData.GetGrid()) {
							selectedComponents.push_back(comp);
							if(comp->GetParent()) {
								groupParent = comp->GetParent();
							}
						}
						for(const auto& child : comp->GetChildren()) {
							collectSelected(child.get());
						}
					};
					collectSelected(&diagramData.GetGrid());
					
					if(!selectedComponents.empty()) {
						auto newGroup = std::make_unique<Diagram::GroupComponent>("Group_" + std::to_string(groupParent->GetChildren().size() + 1));
						
						// Remove selected components and add to group
						for(auto* selectedComp : selectedComponents) {
							if(selectedComp->GetParent()) {
								auto removedComp = selectedComp->GetParent()->RemoveChild(selectedComp);
								if(removedComp) {
									newGroup->AddChild(std::move(removedComp));
								}
							}
						}
						
						groupParent->AddChild(std::move(newGroup));
					}
				}
				if(ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Group %d selected components", selectedCount);
				}
				ImGui::PopStyleVar();
			} else {
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
				if(ImGui::Button(ICON_FA_PLUS "##new_group")) {
					auto newGroup = std::make_unique<Diagram::GroupComponent>("Empty Group");
					diagramData.GetGrid().AddChild(std::move(newGroup));
				}
				if(ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Create empty group");
				}
				ImGui::PopStyleVar();
			}
			
			// Helper function to render a component and its children
			std::function<void(Diagram::Component*, int)> renderComponentRecursive = [&](Diagram::Component* component, int depth) {
				if(!component) return;
				
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				
				// Determine flags and icon
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowItemOverlap;
				if(Diagram::TreeRenderer::GetActiveComponent() == component) flags |= ImGuiTreeNodeFlags_Selected;
				if(component->GetChildren().empty()) flags |= ImGuiTreeNodeFlags_Leaf;
				
				// Handle multi-selection highlighting manually
				bool isMultiSelected = component->IsSelected() && component != Diagram::TreeRenderer::GetActiveComponent();
				if(isMultiSelected) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f)); // Bright text for multi-selected
				}
				
				const char* icon = ICON_FA_CUBE;
				if(dynamic_cast<Diagram::CameraComponent*>(component)) icon = ICON_FA_VIDEO;
				else if(dynamic_cast<Diagram::GridComponent*>(component)) icon = ICON_FA_TH;
				else if(dynamic_cast<Diagram::BlockComponent*>(component)) icon = ICON_FA_SQUARE;
				else if(dynamic_cast<Diagram::GroupComponent*>(component)) icon = ICON_FA_OBJECT_GROUP;
				else if(dynamic_cast<Diagram::MinimapComponent*>(component)) icon = ICON_FA_MAP;
				
				ImGui::PushID(component);
				
				const std::string label = " " + std::string(icon) + "  " + component->GetDisplayName();
				bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags);
				
				// Draw multi-selection background after the item is rendered
				if(isMultiSelected) {
					ImVec2 itemMin = ImGui::GetItemRectMin();
					ImVec2 itemMax = ImGui::GetItemRectMax();
					ImDrawList* drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled(itemMin, itemMax, IM_COL32(0, 120, 215, 60)); // Semi-transparent blue background
					ImGui::PopStyleColor(); // Restore text color
				}
				
				// Drag and drop source
				if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
					ImGui::SetDragDropPayload("COMPONENT_REORDER", &component, sizeof(Diagram::Component*));
					ImGui::Text("%s Move %s", icon, component->GetDisplayName().c_str());
					draggedComponent = component;
					isDragging = true;
					ImGui::EndDragDropSource();
				} else {
					// Reset drag state when not dragging
					if(draggedComponent == component) {
						draggedComponent = nullptr;
						isDragging = false;
						showInsertionLine = false;
					}
				}
				
				// Drag and drop target with visual feedback
				if(ImGui::BeginDragDropTarget()) {
					// Get item rectangle for insertion line calculation
					ImVec2 itemMin = ImGui::GetItemRectMin();
					ImVec2 itemMax = ImGui::GetItemRectMax();
					ImVec2 mousePos = ImGui::GetMousePos();
					
					// Determine if we should insert above or below based on mouse position
					float itemCenterY = (itemMin.y + itemMax.y) * 0.5f;
					bool shouldInsertBelow = mousePos.y > itemCenterY;
					
					// Check for sibling drops first (these take priority over container drops)
					Diagram::Component* targetParent = component->GetParent();
					bool canDropAsSibling = targetParent && draggedComponent && 
											draggedComponent->GetParent();
					
					// Check for container drops (only if not a sibling drop in same parent)
					bool canDropOntoContainer = (dynamic_cast<Diagram::GridComponent*>(component) || 
												 dynamic_cast<Diagram::GroupComponent*>(component)) &&
												draggedComponent && draggedComponent != component;
					
					if(canDropAsSibling) {
						// Show insertion line for sibling positioning - no container highlight
						showInsertionLine = true;
						insertionLineY = shouldInsertBelow ? itemMax.y : itemMin.y;
						insertBelow = shouldInsertBelow;
						
						// Draw insertion line
						ImDrawList* drawList = ImGui::GetWindowDrawList();
						ImVec2 lineStart = ImVec2(itemMin.x, insertionLineY);
						ImVec2 lineEnd = ImVec2(itemMax.x, insertionLineY);
						drawList->AddLine(lineStart, lineEnd, IM_COL32(0, 120, 215, 255), 2.0f);
						
						// Draw insertion indicator
						ImVec2 indicatorPos = ImVec2(itemMin.x - 5, insertionLineY);
						drawList->AddCircleFilled(indicatorPos, 3.0f, IM_COL32(0, 120, 215, 255));
					}
					
					if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT_REORDER")) {
						Diagram::Component* sourceComponent = *(Diagram::Component**)payload->Data;
						
						// Only allow valid moves
						if(sourceComponent && sourceComponent != component) {
							Diagram::Component* targetParent = component->GetParent();
							Diagram::Component* sourceParent = sourceComponent->GetParent();
							
							// Check if we can drop as sibling (same parent or different parent)
							bool canDropAsSibling = targetParent && sourceParent;
							bool canDropOntoContainer = (dynamic_cast<Diagram::GridComponent*>(component) || 
														 dynamic_cast<Diagram::GroupComponent*>(component)) &&
														sourceComponent != component;
							
							if(canDropAsSibling) {
								// Remove from source parent
								auto removedComponent = sourceParent->RemoveChild(sourceComponent);
								
								// Drop as sibling - insert at correct position based on insertion line
								auto& siblings = targetParent->GetChildren();
								
								// Find the position of the target component
								size_t insertPos = siblings.size(); // Default to end
								for(size_t i = 0; i < siblings.size(); ++i) {
									if(siblings[i].get() == component) {
										insertPos = shouldInsertBelow ? i + 1 : i;
										break;
									}
								}
								
								targetParent->AddChildAt(std::move(removedComponent), insertPos);
							} else if(canDropOntoContainer) {
								// Remove from source parent
								auto removedComponent = sourceParent->RemoveChild(sourceComponent);
								
								// Drop onto container - add as child
								component->AddChild(std::move(removedComponent));
							}
						}
						
						// Reset insertion line
						showInsertionLine = false;
					}
					
					// No need to clean up style color - we removed all drag drop target highlighting
					ImGui::EndDragDropTarget();
				} else {
					// Reset insertion line when not hovering
					if(draggedComponent != component) {
						showInsertionLine = false;
					}
				}
				
				if(ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
					Diagram::TreeRenderer::SetActiveComponent(component);
					
					// Support multi-selection with Ctrl
					if(!ImGui::GetIO().KeyCtrl) {
						// Clear all selections if Ctrl is not held
						auto& componentList = diagramData.GetComponentList();
						for(auto& comp : componentList) {
							comp->Deselect();
						}
						diagramData.GetCamera().Deselect();
						diagramData.GetGrid().Deselect();
						
						// Clear all children selections recursively
						std::function<void(Diagram::Component*)> clearSelections = [&](Diagram::Component* comp) {
							comp->Deselect();
							for(const auto& child : comp->GetChildren()) {
								clearSelections(child.get());
							}
						};
						clearSelections(&diagramData.GetGrid());
						
						// Select only this component
						component->Select();
					} else {
						// Toggle selection for this component when Ctrl is held
						if(component->IsSelected()) {
							component->Deselect();
						} else {
							component->Select();
						}
					}
				}
				
				ImGui::TableNextColumn();
				
				// Action buttons
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
				
				// Add button for container components
				if(dynamic_cast<Diagram::GridComponent*>(component) || dynamic_cast<Diagram::GroupComponent*>(component)) {
					if(ImGui::Button(ICON_FA_PLUS)) {
						auto newBlock = std::make_unique<Diagram::BlockComponent>("New Block", glm::vec2{0.0f, 0.0f}, glm::vec2{100.0f, 50.0f});
						newBlock->id = "block_" + std::to_string(component->GetChildren().size() + 1);
						component->AddChild(std::move(newBlock));
					}
					if(ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Add new block to this container");
					}
					ImGui::SameLine();
				}
				
				// Delete button for deletable components (not camera or main grid)
				if(!dynamic_cast<Diagram::CameraComponent*>(component) && 
				   !(dynamic_cast<Diagram::GridComponent*>(component) && component == &diagramData.GetGrid())) {
					if(ImGui::Button(ICON_FA_TRASH)) {
						// Mark for deletion - we'll handle it after the tree rendering
						componentToDelete = component;
						spdlog::info("Marking component for deletion: {}", component->GetDisplayName());
					}
					if(ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Delete this component");
					}
					ImGui::SameLine();
				}
				
				// Settings button
				if(ImGui::Button(ICON_FA_COG)) {
					Diagram::TreeRenderer::AddToEditor(component);
				}
				if(ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Add to component editor");
				}
				
				ImGui::PopStyleVar();
				
				ImGui::TableNextColumn();
				
				// This column is now used for the global group controls in the header
				// Individual rows don't need group buttons anymore
				
				// Render children
				if(isOpen) {
					for(const auto& child : component->GetChildren()) {
						renderComponentRecursive(child.get(), depth + 1);
					}
					ImGui::TreePop();
				}
				
				ImGui::PopID();
			};
			
			// Render minimap as screen overlay component (first, so it appears at top)
			renderComponentRecursive(&diagramData.GetMinimap(), 0);
			
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
		
		// Check if there are orphaned components that need fixing
		auto& componentList = diagramData.GetComponentList();
		bool hasOrphanedBlocks = false;
		for(const auto& component : componentList) {
			if(auto* blockComp = dynamic_cast<Diagram::BlockComponent*>(component.get())) {
				if(!blockComp->GetParent()) {
					hasOrphanedBlocks = true;
					break;
				}
			}
		}
		
		// Only show fix hierarchy button if there are orphaned blocks
		if(hasOrphanedBlocks) {
			if(ImGui::Button("Fix Hierarchy - Move Orphaned Blocks to Grid")) {
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
		}
		
		// Handle component deletion after tree rendering to avoid modifying during iteration
		if(componentToDelete) {
			spdlog::info("Deleting component: {}", componentToDelete->GetDisplayName());
			
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
				spdlog::info("Removing component from parent");
				auto removedComponent = componentToDelete->GetParent()->RemoveChild(componentToDelete);
				spdlog::info("Component removed from parent successfully");
				// removedComponent will be destroyed when it goes out of scope
			} else {
				// Remove from main component list if it's orphaned
				spdlog::info("Removing orphaned component from main list");
				componentList.erase(
					std::remove_if(componentList.begin(), componentList.end(),
						[componentToDelete](const std::unique_ptr<Diagram::Component>& comp) {
							return comp.get() == componentToDelete;
						}),
					componentList.end()
				);
				spdlog::info("Orphaned component removed from main list successfully");
			}
			
			// componentToDelete is now invalid - don't use it after this point
			componentToDelete = nullptr;  // Reset the pointer to avoid accidental reuse
			spdlog::info("Component deletion completed");
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

	ImGui::Separator();

	// Show UI for selected components
	Diagram::Component* selectedComponent = nullptr;
	
	// Check camera
	if(camera.IsSelected()) {
		selectedComponent = &camera;
	}
	
	// Check grid
	if(!selectedComponent && diagramData.GetGrid().IsSelected()) {
		selectedComponent = &diagramData.GetGrid();
	}
	
	// Check minimap
	if(!selectedComponent && diagramData.GetMinimap().IsSelected()) {
		selectedComponent = &diagramData.GetMinimap();
	}
	
	// Check all components in hierarchy
	if(!selectedComponent) {
		std::function<Diagram::Component*(Diagram::Component*)> findSelected = [&](Diagram::Component* comp) -> Diagram::Component* {
			if(comp->IsSelected()) return comp;
			for(const auto& child : comp->GetChildren()) {
				if(auto found = findSelected(child.get())) return found;
			}
			return nullptr;
		};
		selectedComponent = findSelected(&diagramData.GetGrid());
	}

	if(selectedComponent) {
		ImGui::Text("Selected: %s", selectedComponent->GetDisplayName().c_str());
		selectedComponent->RenderUIOfficial();
	} else {
		ImGui::TextDisabled("No component selected");
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

	// Initialize SDL_ttf
	if(TTF_Init() == -1) {
		throw std::runtime_error("TTF_Init error: " + std::string(TTF_GetError()));
	}

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

bool Application::HandleMinimapEvent(const SDL_Event& event, const Diagram::CameraView& view, const glm::vec2& screenSize) noexcept {
	auto& minimap = diagramData.GetMinimap();
	
	if(!minimap.data.visible || !minimap.IsVisible()) {
		return false;
	}

	// Calculate minimap screen position (right side)
	const float minimapX = screenSize.x - minimap.size.x - 10.0f;
	const float minimapY = 10.0f;

	// Check if mouse is over minimap
	glm::vec2 mousePos;
	if(event.type == SDL_MOUSEMOTION) {
		mousePos = {(float)event.motion.x, (float)event.motion.y};
	} else if(event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
		mousePos = {(float)event.button.x, (float)event.button.y};
	} else {
		return false;
	}

	const bool isOverMinimap = (mousePos.x >= minimapX && mousePos.x <= minimapX + minimap.size.x &&
							   mousePos.y >= minimapY && mousePos.y <= minimapY + minimap.size.y);

	static bool isDraggingViewport = false;

	if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT && isOverMinimap) {
		isDraggingViewport = true;
		
		// Jump camera to clicked position (with sensitivity control)
		JumpCameraToMinimapPosition(mousePos, {minimapX, minimapY}, view, screenSize, minimap);
		return true;
	}

	if(event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
		if(isDraggingViewport) {
			isDraggingViewport = false;
			return true;
		}
	}

	if(event.type == SDL_MOUSEMOTION && isDraggingViewport) {
		// Move camera based on minimap drag (with sensitivity control)
		JumpCameraToMinimapPosition(mousePos, {minimapX, minimapY}, view, screenSize, minimap);
		return true;
	}

	return false;
}

void Application::JumpCameraToMinimapPosition(const glm::vec2& mousePos, const glm::vec2& minimapPos, const Diagram::CameraView& view, const glm::vec2& screenSize, const Diagram::MinimapComponent& minimap) noexcept {
	// Safety check for zoom level
	const float safeZoom = std::max(0.001f, std::min(1000.0f, view.zoom));
	
	// Convert minimap click to world position with sensitivity control
	const glm::vec2 minimapWorldSize = minimap.size * minimap.data.zoomFactor / safeZoom;
	const glm::vec2 minimapWorldMin = view.worldPosition - minimapWorldSize * 0.5f;
	const glm::vec2 minimapWorldMax = view.worldPosition + minimapWorldSize * 0.5f;

	// Safety check for world bounds
	const float worldWidth = minimapWorldMax.x - minimapWorldMin.x;
	const float worldHeight = minimapWorldMax.y - minimapWorldMin.y;
	if(worldWidth <= 0.0f || worldHeight <= 0.0f) return;

	const float normalizedX = std::max(0.0f, std::min(1.0f, (mousePos.x - minimapPos.x) / minimap.size.x));
	const float normalizedY = std::max(0.0f, std::min(1.0f, (mousePos.y - minimapPos.y) / minimap.size.y));

	const glm::vec2 targetWorldPos = minimapWorldMin + glm::vec2(normalizedX, normalizedY) * glm::vec2(worldWidth, worldHeight);

	// Safety check for target position
	if(std::abs(targetWorldPos.x) > 100000.0f || std::abs(targetWorldPos.y) > 100000.0f) {
		return;  // Skip if target position is too extreme
	}

	// Apply sensitivity - interpolate between current and target position
	const glm::vec2 currentPos = view.worldPosition;
	const glm::vec2 newPos = currentPos + (targetWorldPos - currentPos) * minimap.data.cameraSensitivity;

	// Update camera position
	auto& camera = diagramData.GetCamera();
	camera.SetPosition(newPos);
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
