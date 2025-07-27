#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <imgui.h>

namespace Notify {
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
        float duration;
        bool dismissing = false;
        
        Toast(std::string msg, Type t, float dur = 3.0f) 
            : message(std::move(msg)), type(t), createdAt(std::chrono::steady_clock::now()), duration(dur) {}
    };

    class Manager {
    public:
        static Manager& Instance() {
            static Manager instance;
            return instance;
        }

        void AddToast(const std::string& message, Type type, float duration = 3.0f) {
            m_toasts.emplace_back(message, type, duration);
        }

        void Render() {
            auto now = std::chrono::steady_clock::now();
            
            // Remove expired toasts
            m_toasts.erase(
                std::remove_if(m_toasts.begin(), m_toasts.end(), [now](const Toast& toast) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - toast.createdAt).count() / 1000.0f;
                    return elapsed > toast.duration;
                }), 
                m_toasts.end()
            );

            if (m_toasts.empty()) return;

            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            const float PAD = 10.0f;
            ImVec2 work_pos = viewport->WorkPos;
            ImVec2 work_size = viewport->WorkSize;
            ImVec2 window_pos = ImVec2(work_pos.x + work_size.x - PAD, work_pos.y + PAD);

            ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
            ImGui::SetNextWindowBgAlpha(0.9f);

            if (ImGui::Begin("Notifications", nullptr, 
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | 
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav)) {
                
                for (auto& toast : m_toasts) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - toast.createdAt).count() / 1000.0f;
                    float alpha = 1.0f;
                    
                    if (elapsed > toast.duration - 0.5f) {
                        alpha = (toast.duration - elapsed) / 0.5f;
                    }

                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                    
                    ImVec4 color;
                    const char* icon;
                    switch (toast.type) {
                        case Type::Info:    color = ImVec4(0.2f, 0.6f, 1.0f, 1.0f); icon = "ℹ"; break;
                        case Type::Success: color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f); icon = "✓"; break;
                        case Type::Warning: color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); icon = "⚠"; break;
                        case Type::Error:   color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); icon = "✗"; break;
                    }

                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::Text("%s %s", icon, toast.message.c_str());
                    ImGui::PopStyleColor();
                    ImGui::PopStyleVar();
                }
            }
            ImGui::End();
        }

    private:
        std::vector<Toast> m_toasts;
    };

    inline void Info(const std::string& message, float duration = 3.0f) {
        Manager::Instance().AddToast(message, Type::Info, duration);
    }

    inline void Success(const std::string& message, float duration = 3.0f) {
        Manager::Instance().AddToast(message, Type::Success, duration);
    }

    inline void Warning(const std::string& message, float duration = 3.0f) {
        Manager::Instance().AddToast(message, Type::Warning, duration);
    }

    inline void Error(const std::string& message, float duration = 3.0f) {
        Manager::Instance().AddToast(message, Type::Error, duration);
    }

    inline void Render() {
        Manager::Instance().Render();
    }
}