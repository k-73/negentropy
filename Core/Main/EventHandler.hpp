#pragma once

#include <SDL.h>
#include <vector>
#include <algorithm>
#include <glm/vec2.hpp>
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"

class EventHandler {
public:
    EventHandler() = default;
    ~EventHandler() = default;
    
    EventHandler(const EventHandler&) = delete;
    EventHandler& operator=(const EventHandler&) = delete;
    EventHandler(EventHandler&&) = delete;
    EventHandler& operator=(EventHandler&&) = delete;

    void HandleEvent(const SDL_Event& event, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks) noexcept {
        HandleMouseEvents(event, camera, blocks);
    }

private:
    void HandleMouseEvents(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks) noexcept {
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

    void HandleMouseButtonDown(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks) noexcept {
        if (e.button.button == SDL_BUTTON_MIDDLE) {
            camera.panning = true;
            camera.panStart = camera.position;
            camera.mouseStart = {static_cast<float>(e.button.x), static_cast<float>(e.button.y)};
        }
        if (e.button.button == SDL_BUTTON_LEFT) {
            const glm::vec2 worldPos = camera.ScreenToWorld({static_cast<float>(e.button.x), static_cast<float>(e.button.y)});

            for (int i = static_cast<int>(blocks.size()) - 1; i >= 0; --i) {
                auto& b = blocks[i];
                if (b.Contains(worldPos)) {
                    b.dragging = true;
                    b.dragOffset = worldPos - b.position;
                    std::rotate(blocks.begin() + i, blocks.begin() + i + 1, blocks.end());
                    break;
                }
            }
        }
    }

    void HandleMouseButtonUp(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks) noexcept {
        if (e.button.button == SDL_BUTTON_MIDDLE) camera.panning = false;
        if (e.button.button == SDL_BUTTON_LEFT) {
            for (auto& b : blocks) b.dragging = false;
        }
    }

    void HandleMouseMotion(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks) noexcept {
        const glm::vec2 currentMouse{static_cast<float>(e.motion.x), static_cast<float>(e.motion.y)};
        
        if (camera.panning) {
            const glm::vec2 mouseDelta = currentMouse - camera.mouseStart;
            camera.position = camera.panStart - mouseDelta;
        }
        
        const glm::vec2 worldPos = camera.ScreenToWorld(currentMouse);
        for (auto& b : blocks) {
            if (b.dragging) {
                b.position = worldPos - b.dragOffset;
            }
        }
    }
};
