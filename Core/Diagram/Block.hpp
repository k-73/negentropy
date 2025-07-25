#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <magic_enum/magic_enum.hpp>

namespace Diagram {
    enum class BlockType {
        Start,
        Process,
        Decision,
        End
    };

    struct Block {
        glm::vec2 position{0.0f};
        glm::vec2 size{120.0f, 60.0f};
        std::string label;
        BlockType type = BlockType::Process;
        glm::vec4 color{0.35f, 0.47f, 0.78f, 1.0f};
        bool dragging = false;
        glm::vec2 dragOffset{0.0f};

        [[nodiscard]] glm::vec4 GetRect() const {
            return {position.x, position.y, size.x, size.y};
        }

        [[nodiscard]] bool Contains(glm::vec2 point) const {
            return point.x >= position.x && point.y >= position.y &&
                   point.x < position.x + size.x && point.y < position.y + size.y;
        }
    };
}
