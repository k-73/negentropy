#pragma once

#include <glm/vec2.hpp>

namespace Diagram {
    struct Camera {
        glm::vec2 position{0.0f};
        bool panning = false;
        glm::vec2 panStart{0.0f};
        glm::vec2 mouseStart{0.0f};

        [[nodiscard]] constexpr glm::vec2 WorldToScreen(glm::vec2 worldPos) const noexcept {
            return worldPos - position;
        }

        [[nodiscard]] constexpr glm::vec2 ScreenToWorld(glm::vec2 screenPos) const noexcept {
            return screenPos + position;
        }
    };
}
