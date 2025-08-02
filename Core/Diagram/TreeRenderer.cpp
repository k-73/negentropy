// #include "TreeRenderer.hpp"
// #include "Component.hpp"
// #include "Block.hpp"
// #include "imgui.h"
// #include <algorithm>
// #include <cstring>
// #include "../Utils/IconsFontAwesome5.h"
// #include "../Utils/Notification.hpp"
// #include <imgui_internal.h>
// #include <ranges>

// namespace Diagram {
//     TreeRenderer::GroupState TreeRenderer::s_groups;

//     void TreeRenderer::RenderComponentTree(std::vector<std::unique_ptr<ComponentBase>>& componentList, const GroupState& config) noexcept {
//         s_groups = config;
        
//         ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.2f, 0.3f));
//         ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.25f, 0.25f, 0.3f));
//         ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.3f, 0.3f, 0.3f, 0.3f));
        
//         if (!ImGui::Begin("Component Tree")) {
//             ImGui::PopStyleColor(3);
//             ImGui::End();
//             return;
//         }

//         if (ImGui::BeginTable("TreeTable", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadInnerX)) {
//             ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
//             ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 48.0f);
            
//             auto hierarchy = BuildHierarchy(componentList);
//             if (hierarchy) {
//                 std::string hoveredRowId;
//                 RenderTreeNode(*hierarchy, &componentList, 0, hoveredRowId);
//             }

//             ImGui::EndTable();
//         }
//         ImGui::PopStyleColor(3);
//         ImGui::End();
//     }

//     void TreeRenderer::RenderComponentEditor() noexcept {
//         if (!ImGui::Begin("Component Editor")) {
//             ImGui::End();
//             return;
//         }

//         auto* s_selected = ComponentBase::GetSelected();
//         if (!s_selected) {
//             ImGui::TextDisabled("Select a component to edit");
//             ImGui::End();
//             return;
//         }

//         static std::map<ComponentBase*, char[64]> idBuffers;
//         if (!idBuffers.contains(s_selected)) {
//             std::strncpy(idBuffers[s_selected], s_selected->id.c_str(), 63);
//             idBuffers[s_selected][63] = '\0';
//         }

//         ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));

//         if (auto* block = dynamic_cast<Block*>(s_selected)) {
//             block->RenderUI(0);
//         }

//         ImGui::PopStyleVar();
//         ImGui::End();
//     }

//     void TreeRenderer::RenderTreeNode(const TreeNode& node, std::vector<std::unique_ptr<ComponentBase>>* componentList, const int depth, std::string& hoveredRowId) noexcept {
//         static constexpr float TREE_INDENT = 16.0f;

//         const char* icon = node.component ? ICON_FA_CUBE : node.isGroup ? ICON_FA_FOLDER : ICON_FA_SITEMAP;
//         const std::string nodeKey = node.name + std::to_string(reinterpret_cast<uintptr_t>(node.component)) + (node.isGroup ? "_group" : "");
//         const bool hasChildren = !node.children.empty();
//         const bool isSceneRoot = node.name == "Scene" && depth == 0;
//         const bool isExpanded = isSceneRoot || (node.isGroup && s_groups.expanded[node.groupId]);
        
//         ImGui::PushID(nodeKey.c_str());
//         ImGui::TableNextRow();
//         ImGui::TableNextColumn();
        
//         ImGui::Indent(static_cast<float>(depth) * TREE_INDENT);
        
//         if (hasChildren && !isSceneRoot) {
//             const char* arrow = isExpanded ? ICON_FA_CHEVRON_DOWN : ICON_FA_CHEVRON_RIGHT;
//             ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            
//             ImGui::Text("%s", arrow);
//             bool arrowClicked = ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            
//             if (arrowClicked && node.isGroup) {
//                 s_groups.expanded[node.groupId] = !isExpanded;
//                 if (s_groups.onExpandedChanged) s_groups.onExpandedChanged(s_groups.expanded);
//             }
            
//             ImGui::PopStyleColor();
//             ImGui::SameLine(0, 4);
//         } else if (node.component || node.isGroup) {
//             ImGui::Dummy(ImVec2(16, 0));
//             ImGui::SameLine(0, 4);
//         }
        
//         const std::string displayText = std::string(" ") + icon + "  " + node.name;
//         const bool isSelected = node.component && ComponentBase::GetSelected() == node.component;
//         const bool selectableClicked = ImGui::Selectable(displayText.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);
//         const bool nameHovered = ImGui::IsItemHovered();
        
//         if (selectableClicked) {
//             if (node.component) ComponentBase::Select(node.component);
//             else if (node.isGroup) ComponentBase::ClearSelection();
//         }
        
//         if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && node.isGroup && hasChildren) {
//             s_groups.expanded[node.groupId] = !isExpanded;
//             if (s_groups.onExpandedChanged) s_groups.onExpandedChanged(s_groups.expanded);
//         }
        
//         if (ImGui::BeginDragDropSource()) {
//             if (node.component) {
//                 ImGui::SetDragDropPayload("COMPONENT_DND", &node.component, sizeof(void*));
//                 ImGui::Text("Moving: %s", node.name.c_str());
//             } else if (node.isGroup) {
//                 ImGui::SetDragDropPayload("GROUP_DND", node.groupId.c_str(), node.groupId.size() + 1);
//                 ImGui::Text("Moving Group: %s", node.name.c_str());
//             }
//             ImGui::EndDragDropSource();
//         }
        
//         HandleDragDrop(node, componentList);
        
//         ImGui::Unindent(static_cast<float>(depth) * TREE_INDENT);
//         ImGui::TableNextColumn();
        
//         if (nameHovered) hoveredRowId = nodeKey;
        
//         if (node.component) {
//             RenderActionButtons(nodeKey, hoveredRowId, componentList, node.component);
//         } else if (node.isGroup) {
//             RenderGroupActions(nodeKey, hoveredRowId);
//         } else if (isSceneRoot) {
//             RenderCenteredIcon(ICON_FA_FOLDER);
//         }
        
//         if ((isExpanded || isSceneRoot) && hasChildren) {
//             for (const auto& child : node.children) RenderTreeNode(*child, componentList, depth + 1, hoveredRowId);
//         }
        
//         ImGui::PopID();
//     }

//     void TreeRenderer::RenderIconButton(const char* icon, const ImVec2& size, const bool visible, const bool highlighted) noexcept {
//         if (!visible) return;
//         const ImVec4 color = highlighted ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
//         const ImVec2 icon_size = ImGui::CalcTextSize(icon);
//         ImVec2 pos = ImGui::GetItemRectMin();
//         pos.x += (size.x - icon_size.x) * 0.5f;
//         pos.y += (size.y - icon_size.y) * 0.5f;
//         ImGui::GetWindowDrawList()->AddText(pos, ImGui::GetColorU32(color), icon);
//     }

//     bool TreeRenderer::SetupActionButtons(const std::string& nodeKey, const std::string& hoveredRowId, const std::vector<const char*>& icons) noexcept {
//         constexpr float spacing = 2.0f;
//         const float row_height = ImGui::GetFrameHeight();
        
//         float total_width = icons.size() > 1 ? (icons.size() - 1) * spacing : 0;
//         for (const char* icon : icons) {
//             constexpr float padding = 4.0f;
//             total_width += ImGui::CalcTextSize(icon).x + padding;
//         }
        
//         const float start_x = ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;
//         const float adjusted_y = ImGui::GetCursorPosY() + (ImGui::GetTextLineHeightWithSpacing() - row_height) * 0.5f - 1.0f;
//         ImGui::SetCursorPos(ImVec2(start_x, adjusted_y));
        
//         const std::string popup_id = "popup_" + nodeKey;
//         const bool popupOpen = ImGui::IsPopupOpen(popup_id.c_str());
//         return hoveredRowId == nodeKey || popupOpen;
//     }

//     void TreeRenderer::RenderActionButtons(const std::string& nodeKey, const std::string& hoveredRowId, std::vector<std::unique_ptr<ComponentBase>>* componentList, ComponentBase* component) noexcept {
//         const std::vector<const char*> icons = {ICON_FA_TRASH, ICON_FA_ELLIPSIS_H};
//         const bool visible = SetupActionButtons(nodeKey, hoveredRowId, icons);
        
//         if (ImGui::InvisibleButton("##trash", ImVec2(ImGui::CalcTextSize(ICON_FA_TRASH).x + 4, ImGui::GetFrameHeight()))) {
//             if (auto it = std::ranges::find_if(*componentList, [&](const auto& c) { return c.get() == component; }); it != componentList->end()) {
//                 if (ComponentBase::GetSelected() == component) ComponentBase::ClearSelection();
//                 componentList->erase(it);
//                 return;
//             }
//         }
//         RenderIconButton(ICON_FA_TRASH, ImGui::GetItemRectSize(), visible, ImGui::IsItemHovered());
        
//         ImGui::SameLine(0.0f, 2.0f);
//         const std::string popup_id = "popup_" + nodeKey;
//         if (ImGui::InvisibleButton("##more", ImVec2(ImGui::CalcTextSize(ICON_FA_ELLIPSIS_H).x + 4, ImGui::GetFrameHeight()))) {
//             ImGui::OpenPopup(popup_id.c_str());
//         }
//         RenderIconButton(ICON_FA_ELLIPSIS_H, ImGui::GetItemRectSize(), visible, ImGui::IsItemHovered() || ImGui::IsPopupOpen(popup_id.c_str()));
        
//         if (ImGui::BeginPopup(popup_id.c_str())) {
//             ImGui::TextDisabled("%s", component->GetDisplayName().c_str());
//             ImGui::Separator();
//             ImGui::TextDisabled("No actions implemented");
//             ImGui::EndPopup();
//         }
//     }

//     void TreeRenderer::RenderGroupActions(const std::string& nodeKey, const std::string& hoveredRowId) noexcept {
//         const std::vector<const char*> icons = {ICON_FA_PLUS, ICON_FA_ELLIPSIS_H};
//         const bool visible = SetupActionButtons(nodeKey, hoveredRowId, icons);
        
//         if (ImGui::InvisibleButton("##add", ImVec2(ImGui::CalcTextSize(ICON_FA_PLUS).x + 4, ImGui::GetFrameHeight()))) {
//             // TODO: Add new component to group
//         }
//         RenderIconButton(ICON_FA_PLUS, ImGui::GetItemRectSize(), visible, ImGui::IsItemHovered());
        
//         ImGui::SameLine(0.0f, 2.0f);
//         const std::string popup_id = "popup_" + nodeKey;
//         if (ImGui::InvisibleButton("##group_more", ImVec2(ImGui::CalcTextSize(ICON_FA_ELLIPSIS_H).x + 4, ImGui::GetFrameHeight()))) {
//             ImGui::OpenPopup(popup_id.c_str());
//         }
//         RenderIconButton(ICON_FA_ELLIPSIS_H, ImGui::GetItemRectSize(), visible, ImGui::IsItemHovered() || ImGui::IsPopupOpen(popup_id.c_str()));
        
//         if (ImGui::BeginPopup(popup_id.c_str())) {
//             ImGui::TextDisabled("Group Actions");
//             ImGui::Separator();
//             ImGui::TextDisabled("No actions implemented");
//             ImGui::EndPopup();
//         }
//     }

//     void TreeRenderer::RenderCenteredIcon(const char* icon) noexcept {
//         const float start_x = ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(icon).x) * 0.5f;
//         ImGui::SetCursorPosX(start_x);
//         ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
//         ImGui::Text("%s", icon);
//         ImGui::PopStyleColor();
//     }

//     void TreeRenderer::HandleDragDrop(const TreeNode& node, std::vector<std::unique_ptr<ComponentBase>>* componentList) noexcept {
//         if (!ImGui::BeginDragDropTarget()) return;
        
//         if (const auto* payload = ImGui::AcceptDragDropPayload("COMPONENT_DND")) {
//             auto* dragged = static_cast<ComponentBase**>(payload->Data)[0];
//             if (!dragged || dragged == node.component) return;
            
//             if (node.isGroup) {
//                 dragged->groupId = node.groupId;
//                 Notify::Success("Component moved to group: " + node.name);
//             } else if (node.component) {
//                 auto draggedIt = std::ranges::find_if(*componentList, [&](const auto& c) { return c.get() == dragged; });
//                 auto targetIt = std::ranges::find_if(*componentList, [&](const auto& c) { return c.get() == node.component; });
//                 if (draggedIt != componentList->end() && targetIt != componentList->end()) {
//                     std::swap(dragged->groupId, node.component->groupId);
//                     std::swap(*draggedIt, *targetIt);
//                     Notify::Success("Components swapped positions and groups");
//                 }
//             } else if (node.name == "Scene") {
//                 dragged->groupId.clear();
//                 Notify::Success("Component moved to Scene");
//             }
//         }
        
//         if (const auto* payload = ImGui::AcceptDragDropPayload("GROUP_DND")) {
//             std::string draggedGroupId(static_cast<const char*>(payload->Data));
//             if (draggedGroupId == node.groupId || draggedGroupId.empty()) return;
            
//             if (node.isGroup && !IsGroupDescendant(node.groupId, draggedGroupId)) {
//                 s_groups.parents[draggedGroupId] = node.groupId;
//                 if (s_groups.onGroupsChanged) s_groups.onGroupsChanged(s_groups.parents);
//                 Notify::Success("Group moved to: " + node.name);
//             } else if (node.component && !IsGroupDescendant(node.component->groupId, draggedGroupId)) {
//                 s_groups.parents[draggedGroupId] = node.component->groupId;
//                 if (s_groups.onGroupsChanged) s_groups.onGroupsChanged(s_groups.parents);
//                 Notify::Success(node.component->groupId.empty() ? "Group moved to Scene (via component)" : "Group moved to component's group");
//             } else if (node.name == "Scene") {
//                 s_groups.parents[draggedGroupId] = "";
//                 if (s_groups.onGroupsChanged) s_groups.onGroupsChanged(s_groups.parents);
//                 Notify::Success("Group moved to Scene");
//             } else {
//                 if (node.component || node.isGroup) Notify::Warning("Cannot create circular group dependency!");
//                 return;
//             }
//         }
        
//         ImGui::EndDragDropTarget();
//     }

//     std::unique_ptr<TreeRenderer::TreeNode> TreeRenderer::BuildHierarchy(const std::vector<std::unique_ptr<ComponentBase>>& componentList) noexcept {
//         auto root = std::make_unique<TreeNode>("Scene");
//         std::map<std::string, TreeNode*> groupNodes;
//         std::vector<std::unique_ptr<TreeNode>> allGroups;
        
//         for (const auto &groupId: s_groups.parents | std::views::keys) {
//             auto it = s_groups.names.find(groupId);
//             std::string groupName = it != s_groups.names.end() ? it->second : groupId;
//             auto groupNode = std::make_unique<TreeNode>(groupName, nullptr, true, groupId);
//             groupNodes[groupId] = groupNode.get();
//             allGroups.push_back(std::move(groupNode));
//         }
        
//         for (const auto& component : componentList) {
//             auto node = std::make_unique<TreeNode>(component->GetDisplayName(), component.get());
            
//             if (!component->groupId.empty() && groupNodes.contains(component->groupId)) {
//                 groupNodes[component->groupId]->children.push_back(std::move(node));
//             } else {
//                 root->children.push_back(std::move(node));
//             }
//         }
        
//         for (auto& groupNode : allGroups) {
//             const std::string& parentId = s_groups.parents[groupNode->groupId];
//             if (parentId.empty() || !groupNodes.contains(parentId)) {
//                 root->children.push_back(std::move(groupNode));
//             } else {
//                 groupNodes[parentId]->children.push_back(std::move(groupNode));
//             }
//         }
        
//         return root;
//     }

//     bool TreeRenderer::IsGroupDescendant(const std::string& groupId, const std::string& potentialAncestor) noexcept {
//         if (groupId == potentialAncestor) return true;
//         if (!s_groups.parents.contains(groupId)) return false;
        
//         std::string parent = s_groups.parents[groupId];
//         while (!parent.empty()) {
//             if (parent == potentialAncestor) return true;
//             if (!s_groups.parents.contains(parent)) break;
//             parent = s_groups.parents[parent];
//         }
//         return false;
//     }
// }