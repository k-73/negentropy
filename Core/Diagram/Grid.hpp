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
            
            RenderSmallGridLines(renderer, camera, screenSize, topLeft, bottomRight);
            RenderLargeGridLines(renderer, camera, screenSize, topLeft, bottomRight);
            RenderOriginCross(renderer, camera, screenSize);
        }

    private:
        void RenderSmallGridLines(SDL_Renderer* renderer, const Camera& camera, glm::vec2 screenSize, glm::vec2 topLeft, glm::vec2 bottomRight) const noexcept {
            const int firstGridX = static_cast<int>(std::floor(topLeft.x / settings.smallStep));
            const int lastGridX = static_cast<int>(std::ceil(bottomRight.x / settings.smallStep));
            const int firstGridY = static_cast<int>(std::floor(topLeft.y / settings.smallStep));
            const int lastGridY = static_cast<int>(std::ceil(bottomRight.y / settings.smallStep));
            
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            
            for (int i = firstGridX; i <= lastGridX; ++i) {
                const float worldX = static_cast<float>(i) * settings.smallStep;
                const glm::vec2 topPoint = camera.WorldToScreen({worldX, topLeft.y}, screenSize);
                const glm::vec2 bottomPoint = camera.WorldToScreen({worldX, bottomRight.y}, screenSize);
                SDL_RenderDrawLineF(renderer, topPoint.x, topPoint.y, bottomPoint.x, bottomPoint.y);
            }
            
            for (int i = firstGridY; i <= lastGridY; ++i) {
                const float worldY = static_cast<float>(i) * settings.smallStep;
                const glm::vec2 leftPoint = camera.WorldToScreen({topLeft.x, worldY}, screenSize);
                const glm::vec2 rightPoint = camera.WorldToScreen({bottomRight.x, worldY}, screenSize);
                SDL_RenderDrawLineF(renderer, leftPoint.x, leftPoint.y, rightPoint.x, rightPoint.y);
            }
        }

        void RenderLargeGridLines(SDL_Renderer* renderer, const Camera& camera, glm::vec2 screenSize, glm::vec2 topLeft, glm::vec2 bottomRight) const noexcept {
            const int firstGridX = static_cast<int>(std::floor(topLeft.x / settings.largeStep));
            const int lastGridX = static_cast<int>(std::ceil(bottomRight.x / settings.largeStep));
            const int firstGridY = static_cast<int>(std::floor(topLeft.y / settings.largeStep));
            const int lastGridY = static_cast<int>(std::ceil(bottomRight.y / settings.largeStep));
            
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            
            for (int i = firstGridX; i <= lastGridX; ++i) {
                const float worldX = static_cast<float>(i) * settings.largeStep;
                const glm::vec2 topPoint = camera.WorldToScreen({worldX, topLeft.y}, screenSize);
                const glm::vec2 bottomPoint = camera.WorldToScreen({worldX, bottomRight.y}, screenSize);
                SDL_RenderDrawLineF(renderer, topPoint.x, topPoint.y, bottomPoint.x, bottomPoint.y);
            }
            
            for (int i = firstGridY; i <= lastGridY; ++i) {
                const float worldY = static_cast<float>(i) * settings.largeStep;
                const glm::vec2 leftPoint = camera.WorldToScreen({topLeft.x, worldY}, screenSize);
                const glm::vec2 rightPoint = camera.WorldToScreen({bottomRight.x, worldY}, screenSize);
                SDL_RenderDrawLineF(renderer, leftPoint.x, leftPoint.y, rightPoint.x, rightPoint.y);
            }
        }

        static void RenderOriginCross(SDL_Renderer* renderer, const Camera& camera, glm::vec2 screenSize) noexcept {
            constexpr glm::vec2 gridCenterWorld{0.0f, 0.0f};
            constexpr glm::vec2 crossMinWorld{-5.0f, -5.0f};
            constexpr glm::vec2 crossMaxWorld{5.0f, 5.0f};
            
            const glm::vec2 centerScreen = camera.WorldToScreen(gridCenterWorld, screenSize);
            const glm::vec2 crossMinScreen = camera.WorldToScreen(crossMinWorld, screenSize);
            const glm::vec2 crossMaxScreen = camera.WorldToScreen(crossMaxWorld, screenSize);
            
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawLineF(renderer, crossMinScreen.x, centerScreen.y, crossMaxScreen.x, centerScreen.y);
            SDL_RenderDrawLineF(renderer, centerScreen.x, crossMinScreen.y, centerScreen.x, crossMaxScreen.y);
        }

    public:
        void xml_serialize(pugi::xml_node& node) const {
            settings.xml_serialize(node);
        }

        void xml_deserialize(const pugi::xml_node& node) {
            settings.xml_deserialize(node);
        }
    };
}