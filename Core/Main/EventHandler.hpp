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

    bool ProcessEvents(Diagram::Camera& camera, std::vector<Diagram::Block>& blocks, bool& running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }

            HandleMouseEvents(e, camera, blocks);
        }
        return true;
    }

private:
    void HandleMouseEvents(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks) {
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

    void HandleMouseButtonDown(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks) {
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

    void HandleMouseButtonUp(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks) {
        if (e.button.button == SDL_BUTTON_MIDDLE) camera.panning = false;
        if (e.button.button == SDL_BUTTON_LEFT) {
            for (auto& b : blocks) b.dragging = false;
        }
    }

    void HandleMouseMotion(const SDL_Event& e, Diagram::Camera& camera, std::vector<Diagram::Block>& blocks) {
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
