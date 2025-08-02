#pragma once

#include "Interface/Component.hpp"
#include "TextComponent.hpp"
#include <cmath>

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

			// Create text label centered in the block
			auto textLabel = std::make_unique<TextComponent>(data.label, glm::vec2(0.0f, 0.0f));
			this->labelComponent = textLabel.get();
			this->AddChild(std::move(textLabel));
			
			// Center the text in the block
			CenterText();
		}

		void CenterText() {
			if(labelComponent) {
				// Calculate proper center based on text size
				const glm::vec2& textSize = labelComponent->size;
				glm::vec2 textPos = glm::vec2(
					(size.x - textSize.x) * 0.5f,
					(size.y - textSize.y) * 0.5f
				);
				labelComponent->SetPosition(textPos);
			}
		}

		void Render(SDL_Renderer* renderer, const CameraView& view, const glm::vec2& screenSize) const override {
			if(!isVisible) {
				return;
			}

			// Draw the block background based on type
			const glm::vec2 screenPos = view.WorldToScreen(GetWorldPosition(), screenSize);
			const glm::vec2 screenSize_block = size * view.zoom;

			// Set the block colors
			const SDL_Color fillColor = {
				(Uint8)(data.backgroundColor.r * 255),
				(Uint8)(data.backgroundColor.g * 255),
				(Uint8)(data.backgroundColor.b * 255),
				(Uint8)(data.backgroundColor.a * 255)
			};
			const SDL_Color borderColor = {
				(Uint8)(data.borderColor.r * 255),
				(Uint8)(data.borderColor.g * 255),
				(Uint8)(data.borderColor.b * 255),
				(Uint8)(data.borderColor.a * 255)
			};

			// Set blend mode for transparency
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

			switch(data.type) {
				case Type::Start:
				case Type::End: {
					// Draw circle
					const float radius = std::min(screenSize_block.x, screenSize_block.y) * 0.4f;
					const glm::vec2 center = screenPos + screenSize_block * 0.5f;
					
					// Simple circle drawing with lines
					SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
					for(int y = -radius; y <= radius; y++) {
						for(int x = -radius; x <= radius; x++) {
							if(x*x + y*y <= radius*radius) {
								SDL_RenderDrawPoint(renderer, center.x + x, center.y + y);
							}
						}
					}
					
					// Draw border
					SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
					for(int angle = 0; angle < 360; angle += 2) {
						float rad = angle * M_PI / 180.0f;
						int x = center.x + radius * cos(rad);
						int y = center.y + radius * sin(rad);
						SDL_RenderDrawPoint(renderer, x, y);
					}
					break;
				}
				case Type::Decision: {
					// Draw diamond/rhombus
					const glm::vec2 center = screenPos + screenSize_block * 0.5f;
					const float halfWidth = screenSize_block.x * 0.4f;
					const float halfHeight = screenSize_block.y * 0.4f;
					
					// Simple diamond fill
					SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
					for(int y = -halfHeight; y <= halfHeight; y++) {
						float widthAtY = halfWidth * (1.0f - abs(y) / halfHeight);
						for(int x = -widthAtY; x <= widthAtY; x++) {
							SDL_RenderDrawPoint(renderer, center.x + x, center.y + y);
						}
					}
					break;
				}
				case Type::Process:
				default: {
					// Draw rectangle
					SDL_Rect fillRect = {
						(int)screenPos.x,
						(int)screenPos.y,
						(int)screenSize_block.x,
						(int)screenSize_block.y
					};
					
					// Fill
					SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
					SDL_RenderFillRect(renderer, &fillRect);
					
					// Border
					SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
					SDL_RenderDrawRect(renderer, &fillRect);
					break;
				}
			}
		}

		void RenderUI() override {
			char labelBuffer[256];
			std::strncpy(labelBuffer, data.label.c_str(), sizeof(labelBuffer) - 1);
			labelBuffer[sizeof(labelBuffer) - 1] = '\0';
			if(ImGui::InputText("Label", labelBuffer, sizeof(labelBuffer))) {
				data.label = labelBuffer;
				if(labelComponent) {
					labelComponent->SetText(data.label);
					CenterText(); // Re-center text when label changes
				}
			}

			// Sync position and size changes with base class
			if(ImGui::DragFloat2("Position", &data.position.x, 1.0f)) {
				position = data.position;
				DirtyCache();
			}
			if(ImGui::DragFloat2("Size", &data.size.x, 1.0f, 10.0f, 500.0f)) {
				size = data.size;
				CenterText(); // Re-center text when size changes
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
				// No need to recreate anything - just the shape changes in Render()
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

				if(enableGridSnapping) {
					newPosition.x = std::round(newPosition.x / 10.0f) * 10.0f;
					newPosition.y = std::round(newPosition.y / 10.0f) * 10.0f;
				}

				// Update both data.position and base class position
				data.position = newPosition;
				SetPosition(newPosition);
				return true;
			}

			return false;
		}

		std::string GetTypeName() const override { return "BlockComponent"; }

		void OnCacheDirty() override {
			// Re-center text when position or size changes
			CenterText();
		}

		void OnChildSizeChanged(Component* child) override {
			// If our text component size changed, re-center it
			if(child == labelComponent) {
				CenterText();
			}
		}

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
