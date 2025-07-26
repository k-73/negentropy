#pragma once

#include <glm/vec2.hpp>
#include <algorithm>
#include <pugixml.hpp>
#include "../Utils/XMLSerialization.hpp"

namespace Diagram {
    struct Camera : public XML::Serializable<Camera> {
        glm::vec2 position{0.0f};
        float zoom = 1.0f;
        
        // Runtime state (not serialized)
        bool panning = false;
        glm::vec2 panStart{0.0f};
        glm::vec2 mouseStart{0.0f};

        static constexpr float MIN_ZOOM = 0.1f;
        static constexpr float MAX_ZOOM = 5.0f;

        [[nodiscard]] constexpr glm::vec2 WorldToScreen(glm::vec2 worldPos) const noexcept {
            return (worldPos - position) * zoom;
        }

        [[nodiscard]] constexpr glm::vec2 ScreenToWorld(glm::vec2 screenPos) const noexcept {
            return screenPos / zoom + position;
        }

        void SetZoom(float newZoom) noexcept {
            zoom = std::clamp(newZoom, MIN_ZOOM, MAX_ZOOM);
        }

        void ZoomAt(glm::vec2 screenPos, float factor) noexcept {
            const glm::vec2 worldPosBeforeZoom = ScreenToWorld(screenPos);
            SetZoom(zoom * factor);
            const glm::vec2 worldPosAfterZoom = ScreenToWorld(screenPos);
            position += worldPosBeforeZoom - worldPosAfterZoom;
        }

        void xml_serialize(pugi::xml_node& node) const {
            XML_FIELD(node, position);
            XML_FIELD(node, zoom);
        }

        void xml_deserialize(const pugi::xml_node& node) {
            XML_FIELD_LOAD(node, position);
            XML_FIELD_LOAD(node, zoom);
        }
    };
}
