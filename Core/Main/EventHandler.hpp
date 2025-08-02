#pragma once

#include <SDL.h>

#include <algorithm>
#include <glm/vec2.hpp>
#include <ranges>

#include "../Diagram/Components/CameraComponent.hpp"
#include "../Diagram/Components/Interface/Component.hpp"

class EventHandler
{
public:
	// TODO: Update event handling for new component architecture
	// This is temporarily disabled while updating to new component system
	template<typename ComponentContainer>
	static void HandleEvent(const SDL_Event &event, Diagram::CameraComponent &camera, ComponentContainer &componentList,
							glm::vec2 screenSize) noexcept {
		// Basic camera event handling only for now
		if(event.type == SDL_MOUSEWHEEL) {
			int mousePositionX, mousePositionY;
			SDL_GetMouseState(&mousePositionX, &mousePositionY);
			const glm::vec2 mousePosition {static_cast<float>(mousePositionX), static_cast<float>(mousePositionY)};
			camera.ZoomAt(mousePosition, screenSize, event.wheel.y > 0 ? 1.1f : 0.9f);
		}
		// Component-specific event handling will be implemented later
	}
};
