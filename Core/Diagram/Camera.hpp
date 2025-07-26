#pragma once

#include <glm/vec2.hpp>
#include <pugixml.hpp>
#include "../Utils/XMLSerialization.hpp"

namespace Diagram {
    struct CameraData {
        glm::vec2 position{0.0f};
        float zoom = 1.0f;
    };

    struct Camera {
        CameraData data;
        
        bool panning = false;
        glm::vec2 panStart{0.0f};
        glm::vec2 mouseStart{0.0f};

        [[nodiscard]] glm::vec2 ScreenToWorld(glm::vec2 screenPos) const {
            return (screenPos / data.zoom) + data.position;
        }

        [[nodiscard]] glm::vec2 WorldToScreen(glm::vec2 worldPos) const {
            return (worldPos - data.position) * data.zoom;
        }

        void ZoomAt(glm::vec2 screenPos, float factor) {
            glm::vec2 worldPosBefore = ScreenToWorld(screenPos);
            data.zoom *= factor;
            glm::vec2 worldPosAfter = ScreenToWorld(screenPos);
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
