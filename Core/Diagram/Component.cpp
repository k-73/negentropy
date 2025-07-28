#include "Component.hpp"
#include "Block.hpp"
#include "imgui.h"
#include <algorithm>
#include <set>
#include <map>
#include <functional>
#include <cstring>
#include "../Utils/IconsFontAwesome5.h"
#include "../Utils/Notification.hpp"
#include <imgui_internal.h>

namespace Diagram {
    ComponentBase* ComponentBase::s_selected = nullptr;
    std::map<std::string, std::string> ComponentBase::s_groupParents;
    std::map<std::string, std::string> ComponentBase::s_groupNames;
    std::map<std::string, bool> ComponentBase::s_groupExpanded;
    std::function<void(const std::map<std::string, std::string>&)> ComponentBase::s_onGroupsChanged;
    std::function<void(const std::map<std::string, bool>&)> ComponentBase::s_onExpandedChanged;

    void ComponentBase::RenderComponentTree(std::vector<std::unique_ptr<ComponentBase>>& components, const std::map<std::string, std::string>& groups, const std::map<std::string, std::string>& groupNames, std::function<void(const std::map<std::string, std::string>&)> onGroupsChanged, const std::map<std::string, bool>& groupExpanded, std::function<void(const std::map<std::string, bool>&)> onExpandedChanged) noexcept {
        s_groupParents = groups;
        s_groupNames = groupNames;
        s_groupExpanded = groupExpanded;
        s_onGroupsChanged = onGroupsChanged;
        s_onExpandedChanged = onExpandedChanged;
        
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.2f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.25f, 0.25f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.3f, 0.3f, 0.3f, 0.3f));
        
        if (!ImGui::Begin("Component Tree")) {
            ImGui::PopStyleColor(3);
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
        ImGui::PopStyleColor(3);
        ImGui::End();
    }

    void ComponentBase::RenderComponentEditor() noexcept {
        if (!ImGui::Begin("Component Editor")) {
            ImGui::End();
            return;
        }

        if (!s_selected) {
            ImGui::TextDisabled("Select a component to edit");
            ImGui::End();
            return;
        }

        static std::map<ComponentBase*, char[64]> idBuffers;
        if (!idBuffers.contains(s_selected)) {
            std::strncpy(idBuffers[s_selected], s_selected->id.c_str(), 63);
            idBuffers[s_selected][63] = '\0';
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
        
        ImGui::Text(ICON_FA_CUBE " %s", s_selected->GetDisplayName().c_str());
        ImGui::Separator();
        
        if (auto* block = dynamic_cast<Block*>(s_selected)) {
            block->RenderUI(0);
        }

        ImGui::PopStyleVar();
        ImGui::End();
    }

    void ComponentBase::RenderTreeNode(const TreeNode& node, std::vector<std::unique_ptr<ComponentBase>>* components, int depth, std::string& hoveredRowId) noexcept {
        static constexpr float TREE_INDENT = 16.0f;

        const char* icon = node.component ? ICON_FA_CUBE : (node.isGroup ? ICON_FA_FOLDER : ICON_FA_SITEMAP);

        std::string nodeKey = node.name + std::to_string(reinterpret_cast<uintptr_t>(node.component)) + (node.isGroup ? "_group" : "");
        bool hasChildren = !node.children.empty();
        bool isExpanded = false;
        
        if (node.isGroup) {
            isExpanded = s_groupExpanded.contains(node.groupId) ? s_groupExpanded[node.groupId] : true;
        } else if (node.name == "Scene" && depth == 0) {
            isExpanded = true;
        }
        
        bool isSceneRoot = node.name == "Scene" && depth == 0;
        
        ImGui::PushID(nodeKey.c_str());
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        
        ImGui::Indent(static_cast<float>(depth) * TREE_INDENT);
        
        if (hasChildren && !isSceneRoot) {
            const char* arrow = isExpanded ? ICON_FA_CHEVRON_DOWN : ICON_FA_CHEVRON_RIGHT;
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            
            ImGui::Text("%s", arrow);
            bool arrowClicked = ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            
            if (arrowClicked && node.isGroup) {
                s_groupExpanded[node.groupId] = !isExpanded;
                if (s_onExpandedChanged) s_onExpandedChanged(s_groupExpanded);
            }
            
            ImGui::PopStyleColor();
            ImGui::SameLine(0, 4);
        } else if (node.component || node.isGroup) {
            ImGui::Dummy(ImVec2(16, 0));
            ImGui::SameLine(0, 4);
        }
        
        std::string displayText = " " + std::string(icon) + "  " + node.name;
        bool isSelected = node.component && s_selected == node.component;
        bool selectableClicked = ImGui::Selectable(displayText.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);
        bool nameHovered = ImGui::IsItemHovered();
        
        if (selectableClicked && node.component) {
            Select(node.component);
        } else if (selectableClicked && node.isGroup) {
            ClearSelection();
        }
        
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && (node.isGroup || node.name == "Scene") && hasChildren) {
            if (node.isGroup) {
                s_groupExpanded[node.groupId] = !isExpanded;
                if (s_onExpandedChanged) s_onExpandedChanged(s_groupExpanded);
            }
        }
        
        if (node.component && ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("COMPONENT_DND", &node.component, sizeof(void*));
            ImGui::Text("Moving: %s", node.name.c_str());
            ImGui::EndDragDropSource();
        } else if (node.isGroup && ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("GROUP_DND", node.groupId.c_str(), node.groupId.size() + 1);
            ImGui::Text("Moving Group: %s", node.name.c_str());
            ImGui::EndDragDropSource();
        }
        
        if (ImGui::BeginDragDropTarget()) {
            if (const auto* payload = ImGui::AcceptDragDropPayload("COMPONENT_DND")) {
                auto* dragged = static_cast<ComponentBase**>(payload->Data)[0];
                if (dragged && dragged != node.component) {
                    if (node.isGroup) {
                        dragged->groupId = node.groupId;
                        Notify::Success("Component moved to group: " + node.name);
                    } else if (node.component) {
                        auto draggedIt = std::ranges::find_if(*components, [&](const auto& c) { return c.get() == dragged; });
                        auto targetIt = std::ranges::find_if(*components, [&](const auto& c) { return c.get() == node.component; });
                        
                        if (draggedIt != components->end() && targetIt != components->end()) {
                            std::string draggedGroupId = dragged->groupId;
                            std::string targetGroupId = node.component->groupId;
                            
                            dragged->groupId = targetGroupId;
                            node.component->groupId = draggedGroupId;
                            
                            std::swap(*draggedIt, *targetIt);
                            Notify::Success("Components swapped positions and groups");
                        }
                    } else if (node.name == "Scene") {
                        dragged->groupId.clear();
                        Notify::Success("Component moved to Scene");
                    }
                }
            }
            if (const auto* payload = ImGui::AcceptDragDropPayload("GROUP_DND")) {
                std::string draggedGroupId(static_cast<const char*>(payload->Data));
                if (draggedGroupId != node.groupId && !draggedGroupId.empty()) {
                    if (node.isGroup && !IsGroupDescendant(node.groupId, draggedGroupId) && !IsGroupDescendant(draggedGroupId, node.groupId)) {
                        s_groupParents[draggedGroupId] = node.groupId;
                        if (s_onGroupsChanged) s_onGroupsChanged(s_groupParents);
                        Notify::Success("Group moved to: " + node.name);
                    } else if (node.component) {
                        const std::string& targetGroup = node.component->groupId;
                        if (!IsGroupDescendant(targetGroup, draggedGroupId)) {
                            s_groupParents[draggedGroupId] = targetGroup;
                            if (s_onGroupsChanged) s_onGroupsChanged(s_groupParents);
                            if (targetGroup.empty()) {
                                Notify::Success("Group moved to Scene (via component)");
                            } else {
                                Notify::Success("Group moved to component's group");
                            }
                        } else {
                            Notify::Warning("Cannot create circular group dependency!");
                        }
                    } else if (node.name == "Scene") {
                        s_groupParents[draggedGroupId] = "";
                        if (s_onGroupsChanged) s_onGroupsChanged(s_groupParents);
                        Notify::Success("Group moved to Scene");
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
        
        ImGui::Unindent(static_cast<float>(depth) * TREE_INDENT);
        ImGui::TableNextColumn();
        
        if (node.component) {
            if (nameHovered) hoveredRowId = nodeKey;
            
            RenderActionButtons(nodeKey, hoveredRowId, components, node.component);
        } else if (node.isGroup) {
            if (nameHovered) hoveredRowId = nodeKey;
            RenderGroupActions(nodeKey, hoveredRowId);
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
        const bool popupOpen = ImGui::IsPopupOpen(popup_id.c_str());
        static std::string activeButton;
        const bool visible = hoveredRowId == nodeKey || popupOpen || activeButton == nodeKey;
        
        if (ImGui::InvisibleButton("##trash", btn_size1)) {
            if (auto it = std::ranges::find_if(*components, [&](const auto& c) { return c.get() == component; }); it != components->end()) {
                if (s_selected == component) ClearSelection();
                components->erase(it);
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
        
        if (ImGui::IsItemActive()) {
            activeButton = nodeKey;
        } else if (activeButton == nodeKey && !ImGui::IsItemHovered() && !popupOpen) {
            activeButton.clear();
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
            ImGui::TextDisabled("No actions implemented");
            ImGui::EndPopup();
        }
    }

    void ComponentBase::RenderGroupActions(const std::string& nodeKey, const std::string& hoveredRowId) noexcept {
        constexpr float spacing = 2.0f;
        constexpr float padding = 4.0f;
        
        const ImVec2 add_size = ImGui::CalcTextSize(ICON_FA_PLUS);
        const ImVec2 more_size = ImGui::CalcTextSize(ICON_FA_ELLIPSIS_H);
        const float row_height = ImGui::GetFrameHeight();
        const ImVec2 btn_size1(add_size.x + padding, row_height);
        const ImVec2 btn_size2(more_size.x + padding, row_height);
        
        const float total_width = btn_size1.x + spacing + btn_size2.x;
        const float start_x = ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;
        const float adjusted_y = ImGui::GetCursorPosY() + (ImGui::GetTextLineHeightWithSpacing() - row_height) * 0.5f - 1.0f;
        
        ImGui::SetCursorPos(ImVec2(start_x, adjusted_y));
        
        const std::string popup_id = "group_popup_" + nodeKey;
        const bool popupOpen = ImGui::IsPopupOpen(popup_id.c_str());
        static std::string activeGroupButton;
        const bool visible = hoveredRowId == nodeKey || popupOpen || activeGroupButton == nodeKey;
        
        if (ImGui::InvisibleButton("##add", btn_size1)) {
            // TODO: Add new component to group
        }
        
        if (ImGui::IsItemActive()) {
            activeGroupButton = nodeKey;
        } else if (activeGroupButton == nodeKey && !ImGui::IsItemHovered() && !popupOpen) {
            activeGroupButton.clear();
        }
        
        if (visible) {
            const ImVec4 color = ImGui::IsItemHovered() ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
            ImVec2 pos = ImGui::GetItemRectMin();
            pos.x += (btn_size1.x - add_size.x) * 0.5f;
            pos.y += (btn_size1.y - add_size.y) * 0.5f;
            ImGui::GetWindowDrawList()->AddText(pos, ImGui::GetColorU32(color), ICON_FA_PLUS);
        }
        
        ImGui::SameLine(0.0f, spacing);
        
        if (ImGui::InvisibleButton("##group_more", btn_size2)) {
            ImGui::OpenPopup(popup_id.c_str());
        }
        
        if (ImGui::IsItemActive()) {
            activeGroupButton = nodeKey;
        } else if (activeGroupButton == nodeKey && !ImGui::IsItemHovered() && !popupOpen) {
            activeGroupButton.clear();
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
            ImGui::TextDisabled("Group Actions");
            ImGui::Separator();
            ImGui::TextDisabled("No actions implemented");
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
        std::map<std::string, TreeNode*> groupNodes;
        std::vector<std::unique_ptr<TreeNode>> allGroups;
        
        for (const auto& [groupId, parentId] : s_groupParents) {
            std::string groupName = s_groupNames.contains(groupId) ? s_groupNames[groupId] : groupId;
            auto groupNode = std::make_unique<TreeNode>(groupName, nullptr, true, groupId);
            groupNodes[groupId] = groupNode.get();
            allGroups.push_back(std::move(groupNode));
        }
        
        for (const auto& component : components) {
            auto node = std::make_unique<TreeNode>(component->GetDisplayName(), component.get());
            
            if (!component->groupId.empty()) {
                if (groupNodes.contains(component->groupId)) {
                    groupNodes[component->groupId]->children.push_back(std::move(node));
                } else {
                    root->children.push_back(std::move(node));
                }
            } else {
                root->children.push_back(std::move(node));
            }
        }
        
        for (auto& groupNode : allGroups) {
            const std::string& parentId = s_groupParents[groupNode->groupId];
            if (parentId.empty() || !groupNodes.contains(parentId)) {
                root->children.push_back(std::move(groupNode));
            } else {
                groupNodes[parentId]->children.push_back(std::move(groupNode));
            }
        }
        
        return root;
    }

    bool ComponentBase::IsGroupDescendant(const std::string& groupId, const std::string& potentialAncestor) noexcept {
        if (groupId == potentialAncestor) return true;
        if (!s_groupParents.contains(groupId)) return false;
        
        std::string parent = s_groupParents[groupId];
        while (!parent.empty()) {
            if (parent == potentialAncestor) return true;
            if (!s_groupParents.contains(parent)) break;
            parent = s_groupParents[parent];
        }
        return false;
    }
}