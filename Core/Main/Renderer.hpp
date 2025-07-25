#pragma once

#include <SDL.h>
#include <vector>

namespace Diagram {
    struct Block;
    struct Camera;
}

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    [[nodiscard]] bool Initialize(SDL_Window* window) noexcept;
    void Clear() noexcept;
    void DrawGrid(const Diagram::Camera& camera) noexcept;
    void DrawBlocks(const std::vector<Diagram::Block>& blocks, const Diagram::Camera& camera) noexcept;
    void Present() noexcept;
    void Cleanup() noexcept;
    
    [[nodiscard]] SDL_Renderer* GetSDLRenderer() const noexcept;

private:
    SDL_Renderer* m_renderer = nullptr;
};
