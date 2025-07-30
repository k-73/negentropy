#pragma once

#include <SDL.h>

#include <algorithm>
#include <glm/vec2.hpp>
#include <ranges>

#include "../Diagram/Camera.hpp"
#include "../Diagram/Component.hpp"

class EventHandler
{
public:
	template<typename ComponentContainer>
	static void HandleEvent(const SDL_Event &event, Diagram::Camera &camera, ComponentContainer &componentList,
							glm::vec2 screenSize) noexcept {
		if(event.type == SDL_MOUSEWHEEL) {
			int mousePositionX, mousePositionY;
			SDL_GetMouseState(&mousePositionX, &mousePositionY);
			const glm::vec2 mousePosition {static_cast<float>(mousePositionX), static_cast<float>(mousePositionY)};
			camera.ZoomAt(mousePosition, screenSize, event.wheel.y > 0 ? 1.1f : 0.9f);
		} else if(event.type == SDL_MOUSEBUTTONDOWN) {
			if(event.button.button == SDL_BUTTON_MIDDLE) {
				camera.panning = true;
				camera.panStart = camera.data.position;
				camera.mouseStart = {static_cast<float>(event.button.x), static_cast<float>(event.button.y)};
			} else if(event.button.button == SDL_BUTTON_LEFT) {
				Diagram::ComponentBase::ClearSelection();
				for(auto &item: std::ranges::reverse_view(componentList)) {
					if(item->HandleEvent(event, camera, screenSize)) {
						Diagram::ComponentBase::Select(item.get());
						break;
					}
				}
			}
		} else if(event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_MIDDLE) {
			camera.panning = false;
		} else if(event.type == SDL_KEYDOWN) {
			if(event.key.keysym.sym == SDLK_DELETE) {
				if(auto *selected = Diagram::ComponentBase::GetSelected()) {
					if(auto it = std::ranges::find_if(componentList,
													  [selected](const auto &comp) {
														  return comp.get() == selected;
													  });
					   it != componentList.end()) {
						componentList.erase(it);
						Diagram::ComponentBase::ClearSelection();
					}
				}
			}
		} else if(event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEMOTION) {
			if(event.type == SDL_MOUSEMOTION && camera.panning) {
				const glm::vec2 mousePosition {static_cast<float>(event.motion.x), static_cast<float>(event.motion.y)};
				camera.data.position = camera.panStart - (mousePosition - camera.mouseStart) / camera.data.zoom;
			}
			for(const auto &item: componentList) {
				item->HandleEvent(event, camera, screenSize);
			}
		}
	}
};
