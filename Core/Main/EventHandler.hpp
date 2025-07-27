#pragma once

#include <SDL.h>
#include <vector>
#include <algorithm>
#include <glm/vec2.hpp>
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"
#include "ComponentSelection.hpp"

class EventHandler {
public:
    static void HandleEvent(const SDL_Event& event, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks, const glm::vec2 screenSize) noexcept {
        HandleMouseEvents(event, camera, blocks, screenSize);
        HandleScrollEvents(event, camera, screenSize);
    }

private:
    static void HandleScrollEvents(const SDL_Event& e, Diagram::Camera& camera, const glm::vec2 screenSize) noexcept {
        if (e.type == SDL_MOUSEWHEEL) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            
            const float zoomFactor = e.wheel.y > 0 ? 1.1f : 0.9f;
            camera.ZoomAt({static_cast<float>(mouseX), static_cast<float>(mouseY)}, screenSize, zoomFactor);
        }
    }

    static void HandleMouseEvents(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks, glm::vec2 screenSize) noexcept {
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            HandleMouseButtonDown(e, camera, blocks, screenSize);
        }
        else if (e.type == SDL_MOUSEBUTTONUP) {
            HandleMouseButtonUp(e, camera, blocks, screenSize);
        }
        else if (e.type == SDL_MOUSEMOTION) {
            HandleMouseMotion(e, camera, blocks, screenSize);
        }
    }

    static void HandleMouseButtonDown(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks, const glm::vec2 screenSize) noexcept {
        if (e.button.button == SDL_BUTTON_MIDDLE) {
            camera.panning = true;
            camera.panStart = camera.data.position;
            camera.mouseStart = {static_cast<float>(e.button.x), static_cast<float>(e.button.y)};
        }
        if (e.button.button == SDL_BUTTON_LEFT) {
            bool blockSelected = false;
            for (int i = static_cast<int>(blocks.size()) - 1; i >= 0; --i) {
                auto& block = blocks[i];
                if (block.HandleEvent(e, camera, screenSize)) {
                    ComponentSelection::Instance().Select(&block);
                    std::rotate(blocks.begin() + i, blocks.begin() + i + 1, blocks.end());
                    blockSelected = true;
                    break;
                }
            }
            if (!blockSelected) {
                ComponentSelection::Instance().Clear();
            }
        }
    }

    static void HandleMouseButtonUp(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks, const glm::vec2 screenSize) noexcept {
        if (e.button.button == SDL_BUTTON_MIDDLE) camera.panning = false;
        if (e.button.button == SDL_BUTTON_LEFT) {
            for (auto& block : blocks) {
                block.HandleEvent(e, camera, screenSize);
            }
        }
    }

    static void HandleMouseMotion(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks, const glm::vec2 screenSize) noexcept {
        const glm::vec2 currentMouse{static_cast<float>(e.motion.x), static_cast<float>(e.motion.y)};
        
        if (camera.panning) {
            const glm::vec2 mouseDelta = currentMouse - camera.mouseStart;
            camera.data.position = camera.panStart - mouseDelta / camera.data.zoom;
        }
        
        for (auto& block : blocks) {
            block.HandleEvent(e, camera, screenSize);
        }
    }
};
