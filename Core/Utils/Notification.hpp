#pragma once

#include <imgui.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

#include "IconsFontAwesome5.h"

namespace Notify
{
	enum class Type {
		Info,
		Success,
		Warning,
		Error
	};

	struct Toast {
		std::string message;
		Type type;
		std::chrono::steady_clock::time_point createdAt;
		float durationSeconds;
		bool isDismissing = false;

		Toast(const std::string& generalMessage, Type generalType, float generalDurationSeconds = 3.0f)
			: message(generalMessage),
			  type(generalType),
			  createdAt(std::chrono::steady_clock::now()),
			  durationSeconds(generalDurationSeconds) {}
	};

	class Manager
	{
	public:
		static Manager& Instance() {
			static Manager instance;
			return instance;
		}

		void AddToast(const std::string& generalMessage, Type generalType, float generalDurationSeconds = 3.0f) {
			toasts.emplace_back(generalMessage, generalType, generalDurationSeconds);
		}

		void Render() {
			const auto now = std::chrono::steady_clock::now();

			// Remove expired toasts
			toasts.erase(
				std::remove_if(toasts.begin(), toasts.end(), [now](const Toast& toast) {
					const auto elapsedSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - toast.createdAt).count() / 1000.0f;
					return elapsedSeconds > toast.durationSeconds;
				}),
				toasts.end());

			if(toasts.empty()) return;

			const float padding = 10.0f;
			const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
			const ImVec2 workPosition = mainViewport->WorkPos;
			const ImVec2 workSize = mainViewport->WorkSize;
			const ImVec2 windowPosition = ImVec2(workPosition.x + workSize.x - padding, workPosition.y + padding * 2);

			ImGui::SetNextWindowPos(windowPosition, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
			ImGui::SetNextWindowBgAlpha(0.9f);

			if(ImGui::Begin("Notifications", nullptr,
							ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration |
								ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
								ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
				for(auto& toast: toasts) {
					const auto elapsedSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - toast.createdAt).count() / 1000.0f;
					float alpha = 1.0f;

					if(elapsedSeconds > toast.durationSeconds - 0.5f) {
						alpha = (toast.durationSeconds - elapsedSeconds) / 0.5f;
					}

					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

					ImVec4 toastColor;
					const char* toastIcon;
					switch(toast.type) {
						case Type::Info:
							toastColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
							toastIcon = ICON_FA_INFO_CIRCLE;
							break;
						case Type::Success:
							toastColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
							toastIcon = ICON_FA_CHECK_CIRCLE;
							break;
						case Type::Warning:
							toastColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
							toastIcon = ICON_FA_EXCLAMATION_TRIANGLE;
							break;
						case Type::Error:
							toastColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
							toastIcon = ICON_FA_TIMES_CIRCLE;
							break;
					}

					ImGui::PushStyleColor(ImGuiCol_Border, toastColor);
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

					if(ImGui::BeginChild(("toast_" + std::to_string(reinterpret_cast<uintptr_t>(&toast))).c_str(),
										 ImVec2(0, 0), ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
						ImGui::PushStyleColor(ImGuiCol_Text, toastColor);
						ImGui::Text("%s %s", toastIcon, toast.message.c_str());
						ImGui::PopStyleColor();
					}
					ImGui::EndChild();

					ImGui::PopStyleVar(1);
					ImGui::PopStyleColor();
					ImGui::PopStyleVar();
				}
			}
			ImGui::End();
		}

	private:
		std::vector<Toast> toasts;
	};

	inline void Info(const std::string& generalMessage, float generalDurationSeconds = 3.0f) {
		Manager::Instance().AddToast(generalMessage, Type::Info, generalDurationSeconds);
	}

	inline void Success(const std::string& generalMessage, float generalDurationSeconds = 3.0f) {
		Manager::Instance().AddToast(generalMessage, Type::Success, generalDurationSeconds);
	}

	inline void Warning(const std::string& generalMessage, float generalDurationSeconds = 3.0f) {
		Manager::Instance().AddToast(generalMessage, Type::Warning, generalDurationSeconds);
	}

	inline void Error(const std::string& generalMessage, float generalDurationSeconds = 3.0f) {
		Manager::Instance().AddToast(generalMessage, Type::Error, generalDurationSeconds);
	}

	inline void Render() {
		Manager::Instance().Render();
	}
}