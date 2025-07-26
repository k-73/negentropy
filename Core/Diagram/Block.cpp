#include "Block.hpp"
#include "Camera.hpp"
#include <imgui.h>
#include <iostream>
#include <cstring>

namespace Diagram {
    bool Block::HandleEvent(const SDL_Event& event, const Camera& camera) noexcept {
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            const glm::vec2 worldPos = camera.ScreenToWorld({static_cast<float>(event.button.x), static_cast<float>(event.button.y)});
            if (Contains(worldPos)) {
                OnMouseDown(worldPos);
                return true;
            }
        }
        else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
            if (m_dragging) {
                OnMouseUp();
                return true;
            }
        }
        else if (event.type == SDL_MOUSEMOTION && m_dragging) {
            const glm::vec2 worldPos = camera.ScreenToWorld({static_cast<float>(event.motion.x), static_cast<float>(event.motion.y)});
            OnMouseMove(worldPos);
            return true;
        }
        return false;
    }

    void Block::Render(SDL_Renderer* renderer, const Camera& camera) const noexcept {
        const auto screenPos = camera.WorldToScreen(data.position);
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

    bool Block::Contains(glm::vec2 point) const noexcept {
        return point.x >= data.position.x && point.y >= data.position.y &&
               point.x < data.position.x + data.size.x && point.y < data.position.y + data.size.y;
    }

    void Block::xml_serialize(pugi::xml_node& node) const {
        XML::auto_serialize(data, node);
    }

    void Block::xml_deserialize(const pugi::xml_node& node) {
        XML::auto_deserialize(data, node);
    }

    void Block::OnMouseDown(glm::vec2 worldPos) noexcept {
        m_dragging = true;
        m_dragOffset = worldPos - data.position;
    }

    void Block::OnMouseUp() noexcept {
        m_dragging = false;
    }

    void Block::OnMouseMove(glm::vec2 worldPos) noexcept {
        if (m_dragging) {
            data.position = worldPos - m_dragOffset;
        }
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