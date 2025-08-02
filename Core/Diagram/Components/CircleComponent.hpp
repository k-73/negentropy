#pragma once

#include "Interface/Component.hpp"
#include <SDL.h>
#include <cmath>

namespace Diagram
{
    class CircleComponent : public Component
    {
    public:
        struct Data {
            float radius = 25.0f;
            glm::vec4 fillColor{0.2f, 0.6f, 0.8f, 0.8f};
            glm::vec4 borderColor{0.1f, 0.1f, 0.1f, 1.0f};
            float borderWidth = 2.0f;
        } data;

    public:
        CircleComponent(float radius = 25.0f, const glm::vec2& pos = {0.0f, 0.0f}) {
            this->data.radius = radius;
            this->position = pos;
            this->size = {radius * 2.0f, radius * 2.0f};
        }

        void Render(SDL_Renderer* renderer, const CameraView& view, const glm::vec2& screenSize) const override {
            if(!isVisible) return;

            const glm::vec2& worldPos = GetWorldPosition();
            const glm::vec2 screenPos = view.WorldToScreen(worldPos, screenSize);
            const float screenRadius = data.radius * view.zoom;

            // Draw filled circle using multiple lines (SDL doesn't have native circle drawing)
            const int segments = 32;
            const float centerX = screenPos.x + screenRadius;
            const float centerY = screenPos.y + screenRadius;

            // Fill circle
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(data.fillColor.r * 255),
                (Uint8)(data.fillColor.g * 255),
                (Uint8)(data.fillColor.b * 255),
                (Uint8)(data.fillColor.a * 255));

            // Draw filled circle by drawing horizontal lines
            for(int y = -screenRadius; y <= screenRadius; ++y) {
                float x = std::sqrt(screenRadius * screenRadius - y * y);
                if(x > 0) {
                    SDL_RenderDrawLineF(renderer, 
                        centerX - x, centerY + y, 
                        centerX + x, centerY + y);
                }
            }

            // Draw border
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(data.borderColor.r * 255),
                (Uint8)(data.borderColor.g * 255),
                (Uint8)(data.borderColor.b * 255),
                (Uint8)(data.borderColor.a * 255));

            // Draw circle outline
            for(int i = 0; i < segments; ++i) {
                float angle1 = (float)i * 2.0f * M_PI / segments;
                float angle2 = (float)(i + 1) * 2.0f * M_PI / segments;
                
                float x1 = centerX + screenRadius * std::cos(angle1);
                float y1 = centerY + screenRadius * std::sin(angle1);
                float x2 = centerX + screenRadius * std::cos(angle2);
                float y2 = centerY + screenRadius * std::sin(angle2);
                
                SDL_RenderDrawLineF(renderer, x1, y1, x2, y2);
            }
        }

        void RenderUI() override {
            ImGui::Text("Circle Component");
            ImGui::Separator();
            
            ImGui::DragFloat("Radius", &data.radius, 1.0f, 5.0f, 100.0f);
            if(ImGui::IsItemDeactivatedAfterEdit()) {
                this->size = {data.radius * 2.0f, data.radius * 2.0f};
            }
            
            ImGui::ColorEdit4("Fill Color", &data.fillColor.x);
            ImGui::ColorEdit4("Border Color", &data.borderColor.x);
            ImGui::DragFloat("Border Width", &data.borderWidth, 0.1f, 0.0f, 10.0f);
        }

        std::string GetTypeName() const override { return "CircleComponent"; }

        void XmlSerialize(pugi::xml_node& node) const override {
            XML::auto_serialize(data, node);
        }

        void XmlDeserialize(const pugi::xml_node& node) override {
            XML::auto_deserialize(data, node);
            this->size = {data.radius * 2.0f, data.radius * 2.0f};
        }
    };
}
