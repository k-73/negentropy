#include "Renderer.hpp"
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

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
    SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 255);
    SDL_RenderClear(m_renderer);
}

void Renderer::DrawGrid(const Diagram::Camera& camera) noexcept {
    SDL_SetRenderDrawColor(m_renderer, 50, 50, 50, 255);
    constexpr int step = 50;
    int w, h;
    SDL_GetRendererOutputSize(m_renderer, &w, &h);
    
    const float scaledStep = step * camera.zoom;
    const int offsetX = -static_cast<int>(static_cast<int>(camera.position.x * camera.zoom) % static_cast<int>(scaledStep));
    const int offsetY = -static_cast<int>(static_cast<int>(camera.position.y * camera.zoom) % static_cast<int>(scaledStep));
    
    for (float x = offsetX; x < w; x += scaledStep)
        SDL_RenderDrawLine(m_renderer, static_cast<int>(x), 0, static_cast<int>(x), h);
    for (float y = offsetY; y < h; y += scaledStep)
        SDL_RenderDrawLine(m_renderer, 0, static_cast<int>(y), w, static_cast<int>(y));
}

void Renderer::DrawBlocks(const std::vector<Diagram::Block>& blocks, const Diagram::Camera& camera) noexcept {
    for (const auto& block : blocks) {
        const auto screenPos = camera.WorldToScreen(block.position);
        const SDL_FRect rect = {screenPos.x, screenPos.y, block.size.x * camera.zoom, block.size.y * camera.zoom};
        
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


SDL_Renderer* Renderer::GetSDLRenderer() const noexcept {
    return m_renderer;
}