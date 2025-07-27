#pragma once

#include <SDL.h>
#include <glm/vec2.hpp>

namespace Diagram {
    struct Component;
    struct Camera;
    struct Grid;
}

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    bool Initialize(SDL_Window* window) noexcept;
    void Clear() const noexcept;
    void DrawGrid(const Diagram::Camera& camera, const Diagram::Grid& grid) const noexcept;
    
    template<typename C>
    void DrawComponents(const C& components, const Diagram::Camera& camera) const noexcept {
        int w, h; SDL_GetRendererOutputSize(m_renderer, &w, &h);
        const glm::vec2 screenSize{static_cast<float>(w), static_cast<float>(h)};
        for (const auto& item : components) {
            item.get()->Render(m_renderer, camera, screenSize);
        }
    }
    
    void Present() const noexcept;

    SDL_Renderer* GetSDLRenderer() const noexcept;

private:
    SDL_Renderer* m_renderer = nullptr;
};
