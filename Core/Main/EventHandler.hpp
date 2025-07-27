#pragma once

#include <SDL.h>
#include <vector>
#include <glm/vec2.hpp>
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"

class EventHandler {
public:
    static void HandleEvent(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks, glm::vec2 screenSize) noexcept {
        if (e.type == SDL_MOUSEWHEEL) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            const float zoom = e.wheel.y > 0 ? 1.1f : 0.9f;
            camera.ZoomAt({static_cast<float>(mouseX), static_cast<float>(mouseY)}, screenSize, zoom);
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (e.button.button == SDL_BUTTON_MIDDLE) {
                camera.panning = true;
                camera.panStart = camera.data.position;
                camera.mouseStart = {static_cast<float>(e.button.x), static_cast<float>(e.button.y)};
            }
            else if (e.button.button == SDL_BUTTON_LEFT) {
                Diagram::Component::ClearSelection();
                for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
                    if (it->HandleEvent(e, camera, screenSize)) {
                        Diagram::Component::Select(&(*it));
                        break;
                    }
                }
            }
        }
        else if (e.type == SDL_MOUSEBUTTONUP) {
            if (e.button.button == SDL_BUTTON_MIDDLE) {
                camera.panning = false;
            }
            else if (e.button.button == SDL_BUTTON_LEFT) {
                for (auto& block : blocks) block.HandleEvent(e, camera, screenSize);
            }
        }
        else if (e.type == SDL_MOUSEMOTION) {
            if (camera.panning) {
                const glm::vec2 mouse{static_cast<float>(e.motion.x), static_cast<float>(e.motion.y)};
                camera.data.position = camera.panStart - (mouse - camera.mouseStart) / camera.data.zoom;
            }
            for (auto& block : blocks) block.HandleEvent(e, camera, screenSize);
        }
    }
};
