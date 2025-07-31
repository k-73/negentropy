#pragma once

#include <string>

#include "Diagram/Camera.hpp"
#include "Diagram/Components/Interface/Component.hpp"
#include "imgui.h"

namespace Diagram
{
	struct Camera;

	class GridComponent : public Component, public EventHandler
	{
	public:
		struct GridSettings {
			float smallStep = 5.0f;
			float largeStep = 50.0f;
			bool visible = true;

			void XmlSerialize(pugi::xml_node& node) const {
				XML::auto_serialize(*this, node);
			}

			void XmlDeserialize(const pugi::xml_node& node) {
				XML::auto_deserialize(*this, node);
			}
		} settings;
		glm::vec4 backgroundColor {0.15f, 0.15f, 0.15f, 1.0f};

	private:
		bool isPanning = false;

	public:
		GridComponent(const glm::vec2& size) {
			this->position = {0.0f, 0.0f};
			this->size = size;
		}

		void Render(SDL_Renderer* renderer, const Camera& camera, const glm::vec2& screenSize) const override {
			SDL_SetRenderDrawColor(renderer,
								   static_cast<Uint8>(backgroundColor.r * 255.0f),
								   static_cast<Uint8>(backgroundColor.g * 255.0f),
								   static_cast<Uint8>(backgroundColor.b * 255.0f),
								   static_cast<Uint8>(backgroundColor.a * 255.0f));
			SDL_RenderClear(renderer);

			if(settings.visible) {
				const glm::vec2 topLeft = camera.ScreenToWorld({0.0f, 0.0f}, screenSize);
				const glm::vec2 bottomRight = camera.ScreenToWorld(screenSize, screenSize);

				RenderGridLines(renderer, camera, screenSize, topLeft, bottomRight, settings.smallStep, {40, 40, 40, 255});
				RenderGridLines(renderer, camera, screenSize, topLeft, bottomRight, settings.largeStep, {50, 50, 50, 255});
				RenderOriginCross(renderer, camera, screenSize);
			}

			Component::Render(renderer, camera, screenSize);
		}

		bool HandleEvent(const SDL_Event& event, const Camera& camera, const glm::vec2& screenSize) override {
			if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_MIDDLE) {
				isPanning = true;
				return true;
			}
			if(event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_MIDDLE) {
				isPanning = false;
				return true;
			}
			if(event.type == SDL_MOUSEMOTION && isPanning) {
				return true;
			}
			if(event.type == SDL_MOUSEWHEEL) {
				return true;
			}
			return false;
		}

		void XmlSerialize(pugi::xml_node& node) const override {
			settings.XmlSerialize(node);
		}

		void XmlDeserialize(const pugi::xml_node& node) override {
			settings.XmlDeserialize(node);
		}

		glm::vec2 SnapToGrid(const glm::vec2& worldPosition) const {
			const float step = settings.smallStep;
			return {std::round(worldPosition.x / step) * step, std::round(worldPosition.y / step) * step};
		}

		std::string GetTypeName() const override { return "GridComponent"; }

	private:
		static void RenderGridLines(SDL_Renderer* renderer, const Camera& camera, const glm::vec2 screenSize,
									glm::vec2 topLeft, glm::vec2 bottomRight, float step, SDL_Color color) {
			const int firstGridX = static_cast<int>(std::floor(topLeft.x / step));
			const int lastGridX = static_cast<int>(std::ceil(bottomRight.x / step));
			const int firstGridY = static_cast<int>(std::floor(topLeft.y / step));
			const int lastGridY = static_cast<int>(std::ceil(bottomRight.y / step));

			SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

			for(int i = firstGridX; i <= lastGridX; ++i) {
				const float worldX = static_cast<float>(i) * step;
				const glm::vec2 topPoint = camera.WorldToScreen({worldX, topLeft.y}, screenSize);
				const glm::vec2 bottomPoint = camera.WorldToScreen({worldX, bottomRight.y}, screenSize);
				SDL_RenderDrawLineF(renderer, topPoint.x, topPoint.y, bottomPoint.x, bottomPoint.y);
			}

			for(int i = firstGridY; i <= lastGridY; ++i) {
				const float worldY = static_cast<float>(i) * step;
				const glm::vec2 leftPoint = camera.WorldToScreen({topLeft.x, worldY}, screenSize);
				const glm::vec2 rightPoint = camera.WorldToScreen({bottomRight.x, worldY}, screenSize);
				SDL_RenderDrawLineF(renderer, leftPoint.x, leftPoint.y, rightPoint.x, rightPoint.y);
			}
		}

		static void RenderOriginCross(SDL_Renderer* renderer, const Camera& camera, const glm::vec2 screenSize) {
			constexpr glm::vec2 gridCenterWorld {0.0f, 0.0f};
			constexpr glm::vec2 crossMinWorld {-1.0f, -1.0f};
			constexpr glm::vec2 crossMaxWorld {1.0f, 1.0f};

			const glm::vec2 centerScreen = camera.WorldToScreen(gridCenterWorld, screenSize);
			const glm::vec2 crossMinScreen = camera.WorldToScreen(crossMinWorld, screenSize);
			const glm::vec2 crossMaxScreen = camera.WorldToScreen(crossMaxWorld, screenSize);

			SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
			SDL_RenderDrawLineF(renderer, crossMinScreen.x, centerScreen.y, crossMaxScreen.x, centerScreen.y);
			SDL_RenderDrawLineF(renderer, centerScreen.x, crossMinScreen.y, centerScreen.x, crossMaxScreen.y);
		}
	};

}
