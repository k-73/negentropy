#include "Block.hpp"
#include "Camera.hpp"
#include "../Main/DiagramData.hpp"
#include <cstring>

#include <imgui.h>

namespace Diagram {
    bool Block::HandleEvent(const SDL_Event& event, const Camera& camera, glm::vec2 screenSize) noexcept {
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            const glm::vec2 worldPos = camera.ScreenToWorld({static_cast<float>(event.button.x), static_cast<float>(event.button.y)}, screenSize);
            const bool contains = (data.position.x <= worldPos.x && worldPos.x <= data.position.x + data.size.x &&
                                   data.position.y <= worldPos.y && worldPos.y <= data.position.y + data.size.y);
            if (contains) {
                m_dragging = true;
                m_dragOffset = worldPos - data.position;
                return true;
            }
        }
        else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
            if (m_dragging) {
                m_dragging = false;
                return true;
            }
        }
        else if (event.type == SDL_MOUSEMOTION && m_dragging) {
            const glm::vec2 worldPos = camera.ScreenToWorld({static_cast<float>(event.motion.x), static_cast<float>(event.motion.y)}, screenSize);
            glm::vec2 newPosition = worldPos - m_dragOffset;
            
            if (auto* diagramData = DiagramData::GetInstance()) {
                newPosition = diagramData->GetGrid().SnapToGrid(newPosition);
            }
            
            data.position = newPosition;
            return true;
        }
        return false;
    }

    void Block::Render(SDL_Renderer* renderer, const Camera& camera, glm::vec2 screenSize) const noexcept {
        const auto screenPos = camera.WorldToScreen(data.position, screenSize);
        const SDL_FRect rect = {screenPos.x, screenPos.y, data.size.x * camera.data.zoom, data.size.y * camera.data.zoom};

        const auto bgR = static_cast<Uint8>(data.backgroundColor.r * 255.0f);
        const auto bgG = static_cast<Uint8>(data.backgroundColor.g * 255.0f);
        const auto bgB = static_cast<Uint8>(data.backgroundColor.b * 255.0f);
        const auto bgA = static_cast<Uint8>(data.backgroundColor.a * 255.0f);

        SDL_SetRenderDrawColor(renderer, bgR, bgG, bgB, bgA);
        SDL_RenderFillRectF(renderer, &rect);

        const auto borderR = static_cast<Uint8>(data.borderColor.r * 255.0f);
        const auto borderG = static_cast<Uint8>(data.borderColor.g * 255.0f);
        const auto borderB = static_cast<Uint8>(data.borderColor.b * 255.0f);
        const auto borderA = static_cast<Uint8>(data.borderColor.a * 255.0f);
        
        SDL_SetRenderDrawColor(renderer, borderR, borderG, borderB, borderA);
        SDL_RenderDrawRectF(renderer, &rect);
    }

    void Block::xml_serialize(pugi::xml_node& node) const {
        serialize_base(node);
        XML::auto_serialize(data, node);
    }

    void Block::xml_deserialize(const pugi::xml_node& node) {
        deserialize_base(node);
        XML::auto_deserialize(data, node);
    }

    std::string Block::GetDisplayName() const noexcept {
        return data.label.empty() ? "Block" : data.label;
    }


    void Block::RenderUI(const int id) noexcept {
        ImGui::PushID(id);
        
        char labelBuffer[256];
        std::strncpy(labelBuffer, data.label.c_str(), sizeof(labelBuffer) - 1);
        labelBuffer[sizeof(labelBuffer) - 1] = '\0';
        if (ImGui::InputText("Label", labelBuffer, sizeof(labelBuffer))) {
            data.label = labelBuffer;
        }
        
        ImGui::DragFloat2("Position", &data.position.x, 1.0f);
        ImGui::DragFloat2("Size", &data.size.x, 1.0f, 10.0f, 500.0f);
        ImGui::ColorEdit4("Background", &data.backgroundColor.x);
        ImGui::ColorEdit4("Border", &data.borderColor.x);
        
        const char* typeNames[] = {"Start", "Process", "Decision", "End"};
        int currentType = static_cast<int>(data.type);
        if (ImGui::Combo("Type", &currentType, typeNames, 4)) {
            data.type = static_cast<Type>(currentType);
        }
        
        ImGui::PopID();
    }
}