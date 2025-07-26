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
            
            auto posNode = node.append_child("position");
            XML::serialize(posNode, "x", position.x);
            XML::serialize(posNode, "y", position.y);
            
            auto sizeNode = node.append_child("size");
            XML::serialize(sizeNode, "x", size.x);
            XML::serialize(sizeNode, "y", size.y);
            
            XML_FIELD(node, label);
            
            auto colorNode = node.append_child("color");
            XML::serialize(colorNode, "r", color.r);
            XML::serialize(colorNode, "g", color.g);
            XML::serialize(colorNode, "b", color.b);
            XML::serialize(colorNode, "a", color.a);
        }

        void xml_deserialize(const pugi::xml_node& node) {
            XML_FIELD_LOAD(node, type);
            
            // Test basic loading first
            if (auto posNode = node.child("position")) {
                XML::deserialize(posNode, "x", position.x);
                XML::deserialize(posNode, "y", position.y);
            }
            if (auto sizeNode = node.child("size")) {
                XML::deserialize(sizeNode, "x", size.x);
                XML::deserialize(sizeNode, "y", size.y);
            }
            
            XML_FIELD_LOAD(node, label);
            
            if (auto colorNode = node.child("color")) {
                XML::deserialize(colorNode, "r", color.r);
                XML::deserialize(colorNode, "g", color.g);
                XML::deserialize(colorNode, "b", color.b);
                XML::deserialize(colorNode, "a", color.a);
            }
        }
    };
}
