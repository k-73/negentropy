#pragma once

#include <SDL.h>
#include <vector>
#include <algorithm>
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
            camera.panStart = {(int)camera.x, (int)camera.y};
            camera.mouseStart = {e.button.x, e.button.y};
        }
        if (e.button.button == SDL_BUTTON_LEFT) {
            const float wx = e.button.x + camera.x;
            const float wy = e.button.y + camera.y;

            for (int i = (int)blocks.size() - 1; i >= 0; --i) {
                auto& b = blocks[i];
                if (Diagram::PointInRect(b.rect, wx, wy)) {
                    b.dragging = true;
                    b.dragOffset = {wx - b.rect.x, wy - b.rect.y};
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
        if (camera.panning) {
            camera.x = camera.panStart.x - (e.motion.x - camera.mouseStart.x);
            camera.y = camera.panStart.y - (e.motion.y - camera.mouseStart.y);
        }
        const float wx = e.motion.x + camera.x;
        const float wy = e.motion.y + camera.y;
        for (auto& b : blocks) {
            if (b.dragging) {
                b.rect.x = wx - b.dragOffset.x;
                b.rect.y = wy - b.dragOffset.y;
            }
        }
    }
};
