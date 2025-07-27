#pragma once

#include <SDL.h>
#include <memory>
#include <type_traits>
#include <glm/vec2.hpp>
#include "../Diagram/Component.hpp"
#include "../Diagram/Camera.hpp"

class EventHandler {
public:
    template<typename ComponentContainer>
    static void HandleEvent(const SDL_Event& e, Diagram::Camera& camera, ComponentContainer& components, glm::vec2 screenSize) noexcept {
        auto get = []<typename T>(T& x) -> Diagram::Component* { 
            if constexpr (std::is_pointer_v<std::decay_t<T>>) return x;
            else if constexpr (std::is_same_v<std::decay_t<T>, std::unique_ptr<Diagram::Component>>) return x.get();
            else return const_cast<Diagram::Component*>(&x);
        };

        if (e.type == SDL_MOUSEWHEEL) {
            int mouseX, mouseY; SDL_GetMouseState(&mouseX, &mouseY);
            camera.ZoomAt({float(mouseX), float(mouseY)}, screenSize, e.wheel.y > 0 ? 1.1f : 0.9f);
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (e.button.button == SDL_BUTTON_MIDDLE) {
                camera.panning = true; camera.panStart = camera.data.position;
                camera.mouseStart = {float(e.button.x), float(e.button.y)};
            } else if (e.button.button == SDL_BUTTON_LEFT) {
                Diagram::Component::ClearSelection();
                for (auto it = components.rbegin(); it != components.rend(); ++it)
                    if (get(*it)->HandleEvent(e, camera, screenSize)) { Diagram::Component::Select(get(*it)); break; }
            }
        } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_MIDDLE) {
            camera.panning = false;
        } else if (e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEMOTION) {
            if (e.type == SDL_MOUSEMOTION && camera.panning)
                camera.data.position = camera.panStart - (glm::vec2{float(e.motion.x), float(e.motion.y)} - camera.mouseStart) / camera.data.zoom;
            for (const auto& item : components) get(item)->HandleEvent(e, camera, screenSize);
        }
    }
};
