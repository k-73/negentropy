#pragma once

#include <SDL.h>
#include <cstdio>
#include <vector>
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    bool Initialize(SDL_Window* window) {
        // Try hardware accelerated first, fallback to software if needed
        m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!m_renderer) {
            // Fallback to software rendering
            m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
            if (!m_renderer) {
                std::fprintf(stderr, "SDL_CreateRenderer error: %s\n", SDL_GetError());
                return false;
            }
        }
        
        // Set blend mode for proper transparency
        SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
        
        return true;
    }

    void Clear() {
        SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 255);
        SDL_RenderClear(m_renderer);
    }

    void DrawGrid(const Diagram::Camera& camera) {
        SDL_SetRenderDrawColor(m_renderer, 50, 50, 50, 255);
        const int step = 50;
        int w, h;
        SDL_GetRendererOutputSize(m_renderer, &w, &h);
        for (int x = -((int)camera.x % step); x < w; x += step)
            SDL_RenderDrawLine(m_renderer, x, 0, x, h);
        for (int y = -((int)camera.y % step); y < h; y += step)
            SDL_RenderDrawLine(m_renderer, 0, y, w, y);
    }

    void DrawBlocks(const std::vector<Diagram::Block>& blocks, const Diagram::Camera& camera) {
        for (const auto& b : blocks) {
            SDL_FRect screen = {b.rect.x - camera.x, b.rect.y - camera.y, b.rect.w, b.rect.h};
            SDL_SetRenderDrawColor(m_renderer, 90, 120, 200, 255);
            SDL_RenderFillRectF(m_renderer, &screen);
            SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
            SDL_RenderDrawRectF(m_renderer, &screen);
        }
    }

    void Present() {
        SDL_RenderPresent(m_renderer);
    }

    void Cleanup() {
        if (m_renderer) {
            SDL_DestroyRenderer(m_renderer);
            m_renderer = nullptr;
        }
    }

    SDL_Renderer* GetSDLRenderer() { return m_renderer; }

private:
    SDL_Renderer* m_renderer = nullptr;
};
