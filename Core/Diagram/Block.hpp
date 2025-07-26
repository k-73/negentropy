#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <pugixml.hpp>
#include "../Utils/XMLSerialization.hpp"

namespace Diagram {
    struct Block {
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
            glm::vec4 color{0.35f, 0.47f, 0.78f, 1.0f};
        } data;

        bool dragging = false;
        glm::vec2 dragOffset{0.0f};

        [[nodiscard]] constexpr glm::vec4 GetRect() const noexcept {
            return {data.position.x, data.position.y, data.size.x, data.size.y};
        }

        [[nodiscard]] constexpr bool Contains(glm::vec2 point) const noexcept {
            return point.x >= data.position.x && point.y >= data.position.y &&
                   point.x < data.position.x + data.size.x && point.y < data.position.y + data.size.y;
        }

        void xml_serialize(pugi::xml_node& node) const {
            XML::auto_serialize(data, node);
        }

        void xml_deserialize(const pugi::xml_node& node) {
            XML::auto_deserialize(data, node);
        }
    };
}
