#pragma once

#include <glm/vec2.hpp>
#include <pugixml.hpp>
#include "../Utils/XMLSerialization.hpp"

namespace Diagram {
    struct Camera {
        struct Data {
            glm::vec2 position{0.0f};
            float zoom = 1.0f;
        } data;
        
        bool panning = false;
        glm::vec2 panStart{0.0f};
        glm::vec2 mouseStart{0.0f};

        [[nodiscard]] glm::vec2 ScreenToWorld(glm::vec2 screenPos, glm::vec2 screenSize) const {
            const glm::vec2 center = screenSize * 0.5f;
            return ((screenPos - center) / data.zoom) + data.position;
        }

        [[nodiscard]] glm::vec2 WorldToScreen(glm::vec2 worldPos, glm::vec2 screenSize) const {
            const glm::vec2 center = screenSize * 0.5f;
            return ((worldPos - data.position) * data.zoom) + center;
        }

        void ZoomAt(glm::vec2 screenPos, glm::vec2 screenSize, float factor) {
            glm::vec2 worldPosBefore = ScreenToWorld(screenPos, screenSize);
            data.zoom *= factor;
            glm::vec2 worldPosAfter = ScreenToWorld(screenPos, screenSize);
            data.position += worldPosBefore - worldPosAfter;
        }

        void xml_serialize(pugi::xml_node& node) const {
            XML::auto_serialize(data, node);
        }

        void xml_deserialize(const pugi::xml_node& node) {
            XML::auto_deserialize(data, node);
        }
    };
}
