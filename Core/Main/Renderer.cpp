#include "Renderer.hpp"
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

bool Renderer::Initialize(SDL_Window* window) noexcept {
    m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!m_renderer) {
        m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!m_renderer) {
            m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
            if (!m_renderer) return false;
        }
    }
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    return true;
}

void Renderer::Clear() noexcept {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 255);
    SDL_RenderClear(m_renderer);
}

void Renderer::DrawGrid(const Diagram::Camera& camera) noexcept {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    SDL_SetRenderDrawColor(m_renderer, 50, 50, 50, 255);
    constexpr int step = 50;
    int w, h;
    SDL_GetRendererOutputSize(m_renderer, &w, &h);
    
    const int offsetX = -static_cast<int>(static_cast<int>(camera.position.x) % step);
    const int offsetY = -static_cast<int>(static_cast<int>(camera.position.y) % step);
    
    for (int x = offsetX; x < w; x += step)
        SDL_RenderDrawLine(m_renderer, x, 0, x, h);
    for (int y = offsetY; y < h; y += step)
        SDL_RenderDrawLine(m_renderer, 0, y, w, y);
}

void Renderer::DrawBlocks(const std::vector<Diagram::Block>& blocks, const Diagram::Camera& camera) noexcept {
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    for (const auto& block : blocks) {
        const auto screenPos = camera.WorldToScreen(block.position);
        const SDL_FRect rect = {screenPos.x, screenPos.y, block.size.x, block.size.y};
        
        const auto r = static_cast<Uint8>(block.color.r * 255.0f);
        const auto g = static_cast<Uint8>(block.color.g * 255.0f);
        const auto b = static_cast<Uint8>(block.color.b * 255.0f);
        const auto a = static_cast<Uint8>(block.color.a * 255.0f);
        
        SDL_SetRenderDrawColor(m_renderer, r, g, b, a);
        SDL_RenderFillRectF(m_renderer, &rect);
        SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
        SDL_RenderDrawRectF(m_renderer, &rect);
    }
}

void Renderer::Present() noexcept {
    SDL_RenderPresent(m_renderer);
}

void Renderer::Cleanup() noexcept {
    m_renderer = nullptr;
}

SDL_Renderer* Renderer::GetSDLRenderer() const noexcept {
    return m_renderer;
}