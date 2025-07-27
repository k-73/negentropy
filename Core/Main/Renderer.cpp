#include "Renderer.hpp"
#include "../Diagram/Block.hpp"
#include "../Diagram/Camera.hpp"
#include "../Diagram/Grid.hpp"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <cmath>

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
    grid.Render(m_renderer, camera);
}

void Renderer::DrawBlocks(const std::vector<Diagram::Block>& blocks, const Diagram::Camera& camera) const noexcept {
    int w, h;
    SDL_GetRendererOutputSize(m_renderer, &w, &h);
    const glm::vec2 screenSize{static_cast<float>(w), static_cast<float>(h)};
    
    for (const auto& block : blocks) {
        const auto screenPos = camera.WorldToScreen(block.data.position, screenSize);
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