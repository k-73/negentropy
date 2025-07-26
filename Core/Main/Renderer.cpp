#include "Renderer.hpp"
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"
#include "../Diagram/Grid.hpp"
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

void Renderer::Clear() const noexcept {
    SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 255);
    SDL_RenderClear(m_renderer);
}

void Renderer::DrawGrid(const Diagram::Camera& camera, const Diagram::Grid& grid) const noexcept {
    if (!grid.settings.visible) return;
    
    int w, h;
    SDL_GetRendererOutputSize(m_renderer, &w, &h);
    
    const auto params = grid.CalculateRenderParams(camera, w, h);
    
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 255);
    for (float x = params.baseOffsetX; x < w; x += params.smallScaled)
        SDL_RenderDrawLine(m_renderer, static_cast<int>(x), 0, static_cast<int>(x), h);
    for (float y = params.baseOffsetY; y < h; y += params.smallScaled)
        SDL_RenderDrawLine(m_renderer, 0, static_cast<int>(y), w, static_cast<int>(y));
    
    SDL_SetRenderDrawColor(m_renderer, 60, 60, 60, 255);
    int lineIndex = 0;
    for (float x = params.baseOffsetX; x < w; x += params.smallScaled, ++lineIndex) {
        if (lineIndex % params.largeStepMultiplier == 0)
            SDL_RenderDrawLine(m_renderer, static_cast<int>(x), 0, static_cast<int>(x), h);
    }
    lineIndex = 0;
    for (float y = params.baseOffsetY; y < h; y += params.smallScaled, ++lineIndex) {
        if (lineIndex % params.largeStepMultiplier == 0)
            SDL_RenderDrawLine(m_renderer, 0, static_cast<int>(y), w, static_cast<int>(y));
    }
}

void Renderer::DrawBlocks(const std::vector<Diagram::Block>& blocks, const Diagram::Camera& camera) const noexcept {
    for (const auto& block : blocks) {
        const auto screenPos = camera.WorldToScreen(block.data.position);
        const SDL_FRect rect = {screenPos.x, screenPos.y, block.data.size.x * camera.data.zoom, block.data.size.y * camera.data.zoom};
        
        const auto r = static_cast<Uint8>(block.data.backgroundColor.r * 255.0f);
        const auto g = static_cast<Uint8>(block.data.backgroundColor.g * 255.0f);
        const auto b = static_cast<Uint8>(block.data.backgroundColor.b * 255.0f);
        const auto a = static_cast<Uint8>(block.data.backgroundColor.a * 255.0f);
        
        SDL_SetRenderDrawColor(m_renderer, r, g, b, a);
        SDL_RenderFillRectF(m_renderer, &rect);
        
        const auto borderR = static_cast<Uint8>(block.data.borderColor.r * 255.0f);
        const auto borderG = static_cast<Uint8>(block.data.borderColor.g * 255.0f);
        const auto borderB = static_cast<Uint8>(block.data.borderColor.b * 255.0f);
        const auto borderA = static_cast<Uint8>(block.data.borderColor.a * 255.0f);
        
        SDL_SetRenderDrawColor(m_renderer, borderR, borderG, borderB, borderA);
        SDL_RenderDrawRectF(m_renderer, &rect);
    }
}

void Renderer::Present() const noexcept {
    SDL_RenderPresent(m_renderer);
}


SDL_Renderer* Renderer::GetSDLRenderer() const noexcept {
    return m_renderer;
}