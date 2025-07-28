#pragma once

#include <glm/vec2.hpp>
#include <SDL.h>
#include <pugixml.hpp>
#include <vector>
#include <string>
#include <memory>
#include <typeinfo>
#include <algorithm>
#include <cxxabi.h>
#include <map>
#include <functional>

namespace Diagram {
    struct Camera;
    class Block;
    
    class ComponentBase {
    public:
        std::string groupId;
        std::string id;
        
        struct TreeNode {
            std::string name;
            ComponentBase* component = nullptr;
            bool isGroup = false;
            std::string groupId;
            std::vector<std::unique_ptr<TreeNode>> children;

            explicit TreeNode(std::string n, ComponentBase* c = nullptr, bool group = false, std::string gId = "") 
                : name(std::move(n)), component(c), isGroup(group), groupId(std::move(gId)) {}
        };

        virtual ~ComponentBase() = default;
        
        virtual bool HandleEvent(const SDL_Event& event, const Camera& camera, glm::vec2 screenSize) noexcept = 0;
        virtual void Render(SDL_Renderer* renderer, const Camera& camera, glm::vec2 screenSize) const noexcept = 0;
        virtual void xml_serialize(pugi::xml_node& node) const = 0;
        virtual void xml_deserialize(const pugi::xml_node& node) = 0;
        
        virtual std::string GetDisplayName() const noexcept = 0;
        virtual std::string GetTypeName() const noexcept = 0;
        
        static ComponentBase* GetSelected() noexcept { return s_selected; }
        static void Select(ComponentBase* component) noexcept { s_selected = component; }
        static void ClearSelection() noexcept { s_selected = nullptr; }
        
        static void RenderComponentTree(std::vector<std::unique_ptr<ComponentBase>>& components, const std::map<std::string, std::string>& groups = {}, const std::map<std::string, std::string>& groupNames = {}, std::function<void(const std::map<std::string, std::string>&)> onGroupsChanged = nullptr) noexcept;
        static void RenderComponentEditor() noexcept;
        
    private:
        static ComponentBase* s_selected;
        
        static void RenderTreeNode(const TreeNode& node, std::vector<std::unique_ptr<ComponentBase>>* components, int depth, std::string& hoveredRowId) noexcept;
        static void RenderActionButtons(const std::string& nodeKey, const std::string& hoveredRowId, std::vector<std::unique_ptr<ComponentBase>>* components, ComponentBase* component) noexcept;
        static void RenderGroupActions(const std::string& nodeKey, const std::string& hoveredRowId) noexcept;
        static void RenderCenteredIcon(const char* icon) noexcept;
        static std::unique_ptr<TreeNode> BuildHierarchy(const std::vector<std::unique_ptr<ComponentBase>>& components) noexcept;
        static bool IsGroupDescendant(const std::string& groupId, const std::string& potentialAncestor) noexcept;
        
        static std::map<std::string, std::string> s_groupParents;
        static std::map<std::string, std::string> s_groupNames;
        static std::function<void(const std::map<std::string, std::string>&)> s_onGroupsChanged;
    };
    
    template<typename T>
    std::string demangle() {
        int status;
        char* demangled = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
        std::string result = demangled ? demangled : "unknown";
        free(demangled);
        auto pos = result.find_last_of("::");
        if (pos != std::string::npos) result = result.substr(pos + 1);
        return result;
    }
    
    template<typename Derived>
    class Component : public ComponentBase {
    public:
        std::string GetTypeName() const noexcept override { return demangle<Derived>(); }
        static std::string GetStaticTypeName() noexcept { return demangle<Derived>(); }
    };
    
}