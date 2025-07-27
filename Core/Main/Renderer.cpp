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
        block.Render(m_renderer, camera, screenSize);
    }
}

void Renderer::Present() const noexcept {
    SDL_RenderPresent(m_renderer);
}

SDL_Renderer* Renderer::GetSDLRenderer() const noexcept {
    return m_renderer;
}