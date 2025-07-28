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

        if (ImGui::BeginTable("TreeTable", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 20.0f);
            
            auto hierarchy = BuildHierarchy(components);
            if (hierarchy) RenderTreeNode(*hierarchy, &components);
            
            ImGui::EndTable();
        }
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
        static constexpr float TREE_INDENT = 4.0f;
        static int depth = 0;
        static std::set<std::string> expanded;
        
        const char* icon = node.component ? ICON_FA_CUBE : ICON_FA_SITEMAP;
        std::string nodeKey = node.name + std::to_string(reinterpret_cast<uintptr_t>(node.component));
        bool hasChildren = !node.children.empty();
        bool isExpanded = expanded.contains(nodeKey) || (node.name == "Scene" && expanded.empty());
        
        ImGui::PushID(nodeKey.c_str());
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        
        ImGui::Indent(static_cast<float>(depth) * TREE_INDENT);
        
        if (hasChildren && node.component) {
            if (ImGui::ArrowButton("##expand", isExpanded ? ImGuiDir_Down : ImGuiDir_Right)) {
                if (isExpanded) expanded.erase(nodeKey);
                else expanded.insert(nodeKey);
            }
            ImGui::SameLine(0, 2);
        } else if (node.component) {
            ImGui::Dummy(ImVec2(16, 0));
            ImGui::SameLine(0, 2);
        }
        
        if (ImGui::Selectable((std::string(icon) + "  " + node.name).c_str(), s_selected == node.component) && node.component) {
            Select(node.component);
        }
        
        if (node.component && ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("COMPONENT_DND", &node.component, sizeof(void*));
            ImGui::Text("Moving: %s", node.name.c_str());
            ImGui::EndDragDropSource();
        }
        
        if (ImGui::BeginDragDropTarget()) {
            if (const auto* payload = ImGui::AcceptDragDropPayload("COMPONENT_DND")) {
                auto* dragged = static_cast<ComponentBase**>(payload->Data)[0];
                if (dragged && dragged != node.component) {
                    auto draggedIt = std::ranges::find_if(*components, [dragged](const auto& c) { return c.get() == dragged; });
                    if (draggedIt != components->end()) {
                        auto targetIt = node.component ? std::ranges::find_if(*components, [&](const auto& c) { return c.get() == node.component; }) : components->end();
                        
                        ptrdiff_t draggedIdx = std::distance(components->begin(), draggedIt);
                        ptrdiff_t targetIdx = targetIt != components->end() ? std::distance(components->begin(), targetIt) : static_cast<ptrdiff_t>(components->size());
                        
                        auto draggedPtr = std::move(*draggedIt);
                        components->erase(draggedIt);
                        
                        if (!node.component) {
                            components->insert(components->begin(), std::move(draggedPtr));
                        } else if (draggedIdx < targetIdx) {
                            components->insert(components->begin() + targetIdx, std::move(draggedPtr));
                        } else {
                            components->insert(components->begin() + targetIdx, std::move(draggedPtr));
                        }
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
        
        ImGui::Unindent(static_cast<float>(depth) * TREE_INDENT);
        ImGui::TableNextColumn();
        
        // Delete button
        if (node.component) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.3f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.3f, 0.3f, 0.5f));
            
            if (ImGui::SmallButton(ICON_FA_TRASH "##trash")) {
                if (auto it = std::ranges::find_if(*components, [&](const auto& c) { return c.get() == node.component; }); it != components->end()) {
                    if (s_selected == node.component) ClearSelection();
                    components->erase(it);
                    ImGui::PopStyleColor(3);
                    ImGui::PopID();
                    return;
                }
            }
            ImGui::PopStyleColor(3);
        }
        
        // Render children
        if (isExpanded && hasChildren) {
            ++depth;
            for (const auto& child : node.children) RenderTreeNode(*child, components);
            --depth;
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