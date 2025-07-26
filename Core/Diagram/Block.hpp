#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <pugixml.hpp>
#include "../Utils/XMLSerialization.hpp"

namespace Diagram {
    enum class BlockType {
        Start,
        Process,
        Decision,
        End
    };

    struct Block : public XML::Serializable<Block> {
        glm::vec2 position{0.0f};
        glm::vec2 size{120.0f, 60.0f};
        std::string label;
        BlockType type = BlockType::Process;
        glm::vec4 color{0.35f, 0.47f, 0.78f, 1.0f};
        
        // Runtime state (not serialized)
        bool dragging = false;
        glm::vec2 dragOffset{0.0f};

        [[nodiscard]] constexpr glm::vec4 GetRect() const noexcept {
            return {position.x, position.y, size.x, size.y};
        }

        [[nodiscard]] constexpr bool Contains(glm::vec2 point) const noexcept {
            return point.x >= position.x && point.y >= position.y &&
                   point.x < position.x + size.x && point.y < position.y + size.y;
        }

        void xml_serialize(pugi::xml_node& node) const {
            XML_FIELD(node, type);
            XML_NODE(node, "position", XML::f("x", position.x), XML::f("y", position.y));
            XML_NODE(node, "size", XML::f("x", size.x), XML::f("y", size.y));
            XML_FIELD(node, label);
            XML_NODE(node, "color", XML::f("r", color.r), XML::f("g", color.g), 
                                    XML::f("b", color.b), XML::f("a", color.a));
        }

        void xml_deserialize(const pugi::xml_node& node) {
            XML_FIELD_LOAD(node, type);
            XML_NODE_LOAD(node, "position", XML::f("x", position.x), XML::f("y", position.y));
            XML_NODE_LOAD(node, "size", XML::f("x", size.x), XML::f("y", size.y));
            XML_FIELD_LOAD(node, label);
            XML_NODE_LOAD(node, "color", XML::f("r", color.r), XML::f("g", color.g), 
                                          XML::f("b", color.b), XML::f("a", color.a));
        }
    };
}
