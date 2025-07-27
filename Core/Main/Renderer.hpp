#pragma once

#include <SDL.h>
#include <vector>
#include <memory>
#include <type_traits>
#include <glm/vec2.hpp>

namespace Diagram {
    struct Block;
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
    
    template<typename ComponentContainer>
    void DrawComponents(const ComponentContainer& components, const Diagram::Camera& camera) const noexcept {
        int w, h;
        SDL_GetRendererOutputSize(m_renderer, &w, &h);
        const glm::vec2 screenSize{static_cast<float>(w), static_cast<float>(h)};
        
        auto getComponent = []<typename T0>(T0& item) -> const Diagram::Component* {
            if constexpr (std::is_pointer_v<std::decay_t<T0>>) return item;
            else if constexpr (std::is_same_v<std::decay_t<T0>, std::unique_ptr<Diagram::Component>>) return item.get();
            else return &item;
        };
        
        for (const auto& item : components) {
            if (auto* block = dynamic_cast<const Diagram::Block*>(getComponent(item))) {
                block->Render(m_renderer, camera, screenSize);
            }
        }
    }
    
    void Present() const noexcept;

    SDL_Renderer* GetSDLRenderer() const noexcept;

private:
    SDL_Renderer* m_renderer = nullptr;
};
