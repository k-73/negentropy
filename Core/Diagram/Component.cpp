#include "Component.hpp"
#include "Block.hpp"
#include "imgui.h"
#include <algorithm>
#include <set>
#include "../Utils/IconsFontAwesome5.h"
#include <imgui_internal.h>

namespace Diagram {
    ComponentBase* ComponentBase::s_selected = nullptr;

    void ComponentBase::RenderComponentTree(std::vector<std::unique_ptr<ComponentBase>>& components) noexcept {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

        if (!ImGui::Begin("Component Tree")) {
            ImGui::End();
            return;
        }

        if (ImGui::BeginTable("TreeTable", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadInnerX)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 48.0f);
            
            auto hierarchy = BuildHierarchy(components);
            if (hierarchy) {
                std::string hoveredRowId;
                RenderTreeNode(*hierarchy, &components, 0, hoveredRowId);
            }

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

        ImGui::PopStyleVar();
    }

    void ComponentBase::RenderTreeNode(const TreeNode& node, std::vector<std::unique_ptr<ComponentBase>>* components, int depth, std::string& hoveredRowId) noexcept {
        static constexpr float TREE_INDENT = 4.0f;
        static std::set<std::string> expanded;

        const char* icon = node.component ? ICON_FA_CUBE : ICON_FA_SITEMAP;
        std::string nodeKey = node.name + std::to_string(reinterpret_cast<uintptr_t>(node.component));
        bool hasChildren = !node.children.empty();
        bool isExpanded = expanded.contains(nodeKey) || (node.name == "Scene" && expanded.empty());
        bool isSceneRoot = node.name == "Scene" && depth == 0;
        
        ImGui::PushID(nodeKey.c_str());
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        
        ImGui::Indent(static_cast<float>(depth) * TREE_INDENT);
        
        if (hasChildren && !isSceneRoot) {
            if (ImGui::ArrowButton("##expand", isExpanded ? ImGuiDir_Down : ImGuiDir_Right)) {
                if (isExpanded) expanded.erase(nodeKey);
                else expanded.insert(nodeKey);
            }
            ImGui::SameLine(0, 2);
        } else if (node.component) {
            ImGui::Dummy(ImVec2(ImGui::GetFrameHeight(), 0));
            ImGui::SameLine(0, 2);
        }
        
        std::string displayText = " " + std::string(icon) + "  " + node.name;
        bool selectableClicked = ImGui::Selectable(displayText.c_str(), s_selected == node.component);
        bool nameHovered = ImGui::IsItemHovered();
        
        if (selectableClicked && node.component) {
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
        
        if (node.component) {
            if (nameHovered || ImGui::IsMouseHoveringRect(ImGui::GetCursorScreenPos(), 
                ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x, ImGui::GetCursorScreenPos().y + ImGui::GetFrameHeight()))) {
                hoveredRowId = nodeKey;
            }
            
            RenderActionButtons(nodeKey, hoveredRowId, components, node.component);
        } else if (node.name == "Scene") {
            RenderCenteredIcon(ICON_FA_FOLDER);
        }
        
        if ((isExpanded || isSceneRoot) && hasChildren) {
            for (const auto& child : node.children) RenderTreeNode(*child, components, depth + 1, hoveredRowId);
        }
        
        ImGui::PopID();
    }

    void ComponentBase::RenderActionButtons(const std::string& nodeKey, const std::string& hoveredRowId, std::vector<std::unique_ptr<ComponentBase>>* components, ComponentBase* component) noexcept {
        constexpr float spacing = 2.0f;
        constexpr float padding = 4.0f;
        
        const ImVec2 trash_size = ImGui::CalcTextSize(ICON_FA_TRASH);
        const ImVec2 more_size = ImGui::CalcTextSize(ICON_FA_ELLIPSIS_H);
        const float row_height = ImGui::GetFrameHeight();
        const ImVec2 btn_size1(trash_size.x + padding, row_height);
        const ImVec2 btn_size2(more_size.x + padding, row_height);
        
        const float total_width = btn_size1.x + spacing + btn_size2.x;
        const float start_x = ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;
        const float adjusted_y = ImGui::GetCursorPosY() + (ImGui::GetTextLineHeightWithSpacing() - row_height) * 0.5f - 1.0f;
        
        ImGui::SetCursorPos(ImVec2(start_x, adjusted_y));
        
        const std::string popup_id = "more_popup_" + nodeKey;
        const bool visible = hoveredRowId == nodeKey || ImGui::IsPopupOpen(popup_id.c_str());
        
        if (ImGui::InvisibleButton("##trash", btn_size1)) {
            if (auto it = std::ranges::find_if(*components, [&](const auto& c) { return c.get() == component; }); it != components->end()) {
                if (s_selected == component) ClearSelection();
                components->erase(it);
                ImGui::PopID();
                return;
            }
        }
        
        if (visible) {
            const ImVec4 color = ImGui::IsItemHovered() ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
            ImVec2 pos = ImGui::GetItemRectMin();
            pos.x += (btn_size1.x - trash_size.x) * 0.5f;
            pos.y += (btn_size1.y - trash_size.y) * 0.5f;
            ImGui::GetWindowDrawList()->AddText(pos, ImGui::GetColorU32(color), ICON_FA_TRASH);
        }
        
        ImGui::SameLine(0.0f, spacing);
        
        if (ImGui::InvisibleButton("##more", btn_size2)) {
            ImGui::OpenPopup(popup_id.c_str());
        }
        
        if (visible) {
            const bool highlighted = ImGui::IsItemHovered() || ImGui::IsPopupOpen(popup_id.c_str());
            const ImVec4 color = highlighted ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
            ImVec2 pos = ImGui::GetItemRectMin();
            pos.x += (btn_size2.x - more_size.x) * 0.5f;
            pos.y += (btn_size2.y - more_size.y) * 0.5f;
            ImGui::GetWindowDrawList()->AddText(pos, ImGui::GetColorU32(color), ICON_FA_ELLIPSIS_H);
        }
        
        if (ImGui::BeginPopup(popup_id.c_str())) {
            ImGui::TextDisabled("%s", component->GetDisplayName().c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Rename")) { /* Action */ }
            if (ImGui::MenuItem("Duplicate")) { /* Action */ }
            ImGui::Separator();
            if (ImGui::MenuItem("Copy")) { /* Action */ }
            if (ImGui::MenuItem("Paste")) { /* Action */ }
            ImGui::EndPopup();
        }
    }

    void ComponentBase::RenderCenteredIcon(const char* icon) noexcept {
        const float icon_width = ImGui::CalcTextSize(icon).x;
        const float start_x = ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - icon_width) * 0.5f;
        ImGui::SetCursorPosX(start_x);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Text("%s", icon);
        ImGui::PopStyleColor();
    }

    std::unique_ptr<ComponentBase::TreeNode> ComponentBase::BuildHierarchy(const std::vector<std::unique_ptr<ComponentBase>>& components) noexcept {
        auto root = std::make_unique<TreeNode>("Scene");
        for (const auto& component : components) {
            root->children.push_back(std::make_unique<TreeNode>(component->GetDisplayName(), component.get()));
        }
        return root;
    }
}