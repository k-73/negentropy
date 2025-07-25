#pragma once

#include <SDL.h>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"
#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    bool Initialize(SDL_Renderer* renderer) {
        m_renderer = renderer;
        if (!m_renderer) return false;
        SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
        return true;
    }

    void Clear() {
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif
        SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 255);
        SDL_RenderClear(m_renderer);
    }

    void DrawGrid(const Diagram::Camera& camera) {
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif
        SDL_SetRenderDrawColor(m_renderer, 50, 50, 50, 255);
        const int step = 50;
        int w, h;
        SDL_GetRendererOutputSize(m_renderer, &w, &h);
        
        const int offsetX = -static_cast<int>(static_cast<int>(camera.position.x) % step);
        const int offsetY = -static_cast<int>(static_cast<int>(camera.position.y) % step);
        
        for (int x = offsetX; x < w; x += step)
            SDL_RenderDrawLine(m_renderer, x, 0, x, h);
        for (int y = offsetY; y < h; y += step)
            SDL_RenderDrawLine(m_renderer, 0, y, w, y);
    }

    void DrawBlocks(const std::vector<Diagram::Block>& blocks, const Diagram::Camera& camera) {
#ifdef TRACY_ENABLE
        ZoneScoped;
#endif
        for (const auto& b : blocks) {
            const glm::vec2 screenPos = camera.WorldToScreen(b.position);
            SDL_FRect screen = {screenPos.x, screenPos.y, b.size.x, b.size.y};
            
            const auto color = static_cast<Uint8>(b.color.r * 255.0f);
            const auto colorG = static_cast<Uint8>(b.color.g * 255.0f);
            const auto colorB = static_cast<Uint8>(b.color.b * 255.0f);
            const auto colorA = static_cast<Uint8>(b.color.a * 255.0f);
            
            SDL_SetRenderDrawColor(m_renderer, color, colorG, colorB, colorA);
            SDL_RenderFillRectF(m_renderer, &screen);
            SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
            SDL_RenderDrawRectF(m_renderer, &screen);
        }
    }

    void Present() {
        SDL_RenderPresent(m_renderer);
    }

    void Cleanup() {
        m_renderer = nullptr;
    }

    SDL_Renderer* GetSDLRenderer() { return m_renderer; }

private:
    SDL_Renderer* m_renderer = nullptr;
};
