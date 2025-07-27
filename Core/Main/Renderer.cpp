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
    if (!grid.settings.visible) return;
    
    int w, h;
    SDL_GetRendererOutputSize(m_renderer, &w, &h);
    const glm::vec2 screenSize{static_cast<float>(w), static_cast<float>(h)};
    
    // Znajdź widoczny obszar world space
    const glm::vec2 topLeft = camera.ScreenToWorld({0.0f, 0.0f}, screenSize);
    const glm::vec2 bottomRight = camera.ScreenToWorld(screenSize, screenSize);
    
    // Znajdź zakres linii grid w world space
    const float firstGridX = std::floor(topLeft.x / grid.settings.smallStep) * grid.settings.smallStep;
    const float lastGridX = std::ceil(bottomRight.x / grid.settings.smallStep) * grid.settings.smallStep;
    const float firstGridY = std::floor(topLeft.y / grid.settings.smallStep) * grid.settings.smallStep;
    const float lastGridY = std::ceil(bottomRight.y / grid.settings.smallStep) * grid.settings.smallStep;
    
    SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 255);
    
    // Rysuj pionowe linie - każda w world space, potem WorldToScreen jak bloki
    for (float worldX = firstGridX; worldX <= lastGridX; worldX += grid.settings.smallStep) {
        const glm::vec2 topPoint = camera.WorldToScreen({worldX, topLeft.y}, screenSize);
        const glm::vec2 bottomPoint = camera.WorldToScreen({worldX, bottomRight.y}, screenSize);
        SDL_RenderDrawLineF(m_renderer, topPoint.x, topPoint.y, bottomPoint.x, bottomPoint.y);
    }
    
    // Rysuj poziome linie - każda w world space, potem WorldToScreen jak bloki  
    for (float worldY = firstGridY; worldY <= lastGridY; worldY += grid.settings.smallStep) {
        const glm::vec2 leftPoint = camera.WorldToScreen({topLeft.x, worldY}, screenSize);
        const glm::vec2 rightPoint = camera.WorldToScreen({bottomRight.x, worldY}, screenSize);
        SDL_RenderDrawLineF(m_renderer, leftPoint.x, leftPoint.y, rightPoint.x, rightPoint.y);
    }
    
    // Rysuj grube linie dla large step
    const int largeStepMultiplier = static_cast<int>(std::round(grid.settings.largeStep / grid.settings.smallStep));
    SDL_SetRenderDrawColor(m_renderer, 60, 60, 60, 255);
    
    // Pionowe grube linie
    for (float worldX = firstGridX; worldX <= lastGridX; worldX += grid.settings.smallStep) {
        const int gridIndex = static_cast<int>(std::round(worldX / grid.settings.smallStep));
        if (gridIndex % largeStepMultiplier == 0) {
            const glm::vec2 topPoint = camera.WorldToScreen({worldX, topLeft.y}, screenSize);
            const glm::vec2 bottomPoint = camera.WorldToScreen({worldX, bottomRight.y}, screenSize);
            SDL_RenderDrawLineF(m_renderer, topPoint.x, topPoint.y, bottomPoint.x, bottomPoint.y);
        }
    }
    
    // Poziome grube linie
    for (float worldY = firstGridY; worldY <= lastGridY; worldY += grid.settings.smallStep) {
        const int gridIndex = static_cast<int>(std::round(worldY / grid.settings.smallStep));
        if (gridIndex % largeStepMultiplier == 0) {
            const glm::vec2 leftPoint = camera.WorldToScreen({topLeft.x, worldY}, screenSize);
            const glm::vec2 rightPoint = camera.WorldToScreen({bottomRight.x, worldY}, screenSize);
            SDL_RenderDrawLineF(m_renderer, leftPoint.x, leftPoint.y, rightPoint.x, rightPoint.y);
        }
    }
    
    // Draw white cross marker at grid center (world coordinates -5 to 5 in x,y)
    
    const glm::vec2 gridCenterWorld{0.0f, 0.0f}; // World center
    const glm::vec2 crossMinWorld{-5.0f, -5.0f};
    const glm::vec2 crossMaxWorld{5.0f, 5.0f};
    
    const glm::vec2 centerScreen = camera.WorldToScreen(gridCenterWorld, screenSize);
    const glm::vec2 crossMinScreen = camera.WorldToScreen(crossMinWorld, screenSize);
    const glm::vec2 crossMaxScreen = camera.WorldToScreen(crossMaxWorld, screenSize);
    
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255); // White color
    
    // Horizontal line from -5 to +5 in world x
    SDL_RenderDrawLineF(m_renderer, crossMinScreen.x, centerScreen.y, crossMaxScreen.x, centerScreen.y);
    
    // Vertical line from -5 to +5 in world y  
    SDL_RenderDrawLineF(m_renderer, centerScreen.x, crossMinScreen.y, centerScreen.x, crossMaxScreen.y);
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