#pragma once

#include <SDL.h>
#include <glm/vec2.hpp>
#include "../Diagram/Component.hpp"
#include "../Diagram/Camera.hpp"

class EventHandler {
public:
    template<typename ComponentContainer>
    static void HandleEvent(const SDL_Event &e, Diagram::Camera &camera, ComponentContainer &components,
                            glm::vec2 screenSize) noexcept {
        auto get = [](auto &x) {
            if constexpr (requires { x.get(); }) return x.get();
            else return &x;
        };

        if (e.type == SDL_MOUSEWHEEL) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            const glm::vec2 mousePos{static_cast<float>(mouseX), static_cast<float>(mouseY)};
            camera.ZoomAt(mousePos, screenSize, e.wheel.y > 0 ? 1.1f : 0.9f);
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (e.button.button == SDL_BUTTON_MIDDLE) {
                camera.panning = true;
                camera.panStart = camera.data.position;
                camera.mouseStart = {static_cast<float>(e.button.x), static_cast<float>(e.button.y)};
            } else if (e.button.button == SDL_BUTTON_LEFT) {
                Diagram::Component::ClearSelection();

                for (auto it = components.rbegin(); it != components.rend(); ++it) {
                    auto *comp = get(*it);
                    if (comp->HandleEvent(e, camera, screenSize)) {
                        Diagram::Component::Select(comp);
                        break;
                    }
                }
            }
        } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_MIDDLE) {
            camera.panning = false;
        } else if (e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION) {
            if (e.type == SDL_MOUSEMOTION && camera.panning) {
                const glm::vec2 mousePos{static_cast<float>(e.motion.x), static_cast<float>(e.motion.y)};
                camera.data.position = camera.panStart - (mousePos - camera.mouseStart) / camera.data.zoom;
            }

            for (const auto &item: components) {
                get(item)->HandleEvent(e, camera, screenSize);
            }
        }
    }
};
