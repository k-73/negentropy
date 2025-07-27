#include "Component.hpp"
#include "Block.hpp"
#include "imgui.h"
#include <algorithm>
#include <set>
#include "../Utils/IconsFontAwesome5.h"

namespace Diagram {
    ComponentBase* ComponentBase::s_selected = nullptr;

    void ComponentBase::RenderComponentTree(std::vector<std::unique_ptr<ComponentBase>>& components) noexcept {
        if (!ImGui::Begin("Component Tree")) {
            ImGui::End();
            return;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
        auto hierarchy = BuildHierarchy(components);
        if (hierarchy) RenderTreeNode(*hierarchy, &components);
        ImGui::PopStyleVar();
        ImGui::End();
    }

    void ComponentBase::RenderComponentEditor() noexcept {
        if (!ImGui::Begin("Component Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::End();
            return;
        }

        if (!s_selected) {
            ImGui::TextDisabled("No component selected");
        } else if (auto* block = dynamic_cast<Block*>(s_selected)) {
            block->RenderUI(0);
        } else {
            ImGui::TextDisabled("Unknown component type");
        }

        ImGui::End();
    }

    void ComponentBase::RenderTreeNode(const TreeNode& node, std::vector<std::unique_ptr<ComponentBase>>* components) noexcept {
        static int depth = 0;
        static std::set<std::string> expanded;
        
        const char* icon = node.component ? ICON_FA_CUBE : ICON_FA_SITEMAP;
        std::string nodeKey = node.name + std::to_string(reinterpret_cast<uintptr_t>(node.component));
        bool hasChildren = !node.children.empty();
        bool isExpanded = expanded.contains(nodeKey) || (node.name == "Scene" && expanded.empty());
        
        ImGui::PushID(nodeKey.c_str());
        if (depth > 0) ImGui::Indent(20.0f);
        
        if (hasChildren) {
            if (ImGui::ArrowButton("##expand", isExpanded ? ImGuiDir_Down : ImGuiDir_Right)) {
                if (isExpanded) expanded.erase(nodeKey);
                else expanded.insert(nodeKey);
            }
            ImGui::SameLine();
        } else {
            ImGui::Dummy(ImVec2(16, 0)); 
            ImGui::SameLine();
        }
        
        float width = ImGui::GetContentRegionAvail().x - (node.component ? 25 : 0);
        if (ImGui::Selectable((std::string(icon) + "  " + node.name).c_str(), 
                             s_selected == node.component, 0, ImVec2(width, 0))) {
            if (node.component) Select(node.component);
        }
        
        if (node.component) {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.3f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.3f, 0.3f, 0.5f));
            
            if (ImGui::Button(ICON_FA_TRASH)) {
                auto it = std::find_if(components->begin(), components->end(),
                    [&](const auto& c) { return c.get() == node.component; });
                if (it != components->end()) {
                    if (s_selected == node.component) ClearSelection();
                    components->erase(it);
                    ImGui::PopStyleColor(3);
                    if (depth > 0) ImGui::Unindent(20.0f);
                    ImGui::PopID();
                    return;
                }
            }
            ImGui::PopStyleColor(3);
            
            if (ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload("COMPONENT_DND", &node.component, sizeof(void*));
                ImGui::Text("Moving: %s", node.name.c_str());
                ImGui::EndDragDropSource();
            }
        }
        
        if (depth > 0) ImGui::Unindent(20.0f);
        
        if (isExpanded && hasChildren) {
            depth++;
            for (auto& child : node.children) {
                RenderTreeNode(*child, components);
            }
            depth--;
        }
        
        ImGui::PopID();
    }

    std::unique_ptr<ComponentBase::TreeNode> ComponentBase::BuildHierarchy(const std::vector<std::unique_ptr<ComponentBase>>& components) noexcept {
        auto root = std::make_unique<TreeNode>("Scene");
        for (const auto& component : components) {
            root->children.push_back(std::make_unique<TreeNode>(component->GetDisplayName(), component.get()));
        }
        return root;
    }
}