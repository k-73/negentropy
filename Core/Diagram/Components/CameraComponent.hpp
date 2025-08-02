#pragma once

#include "../../Utils/XMLSerialization.hpp"
#include "Interface/Component.hpp"

namespace Diagram
{
	class CameraComponent : public Component,
							public EventHandler
	{
	public:
		CameraView data;

	private:
		bool isPanning = false;

	public:
		CameraComponent(const glm::vec2& startPosition = {0.0f, 0.0f}, float startZoom = 1.0f) {
			position = startPosition;
			data.zoom = startZoom;
		}

		CameraView GetView() const {
			return {GetWorldPosition(), data.zoom};
		}

        void ZoomAt(const glm::vec2& screenPos, const glm::vec2& screenSize, float factor) {
            const CameraView currentView = GetView();
            const glm::vec2 worldPosBefore = currentView.ScreenToWorld(screenPos, screenSize);
            data.zoom *= factor;
            const CameraView viewAfterZoom = GetView();
            const glm::vec2 worldPosAfter = viewAfterZoom.ScreenToWorld(screenPos, screenSize);
            
            this->position += worldPosBefore - worldPosAfter;
            DirtyCache();
        }

		void Render(SDL_Renderer* renderer, const CameraView& view, const glm::vec2& screenSize) const override {
			// Don't draw a border for the camera that is currently providing the view.
			if(view.worldPosition == GetWorldPosition() && view.zoom == data.zoom) {
				return;
			}

			const CameraView thisView = GetView();
			const glm::vec2 topLeftWorld = thisView.ScreenToWorld({0.0f, 0.0f}, screenSize);
			const glm::vec2 bottomRightWorld = thisView.ScreenToWorld(screenSize, screenSize);

			// Use the active rendering view to convert our corners to the screen.
			const glm::vec2 screenTopLeft = view.WorldToScreen(topLeftWorld, screenSize);
			const glm::vec2 screenBottomRight = view.WorldToScreen(bottomRightWorld, screenSize);

			const SDL_FRect viewRect = {screenTopLeft.x, screenTopLeft.y, screenBottomRight.x - screenTopLeft.x, screenBottomRight.y - screenTopLeft.y};
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red
			SDL_RenderDrawRectF(renderer, &viewRect);
		}

		bool HandleEvent(const SDL_Event& event, const CameraView& view, const glm::vec2& screenSize) override {
			if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_MIDDLE) {
				isPanning = true;
				return true;
			}
			if(event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_MIDDLE) {
				isPanning = false;
				return true;
			}
			if(event.type == SDL_MOUSEMOTION && isPanning) {
				position.x -= event.motion.xrel / view.zoom;
				position.y -= event.motion.yrel / view.zoom;
				DirtyCache();
				return true;
			}
			if(event.type == SDL_MOUSEWHEEL) {
				const float zoomFactor = (event.wheel.y > 0) ? 1.1f : 1.0f / 1.1f;
				int mouseX, mouseY;
				SDL_GetMouseState(&mouseX, &mouseY);
				ZoomAt({(float)mouseX, (float)mouseY}, screenSize, zoomFactor);
				return true;
			}
			return false;
		}

		void RenderUI() override {
			ImGui::Text("Camera Component");
			ImGui::Separator();
			
			ImGui::DragFloat2("Position", &position.x, 1.0f);
			ImGui::DragFloat("Zoom", &data.zoom, 0.01f, 0.1f, 10.0f);
			
			if(ImGui::Button("Reset Camera")) {
				position = {0.0f, 0.0f};
				data.zoom = 1.0f;
				DirtyCache();
			}
		}

		void XmlSerialize(pugi::xml_node& node) const override {
			XML::auto_serialize(data, node);
		}

		void XmlDeserialize(const pugi::xml_node& node) override {
			XML::auto_deserialize(data, node);
		}

		std::string GetTypeName() const override { return "CameraComponent"; }
	};

}  // namespace Diagram