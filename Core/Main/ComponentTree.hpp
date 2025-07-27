#pragma once

#include <imgui.h>
#include "ComponentHierarchy.hpp"
#include "ComponentSelection.hpp"

namespace UI {
    class ComponentTree {
    public:
        void Render(const ComponentHierarchy& hierarchy) noexcept {
            if (!ImGui::Begin("Component Tree")) {
                ImGui::End();
                return;
            }

            if (auto* root = hierarchy.GetRoot()) {
                RenderNode(*root);
            }

            ImGui::End();
        }

    private:
        void RenderNode(TreeNode& node) noexcept {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
            
            if (node.children.empty()) {
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            }
            
            bool isSelected = ComponentSelection::Instance().GetSelected() == node.component;
            if (isSelected) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            bool nodeOpen = ImGui::TreeNodeEx(node.name.c_str(), flags);
            
            if (ImGui::IsItemClicked() && node.component) {
                ComponentSelection::Instance().Select(node.component);
            }

            if (nodeOpen && !node.children.empty()) {
                for (auto& child : node.children) {
                    RenderNode(*child);
                }
                ImGui::TreePop();
            }
        }
    };
}