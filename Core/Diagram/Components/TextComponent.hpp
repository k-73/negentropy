#pragma once

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
			// TODO: Add other properties like color if needed
		} data;

		void SetText(const std::string& newText) {
			data.text = newText;

			ImFont* font = ImGui::GetFont();
			if(font) {
				const ImVec2 textSize = font->CalcTextSizeA(data.baseFontSize, FLT_MAX, 0.0f, newText.c_str());
				this->size = {textSize.x, textSize.y};
			}
		}

	public:
		TextComponent(const std::string& text, const glm::vec2& pos) {
			data.text = text;
			this->position = pos;

			ImFont* font = ImGui::GetFont();
			if(font) {
				const float scaledFontSize = 14;
				const ImVec2 textSize = font->CalcTextSizeA(scaledFontSize, FLT_MAX, 0.0f, text.c_str());
				this->size = {textSize.x, textSize.y};
			}
		}

		void Render(SDL_Renderer* renderer, const CameraView& view, const glm::vec2& screenSize) const override {
			if(data.text.empty()) return;

			const auto& worldPos = GetWorldPosition();
			const glm::vec2 screenPos = view.WorldToScreen(worldPos, screenSize);

			ImDrawList* drawList = ImGui::GetBackgroundDrawList();
			ImFont* font = ImGui::GetFont();

			const float scaledFontSize = data.baseFontSize * view.zoom;

			drawList->AddText(
				font,
				scaledFontSize,
				ImVec2(screenPos.x, screenPos.y),
				IM_COL32(255, 255, 255, 255),
				data.text.c_str());
		}

		void RenderUI() override {
			char labelBuffer[256];
			std::strncpy(labelBuffer, data.text.c_str(), sizeof(labelBuffer) - 1);
			labelBuffer[sizeof(labelBuffer) - 1] = '\0';

			if(ImGui::InputText("Text", labelBuffer, sizeof(labelBuffer))) {
				SetText(labelBuffer);
			}
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
