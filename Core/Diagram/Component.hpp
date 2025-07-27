#pragma once

#include <glm/vec2.hpp>
#include <SDL.h>
#include <pugixml.hpp>

namespace Diagram {
    struct Camera;
    
    class Component {
    public:
        virtual ~Component() = default;
        
        virtual bool HandleEvent(const SDL_Event& event, const Camera& camera, glm::vec2 screenSize) noexcept = 0;
        virtual void Render(SDL_Renderer* renderer, const Camera& camera, glm::vec2 screenSize) const noexcept = 0;
        virtual bool Contains(glm::vec2 point) const noexcept = 0;
        virtual void xml_serialize(pugi::xml_node& node) const = 0;
        virtual void xml_deserialize(const pugi::xml_node& node) = 0;
        virtual const char* GetTypeName() const noexcept = 0;
    };
}