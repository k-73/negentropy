#include "Component.hpp"
#include "Block.hpp"
#include <imgui.h>
#include <algorithm>

namespace Diagram {
    Component* Component::s_selected = nullptr;

    void Component::RenderComponentTree(std::vector<std::unique_ptr<Component>>& components) noexcept {
        if (!ImGui::Begin("Component Tree")) {
            ImGui::End();
            return;
        }

        auto hierarchy = BuildHierarchy(components);
        if (hierarchy) {
            RenderTreeNode(*hierarchy, &components);
        }

        ImGui::End();
    }

    void Component::RenderComponentEditor() noexcept {
        if (!ImGui::Begin("Component Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::End();
            return;
        }

        if (!s_selected) {
            ImGui::TextDisabled("No component selected");
            ImGui::End();
            return;
        }

        if (auto* block = dynamic_cast<Block*>(s_selected)) {
            block->RenderUI(0);
        } else {
            ImGui::TextDisabled("Unknown component type");
        }

        ImGui::End();
    }

    void Component::RenderTreeNode(const TreeNode& node, std::vector<std::unique_ptr<Component>>* components) noexcept {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
        
        if (node.children.empty()) {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }
        
        if (s_selected == node.component) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        bool nodeOpen = ImGui::TreeNodeEx(node.name.c_str(), flags);
        
        if (ImGui::IsItemClicked() && node.component) {
            Select(node.component);
        }

        // Drag & drop tylko dla komponentów (nie dla root node)
        if (node.component) {
            if (ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload("COMPONENT_DND", &node.component, sizeof(Component*));
                ImGui::Text("Moving: %s", node.name.c_str());
                ImGui::EndDragDropSource();
            }
        }

        if (nodeOpen && !node.children.empty()) {
            for (auto& child : node.children) {
                RenderTreeNode(*child, components);
                
                // Drop target dla reorder
                if (child->component && ImGui::BeginDragDropTarget()) {
                    if (const auto* payload = ImGui::AcceptDragDropPayload("COMPONENT_DND")) {
                        auto* draggedComp = *static_cast<Component**>(payload->Data);
                        if (components && draggedComp != child->component) {
                            auto draggedIt = std::find_if(components->begin(), components->end(),
                                [draggedComp](const auto& c) { return c.get() == draggedComp; });
                            auto targetIt = std::find_if(components->begin(), components->end(),
                                [&child](const auto& c) { return c.get() == child->component; });
                            
                            if (draggedIt != components->end() && targetIt != components->end()) {
                                std::iter_swap(draggedIt, targetIt);
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
            }
            ImGui::TreePop();
        }
    }

    std::unique_ptr<Component::TreeNode> Component::BuildHierarchy(const std::vector<std::unique_ptr<Component>>& components) noexcept {
        auto root = std::make_unique<TreeNode>("Scene");
        
        for (const auto& component : components) {
            auto componentNode = std::make_unique<TreeNode>(
                component->GetDisplayName(),
                component.get()
            );
            root->children.push_back(std::move(componentNode));
        }
        
        return root;
    }
}