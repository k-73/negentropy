#pragma once

#include <SDL.h>
#include <vector>
#include <memory>
#include <type_traits>
#include <glm/vec2.hpp>
#include "../Diagram/Component.hpp"
#include "../Diagram/Camera.hpp"

class EventHandler {
public:
    template<typename ComponentContainer>
    static void HandleEvent(const SDL_Event& e, Diagram::Camera& camera, ComponentContainer& components, glm::vec2 screenSize) noexcept {
        HandleEventImpl(e, camera, components, screenSize);
    }

private:
    template<typename ComponentContainer>
    static void HandleEventImpl(const SDL_Event& e, Diagram::Camera& camera, const ComponentContainer& components, glm::vec2 screenSize) noexcept {
        auto getComponent = []<typename T0>(T0& item) -> Diagram::Component* {
            if constexpr (std::is_pointer_v<std::decay_t<T0>>) return item;
            else if constexpr (std::is_same_v<std::decay_t<T0>, std::unique_ptr<Diagram::Component>>) return item.get();
            else return const_cast<Diagram::Component*>(&item);
        };

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
                for (auto it = components.rbegin(); it != components.rend(); ++it) {
                    auto* component = getComponent(*it);
                    if (component->HandleEvent(e, camera, screenSize)) {
                        Diagram::Component::Select(component);
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
                for (const auto& item : components) getComponent(item)->HandleEvent(e, camera, screenSize);
            }
        }
        else if (e.type == SDL_MOUSEMOTION) {
            if (camera.panning) {
                const glm::vec2 mouse{static_cast<float>(e.motion.x), static_cast<float>(e.motion.y)};
                camera.data.position = camera.panStart - (mouse - camera.mouseStart) / camera.data.zoom;
            }
            for (const auto& item : components) getComponent(item)->HandleEvent(e, camera, screenSize);
        }
    }
};
