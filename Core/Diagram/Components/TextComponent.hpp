#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "Interface/Component.hpp"
#include "imgui.h"

namespace Diagram
{
	struct Camera;

	class TextComponent : public Component
	{
	public:
		struct Data {
			std::string text;
			int baseFontSize = 14;
			glm::vec4 color {1.0f, 1.0f, 1.0f, 1.0f};  // White text by default
		} data;

		void SetText(const std::string& newText) {
			data.text = newText;
			UpdateTextSize();
		}

		void UpdateTextSize() {
			if(data.text.empty()) {
				size = {0.0f, 0.0f};
				return;
			}

			// Get or create a font for SDL rendering to calculate accurate size
			static TTF_Font* font = nullptr;
			static int lastFontSize = -1;

			// Recreate font if size changed
			if(!font || lastFontSize != data.baseFontSize) {
				if(font) {
					TTF_CloseFont(font);
					font = nullptr;
				}

				font = TTF_OpenFont("../Assets/fonts/LiberationSans-Regular.ttf", data.baseFontSize);
				if(!font) {
					// Fallback to system font
					font = TTF_OpenFont("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", data.baseFontSize);
				}
				lastFontSize = data.baseFontSize;
			}

			if(font) {
				int textWidth, textHeight;
				if(TTF_SizeText(font, data.text.c_str(), &textWidth, &textHeight) == 0) {
					size = {(float)textWidth, (float)textHeight};
				} else {
					// Fallback to ImGui calculation if TTF fails
					ImFont* imguiFont = ImGui::GetFont();
					if(imguiFont) {
						const ImVec2 textSize = imguiFont->CalcTextSizeA(data.baseFontSize, FLT_MAX, 0.0f, data.text.c_str());
						size = {textSize.x, textSize.y};
					}
				}
			}
			DirtyCache();  // This will trigger parent's OnCacheDirty()
		}

	public:
		TextComponent(const std::string& text, const glm::vec2& pos) {
			data.text = text;
			this->position = pos;
			UpdateTextSize();
		}

		void Render(SDL_Renderer* renderer, const CameraView& view, const glm::vec2& screenSize) const override {
			if(data.text.empty()) return;

			// Calculate scaled font size based on zoom
			int scaledFontSize = (int)(data.baseFontSize * view.zoom);
			if(scaledFontSize < 1) scaledFontSize = 1;	// Minimum font size

			// Get or create a font for SDL rendering - ensure it matches the size calculation
			static TTF_Font* renderFont = nullptr;
			static int lastRenderFontSize = -1;

			// Recreate font if size changed
			if(!renderFont || lastRenderFontSize != scaledFontSize) {
				if(renderFont) {
					TTF_CloseFont(renderFont);
					renderFont = nullptr;
				}

				renderFont = TTF_OpenFont("../Assets/fonts/LiberationSans-Regular.ttf", scaledFontSize);
				if(!renderFont) {
					// Fallback to system font
					renderFont = TTF_OpenFont("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", scaledFontSize);
				}
				lastRenderFontSize = scaledFontSize;
			}

			if(!renderFont) return;	 // No font available

			const auto& worldPos = GetWorldPosition();
			const glm::vec2 screenPos = view.WorldToScreen(worldPos, screenSize);

			// Create SDL surface from text
			SDL_Color textColor = {
				(Uint8)(data.color.r * 255),
				(Uint8)(data.color.g * 255),
				(Uint8)(data.color.b * 255),
				(Uint8)(data.color.a * 255)};

			SDL_Surface* textSurface = TTF_RenderText_Blended(renderFont, data.text.c_str(), textColor);
			if(!textSurface) return;

			// Create texture from surface
			SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
			if(textTexture) {
				// Set alpha blending
				SDL_SetTextureBlendMode(textTexture, SDL_BLENDMODE_BLEND);
				SDL_SetTextureAlphaMod(textTexture, (Uint8)(data.color.a * 255));

				// Render the texture
				SDL_Rect dstRect = {
					(int)screenPos.x,
					(int)screenPos.y,
					textSurface->w,
					textSurface->h};

				SDL_RenderCopy(renderer, textTexture, nullptr, &dstRect);
				SDL_DestroyTexture(textTexture);
			}

			SDL_FreeSurface(textSurface);
		}

		void RenderUI() override {
			ImGui::Text("Text Component");
			ImGui::Separator();

			char labelBuffer[256];
			std::strncpy(labelBuffer, data.text.c_str(), sizeof(labelBuffer) - 1);
			labelBuffer[sizeof(labelBuffer) - 1] = '\0';

			if(ImGui::InputText("Text", labelBuffer, sizeof(labelBuffer))) {
				SetText(labelBuffer);
			}

			if(ImGui::DragFloat2("Position", &position.x, 1.0f)) {
				DirtyCache();
			}

			if(ImGui::DragFloat2("Size", &size.x, 1.0f, 1.0f, 500.0f)) {
				// Size is automatically calculated based on text, but we can display it
				// Note: This is read-only for text as size depends on font and text content
			}

			if(ImGui::SliderInt("Font Size", &data.baseFontSize, 8, 48)) {
				UpdateTextSize();  // Recalculate size when font size changes
			}

			ImGui::ColorEdit4("Text Color", &data.color.x);
		}

		std::string GetTypeName() const override { return "TextComponent"; }

		void XmlSerialize(pugi::xml_node& node) const override {
			XML::auto_serialize(data, node);
		}

		void XmlDeserialize(const pugi::xml_node& node) override {
			XML::auto_deserialize(data, node);
		}
	};

}  // namespace Diagram
