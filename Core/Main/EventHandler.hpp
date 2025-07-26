#pragma once

#include <SDL.h>
#include <vector>
#include <algorithm>
#include <glm/vec2.hpp>
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"

class EventHandler {
public:
    static void HandleEvent(const SDL_Event& event, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks) noexcept {
        HandleMouseEvents(event, camera, blocks);
        HandleScrollEvents(event, camera);
    }

private:
    static void HandleScrollEvents(const SDL_Event& e, Diagram::Camera& camera) noexcept {
        if (e.type == SDL_MOUSEWHEEL) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            
            const float zoomFactor = e.wheel.y > 0 ? 1.1f : 0.9f;
            camera.ZoomAt({static_cast<float>(mouseX), static_cast<float>(mouseY)}, zoomFactor);
        }
    }

    static void HandleMouseEvents(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks) noexcept {
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            HandleMouseButtonDown(e, camera, blocks);
        }
        else if (e.type == SDL_MOUSEBUTTONUP) {
            HandleMouseButtonUp(e, camera, blocks);
        }
        else if (e.type == SDL_MOUSEMOTION) {
            HandleMouseMotion(e, camera, blocks);
        }
    }

    static void HandleMouseButtonDown(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks) noexcept {
        if (e.button.button == SDL_BUTTON_MIDDLE) {
            camera.panning = true;
            camera.panStart = camera.data.position;
            camera.mouseStart = {static_cast<float>(e.button.x), static_cast<float>(e.button.y)};
        }
        if (e.button.button == SDL_BUTTON_LEFT) {
            const glm::vec2 worldPos = camera.ScreenToWorld({static_cast<float>(e.button.x), static_cast<float>(e.button.y)});

            for (int i = static_cast<int>(blocks.size()) - 1; i >= 0; --i) {
                auto& b = blocks[i];
                if (b.Contains(worldPos)) {
                    b.dragging = true;
                    b.dragOffset = worldPos - b.data.position;
                    std::rotate(blocks.begin() + i, blocks.begin() + i + 1, blocks.end());
                    break;
                }
            }
        }
    }

    static void HandleMouseButtonUp(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks) noexcept {
        if (e.button.button == SDL_BUTTON_MIDDLE) camera.panning = false;
        if (e.button.button == SDL_BUTTON_LEFT) {
            for (auto& b : blocks) b.dragging = false;
        }
    }

    static void HandleMouseMotion(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks) noexcept {
        const glm::vec2 currentMouse{static_cast<float>(e.motion.x), static_cast<float>(e.motion.y)};
        
        if (camera.panning) {
            const glm::vec2 mouseDelta = currentMouse - camera.mouseStart;
            camera.data.position = camera.panStart - mouseDelta / camera.data.zoom;
        }
        
        const glm::vec2 worldPos = camera.ScreenToWorld(currentMouse);
        for (auto& b : blocks) {
            if (b.dragging) {
                b.data.position = worldPos - b.dragOffset;
            }
        }
    }
};
