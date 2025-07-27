#pragma once

#include <pugixml.hpp>
#include <cmath>
#include <glm/vec2.hpp>
#include "../Utils/XMLSerialization.hpp"
#include "Camera.hpp"

namespace Diagram {

    struct GridSettings {
        float smallStep = 5.0f;
        float largeStep = 50.0f;
        bool visible = false;

        void xml_serialize(pugi::xml_node& node) const {
            XML::auto_serialize(*this, node);
        }

        void xml_deserialize(const pugi::xml_node& node) {
            XML::auto_deserialize(*this, node);
        }
    };

    struct GridRenderParams {
        float baseOffsetX;
        float baseOffsetY;
        float smallScaled;
        float largeScaled;
        int largeStepMultiplier;
    };

    struct Grid {
        GridSettings settings;

        [[nodiscard]] GridRenderParams CalculateRenderParams(const Camera& camera, int screenWidth, int screenHeight) const noexcept {
            const glm::vec2 screenSize{static_cast<float>(screenWidth), static_cast<float>(screenHeight)};
            
            // Znajdź jaką część world space widzimy na ekranie
            const glm::vec2 topLeft = camera.ScreenToWorld({0.0f, 0.0f}, screenSize);
            const glm::vec2 bottomRight = camera.ScreenToWorld(screenSize, screenSize);
            
            // Znajdź pierwszą world grid linię widoczną na ekranie  
            const float firstWorldLineX = std::floor(topLeft.x / settings.smallStep) * settings.smallStep;
            const float firstWorldLineY = std::floor(topLeft.y / settings.smallStep) * settings.smallStep;
            
            // Konwertuj pierwszą world grid linię na screen position (jak blok!)
            const glm::vec2 firstLineScreen = camera.WorldToScreen({firstWorldLineX, firstWorldLineY}, screenSize);
            
            // Oblicz spacing na ekranie (skalowany przez zoom)
            const float smallScaled = settings.smallStep * camera.data.zoom;
            const int largeStepMultiplier = static_cast<int>(std::round(settings.largeStep / settings.smallStep));
            const float largeScaled = smallScaled * static_cast<float>(largeStepMultiplier);
            
            return {firstLineScreen.x, firstLineScreen.y, smallScaled, largeScaled, largeStepMultiplier};
        }

        void xml_serialize(pugi::xml_node& node) const {
            settings.xml_serialize(node);
        }

        void xml_deserialize(const pugi::xml_node& node) {
            settings.xml_deserialize(node);
        }
    };
}