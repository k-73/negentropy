#pragma once

#include "Interface/Component.hpp"
#include <SDL.h>

namespace Diagram
{
    class RectangleComponent : public Component
    {
    public:
        struct Data {
            glm::vec4 fillColor{0.6f, 0.8f, 0.2f, 0.8f};
            glm::vec4 borderColor{0.1f, 0.1f, 0.1f, 1.0f};
            float borderWidth = 2.0f;
            float cornerRadius = 0.0f;  // For future rounded corners
        } data;

    public:
        RectangleComponent(const glm::vec2& size = {60.0f, 40.0f}, const glm::vec2& pos = {0.0f, 0.0f}) {
            this->position = pos;
            this->size = size;
        }

        void Render(SDL_Renderer* renderer, const CameraView& view, const glm::vec2& screenSize) const override {
            if(!isVisible) return;

            const glm::vec2& worldPos = GetWorldPosition();
            const glm::vec2 screenPos = view.WorldToScreen(worldPos, screenSize);
            const glm::vec2 screenSize_scaled = size * view.zoom;

            const SDL_FRect rect = {
                screenPos.x, 
                screenPos.y, 
                screenSize_scaled.x, 
                screenSize_scaled.y
            };

            // Fill rectangle
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(data.fillColor.r * 255),
                (Uint8)(data.fillColor.g * 255),
                (Uint8)(data.fillColor.b * 255),
                (Uint8)(data.fillColor.a * 255));
            SDL_RenderFillRectF(renderer, &rect);

            // Draw border
            SDL_SetRenderDrawColor(renderer, 
                (Uint8)(data.borderColor.r * 255),
                (Uint8)(data.borderColor.g * 255),
                (Uint8)(data.borderColor.b * 255),
                (Uint8)(data.borderColor.a * 255));
            
            // Draw border with specified width
            for(int i = 0; i < (int)data.borderWidth; ++i) {
                SDL_FRect borderRect = {
                    rect.x - i, 
                    rect.y - i, 
                    rect.w + 2 * i, 
                    rect.h + 2 * i
                };
                SDL_RenderDrawRectF(renderer, &borderRect);
            }
        }

        void RenderUI() override {
            ImGui::Text("Rectangle Component");
            ImGui::Separator();
            
            ImGui::DragFloat2("Size", &size.x, 1.0f, 10.0f, 200.0f);
            ImGui::ColorEdit4("Fill Color", &data.fillColor.x);
            ImGui::ColorEdit4("Border Color", &data.borderColor.x);
            ImGui::DragFloat("Border Width", &data.borderWidth, 0.1f, 0.0f, 10.0f);
            ImGui::DragFloat("Corner Radius", &data.cornerRadius, 0.1f, 0.0f, 20.0f);
            if(ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Corner radius (not yet implemented)");
            }
        }

        std::string GetTypeName() const override { return "RectangleComponent"; }

        void XmlSerialize(pugi::xml_node& node) const override {
            XML::auto_serialize(data, node);
        }

        void XmlDeserialize(const pugi::xml_node& node) override {
            XML::auto_deserialize(data, node);
        }
    };
}
