#pragma once

#include <SDL.h>
#include <memory>
#include <type_traits>
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
        auto get = []<typename T>(T& x) -> const Diagram::Component* {
            if constexpr (std::is_pointer_v<std::decay_t<T>>) return x;
            else if constexpr (std::is_same_v<std::decay_t<T>, std::unique_ptr<Diagram::Component>>) return x.get();
            else return &x;
        };
        for (const auto& item : components)
            get(item)->Render(m_renderer, camera, {float(w), float(h)});
    }
    
    void Present() const noexcept;

    SDL_Renderer* GetSDLRenderer() const noexcept;

private:
    SDL_Renderer* m_renderer = nullptr;
};
