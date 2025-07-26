#pragma once

#include <pugixml.hpp>
#include <cmath>
#include "../Utils/XMLSerialization.hpp"

namespace Diagram {
    struct Camera;

    struct GridSettings {
        float smallStep = 10.0f;
        float largeStep = 50.0f;
        bool visible = true;

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
            const float smallScaled = settings.smallStep * camera.data.zoom;
            const float largeScaled = settings.largeStep * camera.data.zoom;
            
            const float centerX = static_cast<float>(screenWidth) * 0.5f;
            const float centerY = static_cast<float>(screenHeight) * 0.5f;
            
            const float worldOriginX = -camera.data.position.x * camera.data.zoom + centerX;
            const float worldOriginY = -camera.data.position.y * camera.data.zoom + centerY;
            
            const float offsetX = std::fmod(worldOriginX, smallScaled);
            const float offsetY = std::fmod(worldOriginY, smallScaled);
            
            const float baseOffsetX = offsetX < 0 ? offsetX + smallScaled : offsetX;
            const float baseOffsetY = offsetY < 0 ? offsetY + smallScaled : offsetY;
            
            const int largeStepMultiplier = static_cast<int>(std::round(settings.largeStep / settings.smallStep));
            
            return {baseOffsetX, baseOffsetY, smallScaled, largeScaled, largeStepMultiplier};
        }

        void xml_serialize(pugi::xml_node& node) const {
            settings.xml_serialize(node);
        }

        void xml_deserialize(const pugi::xml_node& node) {
            settings.xml_deserialize(node);
        }
    };
}