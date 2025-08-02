#pragma once

#include "Interface/Component.hpp"
#include <SDL.h>

namespace Diagram
{
    class TriangleComponent : public Component
    {
    public:
        struct Data {
            glm::vec4 fillColor{0.8f, 0.2f, 0.6f, 0.8f};
            glm::vec4 borderColor{0.1f, 0.1f, 0.1f, 1.0f};
            float borderWidth = 2.0f;
        } data;

    public:
        TriangleComponent(const glm::vec2& size = {50.0f, 50.0f}, const glm::vec2& pos = {0.0f, 0.0f}) {
            this->position = pos;
            this->size = size;
        }

        void Render(SDL_Renderer* renderer, const CameraView& view, const glm::vec2& screenSize) const override {
            if(!isVisible) return;

            const glm::vec2& worldPos = GetWorldPosition();
            const glm::vec2 screenPos = view.WorldToScreen(worldPos, screenSize);
            const glm::vec2 screenSize_scaled = size * view.zoom;

            // Triangle vertices (pointing upward)
            const float x1 = screenPos.x + screenSize_scaled.x * 0.5f;  // Top center
            const float y1 = screenPos.y;
            const float x2 = screenPos.x;  // Bottom left
            const float y2 = screenPos.y + screenSize_scaled.y;
            const float x3 = screenPos.x + screenSize_scaled.x;  // Bottom right
            const float y3 = screenPos.y + screenSize_scaled.y;

            // Fill triangle by drawing horizontal lines
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(data.fillColor.r * 255),
                (Uint8)(data.fillColor.g * 255),
                (Uint8)(data.fillColor.b * 255),
                (Uint8)(data.fillColor.a * 255));

            // Fill triangle using scanline method
            for(float y = y1; y <= y2; ++y) {
                float progress = (y - y1) / (y2 - y1);
                float leftX = x1 + (x2 - x1) * progress;
                float rightX = x1 + (x3 - x1) * progress;
                
                if(leftX <= rightX) {
                    SDL_RenderDrawLineF(renderer, leftX, y, rightX, y);
                }
            }

            // Draw border
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(data.borderColor.r * 255),
                (Uint8)(data.borderColor.g * 255),
                (Uint8)(data.borderColor.b * 255),
                (Uint8)(data.borderColor.a * 255));

            // Draw triangle outline
            SDL_RenderDrawLineF(renderer, x1, y1, x2, y2);  // Top to bottom-left
            SDL_RenderDrawLineF(renderer, x2, y2, x3, y3);  // Bottom-left to bottom-right
            SDL_RenderDrawLineF(renderer, x3, y3, x1, y1);  // Bottom-right to top
        }

        void RenderUI() override {
            ImGui::Text("Triangle Component");
            ImGui::Separator();
            
            ImGui::DragFloat2("Size", &size.x, 1.0f, 10.0f, 200.0f);
            ImGui::ColorEdit4("Fill Color", &data.fillColor.x);
            ImGui::ColorEdit4("Border Color", &data.borderColor.x);
            ImGui::DragFloat("Border Width", &data.borderWidth, 0.1f, 0.0f, 10.0f);
        }

        std::string GetTypeName() const override { return "TriangleComponent"; }

        void XmlSerialize(pugi::xml_node& node) const override {
            XML::auto_serialize(data, node);
        }

        void XmlDeserialize(const pugi::xml_node& node) override {
            XML::auto_deserialize(data, node);
        }
    };
}
