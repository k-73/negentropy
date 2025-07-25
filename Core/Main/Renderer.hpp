#pragma once

#include <SDL.h>
#include <cstdio>
#include <vector>
#include <spdlog/spdlog.h>
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    bool Initialize(SDL_Window* window) {
        spdlog::info("Attempting fast hardware renderer...");
        
        // Try hardware accelerated without VSync first for faster init
        m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!m_renderer) {
            spdlog::warn("Fast hardware renderer failed: {}, trying with VSync...", SDL_GetError());
            // Try with VSync if that failed
            m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (!m_renderer) {
                spdlog::warn("Hardware accelerated renderer failed: {}", SDL_GetError());
                spdlog::info("Falling back to software renderer...");
                // Fallback to software rendering
                m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
                if (!m_renderer) {
                    spdlog::error("Software renderer also failed: {}", SDL_GetError());
                    return false;
                } else {
                    spdlog::info("Software renderer created successfully");
                }
            } else {
                spdlog::info("Hardware accelerated renderer with VSync created successfully");
            }
        } else {
            spdlog::info("Fast hardware renderer created successfully");
            
            // Log renderer info
            SDL_RendererInfo info;
            if (SDL_GetRendererInfo(m_renderer, &info) == 0) {
                spdlog::info("Renderer name: {}", info.name);
                spdlog::info("Renderer flags: {}", info.flags);
                spdlog::info("Texture formats: {}", info.num_texture_formats);
                spdlog::info("Max texture size: {}x{}", info.max_texture_width, info.max_texture_height);
            }
        }
        
        // Set blend mode for proper transparency
        if (SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND) != 0) {
            spdlog::warn("Failed to set blend mode: {}", SDL_GetError());
        } else {
            spdlog::info("Blend mode set successfully");
        }
        
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
