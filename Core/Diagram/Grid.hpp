#pragma once

#include <pugixml.hpp>
#include <cmath>
#include <glm/vec2.hpp>
#include <SDL2/SDL.h>
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


    struct Grid {
        GridSettings settings;

        void Render(SDL_Renderer* renderer, const Camera& camera) const noexcept {
            if (!settings.visible) return;
            
            int w, h;
            SDL_GetRendererOutputSize(renderer, &w, &h);
            const glm::vec2 screenSize{static_cast<float>(w), static_cast<float>(h)};
            
            const glm::vec2 topLeft = camera.ScreenToWorld({0.0f, 0.0f}, screenSize);
            const glm::vec2 bottomRight = camera.ScreenToWorld(screenSize, screenSize);
            
            const float firstGridX = std::floor(topLeft.x / settings.smallStep) * settings.smallStep;
            const float lastGridX = std::ceil(bottomRight.x / settings.smallStep) * settings.smallStep;
            const float firstGridY = std::floor(topLeft.y / settings.smallStep) * settings.smallStep;
            const float lastGridY = std::ceil(bottomRight.y / settings.smallStep) * settings.smallStep;
            
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            
            for (float worldX = firstGridX; worldX <= lastGridX; worldX += settings.smallStep) {
                const glm::vec2 topPoint = camera.WorldToScreen({worldX, topLeft.y}, screenSize);
                const glm::vec2 bottomPoint = camera.WorldToScreen({worldX, bottomRight.y}, screenSize);
                SDL_RenderDrawLineF(renderer, topPoint.x, topPoint.y, bottomPoint.x, bottomPoint.y);
            }
            
            for (float worldY = firstGridY; worldY <= lastGridY; worldY += settings.smallStep) {
                const glm::vec2 leftPoint = camera.WorldToScreen({topLeft.x, worldY}, screenSize);
                const glm::vec2 rightPoint = camera.WorldToScreen({bottomRight.x, worldY}, screenSize);
                SDL_RenderDrawLineF(renderer, leftPoint.x, leftPoint.y, rightPoint.x, rightPoint.y);
            }
            
            const int largeStepMultiplier = static_cast<int>(std::round(settings.largeStep / settings.smallStep));
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
            
            for (float worldX = firstGridX; worldX <= lastGridX; worldX += settings.smallStep) {
                const int gridIndex = static_cast<int>(std::round(worldX / settings.smallStep));
                if (gridIndex % largeStepMultiplier == 0) {
                    const glm::vec2 topPoint = camera.WorldToScreen({worldX, topLeft.y}, screenSize);
                    const glm::vec2 bottomPoint = camera.WorldToScreen({worldX, bottomRight.y}, screenSize);
                    SDL_RenderDrawLineF(renderer, topPoint.x, topPoint.y, bottomPoint.x, bottomPoint.y);
                }
            }
            
            for (float worldY = firstGridY; worldY <= lastGridY; worldY += settings.smallStep) {
                const int gridIndex = static_cast<int>(std::round(worldY / settings.smallStep));
                if (gridIndex % largeStepMultiplier == 0) {
                    const glm::vec2 leftPoint = camera.WorldToScreen({topLeft.x, worldY}, screenSize);
                    const glm::vec2 rightPoint = camera.WorldToScreen({bottomRight.x, worldY}, screenSize);
                    SDL_RenderDrawLineF(renderer, leftPoint.x, leftPoint.y, rightPoint.x, rightPoint.y);
                }
            }
            
            const glm::vec2 gridCenterWorld{0.0f, 0.0f};
            const glm::vec2 crossMinWorld{-5.0f, -5.0f};
            const glm::vec2 crossMaxWorld{5.0f, 5.0f};
            
            const glm::vec2 centerScreen = camera.WorldToScreen(gridCenterWorld, screenSize);
            const glm::vec2 crossMinScreen = camera.WorldToScreen(crossMinWorld, screenSize);
            const glm::vec2 crossMaxScreen = camera.WorldToScreen(crossMaxWorld, screenSize);
            
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawLineF(renderer, crossMinScreen.x, centerScreen.y, crossMaxScreen.x, centerScreen.y);
            SDL_RenderDrawLineF(renderer, centerScreen.x, crossMinScreen.y, centerScreen.x, crossMaxScreen.y);
        }

        void xml_serialize(pugi::xml_node& node) const {
            settings.xml_serialize(node);
        }

        void xml_deserialize(const pugi::xml_node& node) {
            settings.xml_deserialize(node);
        }
    };
}