#pragma once

#include <glm/vec2.hpp>
#include <SDL.h>
#include <pugixml.hpp>
#include <vector>
#include <string>
#include <memory>

namespace Diagram {
    struct Camera;
    class Block;
    
    class Component {
    public:
        virtual ~Component() = default;
        
        virtual bool HandleEvent(const SDL_Event& event, const Camera& camera, glm::vec2 screenSize) noexcept = 0;
        virtual void Render(SDL_Renderer* renderer, const Camera& camera, glm::vec2 screenSize) const noexcept = 0;
        virtual bool Contains(glm::vec2 point) const noexcept = 0;
        virtual void xml_serialize(pugi::xml_node& node) const = 0;
        virtual void xml_deserialize(const pugi::xml_node& node) = 0;
        
        static Component* GetSelected() noexcept { return s_selected; }
        static void Select(Component* component) noexcept { s_selected = component; }
        static void ClearSelection() noexcept { s_selected = nullptr; }
        
        static void RenderComponentTree(const std::vector<Block>& blocks) noexcept;
        static void RenderComponentEditor() noexcept;
        
    private:
        static Component* s_selected;
        
        struct TreeNode {
            std::string name;
            Component* component = nullptr;
            std::vector<std::unique_ptr<TreeNode>> children;
            
            TreeNode(std::string n, Component* c = nullptr) 
                : name(std::move(n)), component(c) {}
        };
        
        static void RenderTreeNode(TreeNode& node) noexcept;
        static std::unique_ptr<TreeNode> BuildHierarchy(const std::vector<Block>& blocks) noexcept;
    };
}