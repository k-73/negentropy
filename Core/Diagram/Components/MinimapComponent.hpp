#pragma once

#include "Interface/Component.hpp"
#include "CameraComponent.hpp" // Needs to know about CameraComponent and CameraView

namespace Diagram {

    class MinimapComponent : public Component {
    public:
        // --- Visual Properties ---
        glm::vec4 backgroundColor{0.1f, 0.1f, 0.1f, 0.8f};
        glm::vec4 viewRectColor{1.0f, 0.0f, 0.0f, 0.7f};
        glm::vec4 borderColor{0.5f, 0.5f, 0.5f, 1.0f};

        // --- Target Components ---
        // The camera whose view we want to display on the minimap.
        const CameraComponent* targetCamera = nullptr;
        // The grid, to know the total world size.
        const Component* worldBounds = nullptr;

    public:
        MinimapComponent(const glm::vec2& pos, const glm::vec2& size, const CameraComponent* target, const Component* world) {
            this->position = pos;
            this->size = size;
            this->targetCamera = target;
            this->worldBounds = world;
        }

        void Render(SDL_Renderer* renderer, const CameraView& view, const glm::vec2& screenSize) const override {
            if (!targetCamera || !worldBounds) {
                return;
            }

            const SDL_FRect backgroundRect = { this->position.x, this->position.y, this->size.x, this->size.y };
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(backgroundColor.r * 255), 
                (Uint8)(backgroundColor.g * 255), 
                (Uint8)(backgroundColor.b * 255), 
                (Uint8)(backgroundColor.a * 255)
            );
            SDL_RenderFillRectF(renderer, &backgroundRect);

            // 2. Draw the border for the minimap
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(borderColor.r * 255), 
                (Uint8)(borderColor.g * 255), 
                (Uint8)(borderColor.b * 255), 
                (Uint8)(borderColor.a * 255)
            );
            SDL_RenderDrawRectF(renderer, &backgroundRect);

            // 3. Calculate and draw the camera's view rectangle
            
            // Get the total size of the world from the grid component
            const glm::vec2 totalWorldSize = worldBounds->size;
            if (totalWorldSize.x <= 0 || totalWorldSize.y <= 0) return;

            // Get the view of the camera we are tracking
            const CameraView targetView = targetCamera->GetView();
            
            // Determine the top-left of the camera's view in world coordinates
            const glm::vec2 cameraTopLeftWorld = targetView.ScreenToWorld({0,0}, screenSize);

            // Calculate the scale factor to map world coordinates to minimap coordinates
            const float scaleX = this->size.x / totalWorldSize.x;
            const float scaleY = this->size.y / totalWorldSize.y;

            // Calculate the position and size of the view rectangle on the minimap
            const SDL_FRect cameraViewRect = {
                backgroundRect.x + cameraTopLeftWorld.x * scaleX,
                backgroundRect.y + cameraTopLeftWorld.y * scaleY,
                screenSize.x / targetView.zoom * scaleX,
                screenSize.y / targetView.zoom * scaleY
            };

            // Draw the view rectangle
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(viewRectColor.r * 255), 
                (Uint8)(viewRectColor.g * 255), 
                (Uint8)(viewRectColor.b * 255), 
                (Uint8)(viewRectColor.a * 255)
            );
            SDL_RenderDrawRectF(renderer, &cameraViewRect);
        }

        std::string GetTypeName() const override { return "MinimapComponent"; }
    };

} // namespace Diagram
