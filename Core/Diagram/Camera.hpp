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

        glm::vec2 ScreenToWorld(const glm::vec2 screenPos, const glm::vec2 screenSize) const {
            const glm::vec2 center = screenSize * 0.5f;
            return ((screenPos - center) / data.zoom) + data.position;
        }

        glm::vec2 WorldToScreen(const glm::vec2 worldPos, const glm::vec2 screenSize) const {
            const glm::vec2 center = screenSize * 0.5f;
            return ((worldPos - data.position) * data.zoom) + center;
        }

        void ZoomAt(const glm::vec2 screenPos, const glm::vec2 screenSize, const float factor) {
            const glm::vec2 worldPosBefore = ScreenToWorld(screenPos, screenSize);
            data.zoom *= factor;
            const glm::vec2 worldPosAfter = ScreenToWorld(screenPos, screenSize);
            data.position += worldPosBefore - worldPosAfter;
        }

        void XmlSerialize(pugi::xml_node& node) const {
            XML::auto_serialize(data, node);
        }

        void XmlDeserialize(const pugi::xml_node& node) {
            XML::auto_deserialize(data, node);
        }
    };
}
