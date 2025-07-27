#include "Component.hpp"
#include "Block.hpp"
#include <imgui.h>

namespace Diagram {
    Component* Component::s_selected = nullptr;

    void Component::RenderComponentTree(const std::vector<std::unique_ptr<Component>>& components) noexcept {
        if (!ImGui::Begin("Component Tree")) {
            ImGui::End();
            return;
        }

        auto hierarchy = BuildHierarchy(components);
        if (hierarchy) {
            RenderTreeNode(*hierarchy);
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

    void Component::RenderTreeNode(const TreeNode& node) noexcept {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        
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

        if (nodeOpen && !node.children.empty()) {
            for (auto& child : node.children) {
                RenderTreeNode(*child);
            }
            ImGui::TreePop();
        }
    }

    std::unique_ptr<Component::TreeNode> Component::BuildHierarchy(const std::vector<std::unique_ptr<Component>>& components) noexcept {
        auto root = std::make_unique<TreeNode>("Scene");
        auto componentsNode = std::make_unique<TreeNode>("Components");
        
        for (size_t i = 0; i < components.size(); ++i) {
            const auto& component = components[i];
            auto componentNode = std::make_unique<TreeNode>(
                component->GetDisplayName(),
                component.get()
            );
            componentsNode->children.push_back(std::move(componentNode));
        }
        
        root->children.push_back(std::move(componentsNode));
        return root;
    }
}