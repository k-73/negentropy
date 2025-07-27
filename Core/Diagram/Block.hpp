#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <pugixml.hpp>
#include <SDL.h>
#include "../Utils/XMLSerialization.hpp"
#include "Component.hpp"

namespace Diagram {
    struct Camera;
    
    class Block final : public Component {
    public:
        enum class Type {
            Start,
            Process,
            Decision,
            End
        };

        struct Data {
            glm::vec2 position{0.0f};
            glm::vec2 size{120.0f, 60.0f};
            std::string label;
            Type type = Type::Process;
            glm::vec4 backgroundColor{0.35f, 0.47f, 0.78f, 1.0f};
            glm::vec4 borderColor{1.0f, 1.0f, 1.0f, 1.0f};
        } data;

        bool HandleEvent(const SDL_Event& event, const Camera& camera, glm::vec2 screenSize) noexcept override;
        void Render(SDL_Renderer* renderer, const Camera& camera, glm::vec2 screenSize) const noexcept override;
        void xml_serialize(pugi::xml_node& node) const override;
        void xml_deserialize(const pugi::xml_node& node) override;
        std::string GetDisplayName() const noexcept override;
        std::string GetTypeName() const noexcept override;

        void RenderUI(int id) noexcept;
    private:
        bool m_dragging = false;
        glm::vec2 m_dragOffset{0.0f};
    };
}
