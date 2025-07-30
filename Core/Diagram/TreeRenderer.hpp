#pragma once

#include <vector>
#include <memory>
#include <string>
#include <map>
#include <functional>

struct ImVec2;
namespace Diagram {
    class ComponentBase;
}

namespace Diagram {
    
    class TreeRenderer {
    public:
        struct GroupState {
            std::map<std::string, std::string> parents;
            std::map<std::string, std::string> names;
            std::map<std::string, bool> expanded;
            std::function<void(const std::map<std::string, std::string>&)> onGroupsChanged;
            std::function<void(const std::map<std::string, bool>&)> onExpandedChanged;
        };
        
        static void RenderComponentTree(std::vector<std::unique_ptr<ComponentBase>>& componentList, const GroupState& config = {}) noexcept;
        static void RenderComponentEditor() noexcept;
        
    private:
        struct TreeNode {
            std::string name;
            ComponentBase* component = nullptr;
            bool isGroup = false;
            std::string groupId;
            std::vector<std::unique_ptr<TreeNode>> children;

            explicit TreeNode(std::string n, ComponentBase* c = nullptr, bool group = false, std::string gId = "") 
                : name(std::move(n)), component(c), isGroup(group), groupId(std::move(gId)) {}
        };
        
        static GroupState s_groups;
        
        static void RenderTreeNode(const TreeNode& node, std::vector<std::unique_ptr<ComponentBase>>* componentList, int depth, std::string& hoveredRowId) noexcept;
        static std::unique_ptr<TreeNode> BuildHierarchy(const std::vector<std::unique_ptr<ComponentBase>>& componentList) noexcept;
        static bool IsGroupDescendant(const std::string& groupId, const std::string& potentialAncestor) noexcept;
        
        static void RenderActionButtons(const std::string& nodeKey, const std::string& hoveredRowId, std::vector<std::unique_ptr<ComponentBase>>* componentList, ComponentBase* component) noexcept;
        static void RenderGroupActions(const std::string& nodeKey, const std::string& hoveredRowId) noexcept;
        static void RenderCenteredIcon(const char* icon) noexcept;
        static void RenderIconButton(const char* icon, const ImVec2& size, bool visible, bool highlighted) noexcept;
        static bool SetupActionButtons(const std::string& nodeKey, const std::string& hoveredRowId, const std::vector<const char*>& icons) noexcept;
        
        static void HandleDragDrop(const TreeNode& node, std::vector<std::unique_ptr<ComponentBase>>* componentList) noexcept;
    };
    
}