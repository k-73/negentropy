#pragma once

#include <memory>
#include <vector>

#include "Components/BlockComponent.hpp"
#include "Components/CameraComponent.hpp"
#include "Components/GridComponent.hpp"
#include "Components/GroupComponent.hpp"
#include "Utils/IconsFontAwesome5.h"  // Assuming you have this file
#include "Utils/Notification.hpp"
#include "imgui.h"

namespace Diagram
{
	class TreeRenderer
	{
	private:
		inline static Component* sActiveComponent = nullptr;
		inline static std::vector<Component*> sUiComponents;

	public:
		static Component* GetActiveComponent() { return sActiveComponent; }
		static void SetActiveComponent(Component* component) { sActiveComponent = component; }
		static void ClearActiveComponent() { sActiveComponent = nullptr; }

		static void RenderComponentTree(Component* rootComponent) {
			if(!ImGui::Begin("Component Tree")) {
				ImGui::End();
				return;
			}

			if(ImGui::BeginTable("TreeTable", 2, ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 64.0f);
				RenderNode(rootComponent, 0);
				ImGui::EndTable();
			}

			ImGui::End();
		}

		static void RenderComponentEditor() {
			if(!ImGui::Begin("Component Editor")) {
				ImGui::End();
				return;
			}

			if(sUiComponents.empty()) {
				if(sActiveComponent) {
					sActiveComponent->RenderUIOfficial();
				} else {
					ImGui::TextDisabled("Select a component to edit");
				}
			} else {
				for(auto* component: sUiComponents) {
					if(ImGui::CollapsingHeader(component->GetDisplayName().c_str())) {
						component->RenderUIOfficial();
					}
				}
			}

			ImGui::End();
		}

		static void AddToEditor(Component* component) {
			if(std::find(sUiComponents.begin(), sUiComponents.end(), component) == sUiComponents.end()) {
				sUiComponents.push_back(component);
			}
		}

		static void RemoveFromEditor(Component* component) {
			std::erase(sUiComponents, component);
		}

	private:
		static void RenderNode(Component* component, int depth) {
			if(!component) return;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowItemOverlap;
			if(sActiveComponent == component) flags |= ImGuiTreeNodeFlags_Selected;
			if(component->GetChildren().empty()) flags |= ImGuiTreeNodeFlags_Leaf;

			ImGui::PushID(component);

			const char* icon = ICON_FA_CUBE;
			if(dynamic_cast<GroupComponent*>(component) || dynamic_cast<GridComponent*>(component)) icon = ICON_FA_FOLDER;
			else if(dynamic_cast<CameraComponent*>(component)) icon = ICON_FA_VIDEO;

			const std::string label = " " + std::string(icon) + "  " + component->GetDisplayName();
			bool isOpen = ImGui::TreeNodeEx(label.c_str(), flags);

			if(ImGui::IsItemClicked(ImGuiMouseButton_Left)) SetActiveComponent(component);

			HandleDragDropSource(component);
			HandleDragDropTarget(component);

			ImGui::TableNextColumn();
			RenderActionButtons(component);

			if(isOpen) {
				for(const auto& child: component->GetChildren()) {
					RenderNode(child.get(), depth + 1);
				}
				ImGui::TreePop();
			}

			ImGui::PopID();
		}

		static void RenderActionButtons(Component* component) {
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

			// Add Button for containers
			if(dynamic_cast<GroupComponent*>(component) || dynamic_cast<GridComponent*>(component)) {
				if(ImGui::Button(ICON_FA_PLUS)) {
					component->AddChild(std::make_unique<BlockComponent>("New Block", glm::vec2 {0, 0}, glm::vec2 {150, 50}));
				}
				ImGui::SameLine();
			}

			// Delete Button for non-root components
			if(component->GetParent()) {
				if(ImGui::Button(ICON_FA_TRASH)) {
					if(GetActiveComponent() == component) ClearActiveComponent();
					RemoveFromEditor(component);
					component->GetParent()->RemoveChild(component);
					ImGui::PopStyleVar();
					return;
				}
				ImGui::SameLine();
			}

			// "More Actions" Popup Button
			const std::string popupId = "popup_" + std::to_string(reinterpret_cast<uintptr_t>(component));
			if(ImGui::Button(ICON_FA_ELLIPSIS_H)) {
				ImGui::OpenPopup(popupId.c_str());
			}

			if(ImGui::BeginPopup(popupId.c_str())) {
				ImGui::TextDisabled("%s", component->GetDisplayName().c_str());
				ImGui::Separator();
				if(ImGui::MenuItem("Add to Editor")) AddToEditor(component);
				if(ImGui::MenuItem("Remove from Editor")) RemoveFromEditor(component);
				ImGui::Separator();
				ImGui::TextDisabled("More actions...");
				ImGui::EndPopup();
			}

			ImGui::PopStyleVar();
		}

		static void HandleDragDropSource(Component* component) {
			if(!component->GetParent()) return;

			if(ImGui::BeginDragDropSource()) {
				ImGui::SetDragDropPayload("COMPONENT_DND", &component, sizeof(Component*));
				ImGui::Text("Move %s", component->GetDisplayName().c_str());
				ImGui::EndDragDropSource();
			}
		}

		static void HandleDragDropTarget(Component* targetComponent) {
			if(ImGui::BeginDragDropTarget()) {
				if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT_DND")) {
					Component* draggedComponent = *(Component**)payload->Data;

					const float dropTargetHeight = ImGui::GetItemRectSize().y;
					const float dropTargetY = ImGui::GetItemRectMin().y;
					const float dropMouseY = ImGui::GetMousePos().y;

					const float topThird = dropTargetY + dropTargetHeight * 0.33f;
					const float bottomThird = dropTargetY + dropTargetHeight * 0.66f;

					if(dropMouseY < topThird) {
						Component* parent = targetComponent->GetParent();
						if(parent) {
							auto& children = parent->GetChildren();
							auto it = std::find_if(children.begin(), children.end(), [&](const auto& p) {
								return p.get() == targetComponent;
							});
							size_t index = std::distance(children.begin(), it);
							Component::Move(draggedComponent, parent, index);
							Notify::Success("Component reordered");
						}
					} else if(dropMouseY > bottomThird) {
						Component* parent = targetComponent->GetParent();
						if(parent) {
							auto& children = parent->GetChildren();
							auto it = std::find_if(children.begin(), children.end(), [&](const auto& p) {
								return p.get() == targetComponent;
							});
							size_t index = std::distance(children.begin(), it) + 1;
							Component::Move(draggedComponent, parent, index);
							Notify::Success("Component reordered");
						}
					} else {
						// Only allow dropping inside a container component
						if(dynamic_cast<GroupComponent*>(targetComponent) || dynamic_cast<GridComponent*>(targetComponent)) {
							Component::Move(draggedComponent, targetComponent, targetComponent->GetChildren().size());
							Notify::Success("Component moved to " + targetComponent->GetDisplayName());
						}
					}
				}
				ImGui::EndDragDropTarget();
			}
		}
	};

}  // namespace Diagram
