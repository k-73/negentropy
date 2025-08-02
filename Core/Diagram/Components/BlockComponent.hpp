#pragma once

#include "Interface/Component.hpp"
#include "TextComponent.hpp"
#include "../../Main/DiagramData.hpp"

namespace Diagram
{
	class BlockComponent : public Component, public EventHandler
	{
	public:
		enum class Type { Start,
						  Process,
						  Decision,
						  End };

		struct Data {
			glm::vec2 position {0.0f};
			glm::vec2 size {10.0f, 5.0f};
			std::string label;
			Type type = Type::Process;
			glm::vec4 backgroundColor {0.8f, 0.33f, 0.08f, 0.7f};
			glm::vec4 borderColor {0.07f, 0.07f, 0.07f, 1.0f};
		} data;

	private:
		bool isBeingDragged = false;
		bool enableGridSnapping = true;
		bool showSnapPreview = true;
		glm::vec2 dragStartOffset {0.0f, 0.0f};
		TextComponent* labelComponent = nullptr;

	public:
		BlockComponent(const std::string& label, const glm::vec2& pos, const glm::vec2& size) {
			this->data.label = label;
			this->data.position = pos;
			this->data.size = size;
			
			// Sync with base class members
			this->position = pos;
			this->size = size;

			auto textLabel = std::make_unique<TextComponent>(label, glm::vec2(5.0f, 5.0f)); // Small offset for padding
			this->labelComponent = textLabel.get();

			this->AddChild(std::move(textLabel));
		}

		void Render(SDL_Renderer* renderer, const CameraView& view, const glm::vec2& screenSize) const override {
			if(!isVisible) {
				return;
			}

			// Use base class position instead of data.position for proper hierarchy
			const glm::vec2& worldPos = GetWorldPosition();
			const glm::vec2 screenPos = view.WorldToScreen(worldPos, screenSize);
			const SDL_FRect rect = {screenPos.x, screenPos.y, size.x * view.zoom, size.y * view.zoom};

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

		void RenderUI() override {
			char labelBuffer[256];
			std::strncpy(labelBuffer, data.label.c_str(), sizeof(labelBuffer) - 1);
			labelBuffer[sizeof(labelBuffer) - 1] = '\0';
			if(ImGui::InputText("Label", labelBuffer, sizeof(labelBuffer))) {
				data.label = labelBuffer;
				if(labelComponent) labelComponent->SetText(data.label);
			}

			// Sync position and size changes with base class
			if(ImGui::DragFloat2("Position", &data.position.x, 1.0f)) {
				position = data.position;
				DirtyCache();
			}
			if(ImGui::DragFloat2("Size", &data.size.x, 1.0f, 10.0f, 500.0f)) {
				size = data.size;
			}
			ImGui::ColorEdit4("Background", &data.backgroundColor.x);
			ImGui::ColorEdit4("Border", &data.borderColor.x);

			// Grid snapping controls
			ImGui::Checkbox("Grid Snapping", &enableGridSnapping);
			if(ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Enable automatic snapping to grid when dragging");
			}
			
			if(enableGridSnapping) {
				ImGui::SameLine();
				ImGui::Checkbox("Show Preview", &showSnapPreview);
				if(ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Show visual preview of snap position while dragging");
				}
			}

			const char* typeNames[] = {"Start", "Process", "Decision", "End"};
			int currentType = static_cast<int>(data.type);
			if(ImGui::Combo("Type", &currentType, typeNames, 4)) {
				data.type = static_cast<Type>(currentType);
			}
		}

		bool HandleEvent(const SDL_Event& event, const CameraView& view, const glm::vec2& screenSize) override {
			if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
				const glm::vec2 worldMousePos = view.ScreenToWorld({(float)event.button.x, (float)event.button.y}, screenSize);
				const glm::vec2& worldPos = GetWorldPosition();
				const bool contains = (worldPos.x <= worldMousePos.x && worldMousePos.x <= worldPos.x + size.x &&
									   worldPos.y <= worldMousePos.y && worldMousePos.y <= worldPos.y + size.y);
				if(contains) {
					isBeingDragged = true;
					dragStartOffset = worldMousePos - worldPos;
					Component::Select();
					return true;
				}
			}

			if(event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
				if(isBeingDragged) {
					isBeingDragged = false;
					return true;
				}
			}

			if(event.type == SDL_MOUSEMOTION && isBeingDragged) {
				const glm::vec2 worldMousePos = view.ScreenToWorld({(float)event.motion.x, (float)event.motion.y}, screenSize);
				glm::vec2 newPosition = worldMousePos - dragStartOffset;

				// Apply grid snapping if enabled and we have access to DiagramData
				if(enableGridSnapping) {
					if(auto* diagramData = DiagramData::GetInstance()) {
						newPosition = diagramData->GetGrid().SnapToGrid(newPosition);
					}
				}

				// Update both data.position and base class position
				data.position = newPosition;
				SetPosition(newPosition);
				return true;
			}

			return false;
		}

		std::string GetTypeName() const override { return "BlockComponent"; }

		void XmlSerialize(pugi::xml_node& node) const override {
			XML::auto_serialize(data, node);
			node.append_attribute("enableGridSnapping") = enableGridSnapping;
			node.append_attribute("showSnapPreview") = showSnapPreview;
		}

		void XmlDeserialize(const pugi::xml_node& node) override {
			XML::auto_deserialize(data, node);
			enableGridSnapping = node.attribute("enableGridSnapping").as_bool(true); // Default to true
			showSnapPreview = node.attribute("showSnapPreview").as_bool(true); // Default to true
		}
	};

}
