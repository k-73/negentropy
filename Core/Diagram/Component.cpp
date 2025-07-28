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
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 28.0f);
            
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
        ImGui::PushID(nodeKey.c_str());
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        
        ImGui::Indent(static_cast<float>(depth) * TREE_INDENT);
        
        if (hasChildren) {
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
            ImVec2 columnStart = ImGui::GetCursorScreenPos();
            ImVec2 columnEnd = ImVec2(columnStart.x + ImGui::GetContentRegionAvail().x, columnStart.y + ImGui::GetFrameHeight());
            bool actionsColumnHovered = ImGui::IsMouseHoveringRect(columnStart, columnEnd);
            
            if (nameHovered || actionsColumnHovered) {
                hoveredRowId = nodeKey;
            }
            
            std::string popup_id = "more_popup_" + nodeKey;
            if (hoveredRowId == nodeKey || ImGui::IsPopupOpen(popup_id.c_str())) {
                // --- Configuration ---
                const char* trash_icon = ICON_FA_TRASH;
                const char* more_icon = ICON_FA_ELLIPSIS_H;
                const ImGuiStyle& style = ImGui::GetStyle();
                const float spacing = style.ItemSpacing.x / 2.0f;

                // --- Size Calculations ---
                const ImVec2 trash_label_size = ImGui::CalcTextSize(trash_icon);
                const ImVec2 more_label_size = ImGui::CalcTextSize(more_icon);
                const float button_height = ImGui::GetFrameHeight();
                const float button_padding = style.FramePadding.x * 2.0f;
                const ImVec2 trash_button_size(trash_label_size.x + button_padding, button_height);
                const ImVec2 more_button_size(more_label_size.x + button_padding, button_height);
                const float total_width = trash_button_size.x + spacing + more_button_size.x;

                // --- Centered Positioning ---
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - total_width) * 0.5f);

                // --- Render Trash Button ---
                if (ImGui::InvisibleButton("##trash", trash_button_size)) {
                    if (auto it = std::ranges::find_if(*components, [&](const auto& c) { return c.get() == node.component; }); it != components->end()) {
                        if (s_selected == node.component) ClearSelection();
                        components->erase(it);
                        ImGui::PopID();
                        return;
                    }
                }
                bool trash_hovered = ImGui::IsItemHovered();
                ImVec4 trash_textColor = trash_hovered ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
                ImVec2 trash_text_pos = ImGui::GetItemRectMin();
                trash_text_pos.x += (trash_button_size.x - trash_label_size.x) * 0.5f;
                trash_text_pos.y += (trash_button_size.y - trash_label_size.y) * 0.5f;
                ImGui::GetWindowDrawList()->AddText(trash_text_pos, ImGui::GetColorU32(trash_textColor), trash_icon);

                ImGui::SameLine(0.0f, spacing);

                // --- Render More Button ---
                if (ImGui::InvisibleButton("##more", more_button_size)) {
                    ImGui::OpenPopup(popup_id.c_str());
                }

                bool more_hovered = ImGui::IsItemHovered();
                bool popup_open = ImGui::IsPopupOpen(popup_id.c_str());

                ImVec4 more_textColor = (more_hovered || popup_open) ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
                ImVec2 more_text_pos = ImGui::GetItemRectMin();
                more_text_pos.x += (more_button_size.x - more_label_size.x) * 0.5f;
                more_text_pos.y += (more_button_size.y - more_label_size.y) * 0.5f;
                ImGui::GetWindowDrawList()->AddText(more_text_pos, ImGui::GetColorU32(more_textColor), more_icon);

                if (ImGui::BeginPopup(popup_id.c_str())) {
                    ImGui::TextDisabled("%s", node.name.c_str());
                    ImGui::Separator();
                    if (ImGui::MenuItem("Rename")) { /* Action */ }
                    if (ImGui::MenuItem("Duplicate")) { /* Action */ }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Copy")) { /* Action */ }
                    if (ImGui::MenuItem("Paste")) { /* Action */ }
                    ImGui::EndPopup();
                }
            }
        } else if (node.name == "Scene") {
            float availWidth = ImGui::GetContentRegionAvail().x;
            float iconWidth = ImGui::CalcTextSize(ICON_FA_FOLDER).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - iconWidth) * 0.5f);
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            ImGui::Text(ICON_FA_FOLDER);
            ImGui::PopStyleColor();
        }
        
        if (isExpanded && hasChildren) {
            for (const auto& child : node.children) RenderTreeNode(*child, components, depth + 1, hoveredRowId);
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