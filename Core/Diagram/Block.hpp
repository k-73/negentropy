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

    class Block final : public Component<Block> {
    public:
        enum class Type {
            Start,
            Process,
            Decision,
            End
        };

        struct Data {
            glm::vec2 position{0.0f};
            glm::vec2 size{10.0f, 5.0f};
            std::string label;
            Type type = Type::Process;

            glm::vec4 backgroundColor{0.8f, 0.33f, 0.08f, 0.7f};
            glm::vec4 borderColor{0.07f, 0.07f, 0.07f, 1.0f};
        } data;

        bool HandleEvent(const SDL_Event& event, const Camera& camera, glm::vec2 screenSize) noexcept override;
        void Render(SDL_Renderer* renderer, const Camera& camera, glm::vec2 screenSize) const noexcept override;
        void XmlSerialize(pugi::xml_node& node) const override;
        void XmlDeserialize(const pugi::xml_node& node) override;
        std::string GetDisplayName() const noexcept override;

        void RenderUI(int id) noexcept;

    private:
        bool m_dragging = false;
        glm::vec2 m_dragOffset{0.0f};
    };
}
