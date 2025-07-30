#pragma once

#include <SDL.h>

#include <glm/vec2.hpp>
#include <memory>
#include <vector>

#include "../Diagram/Component.hpp"

namespace Diagram
{
	struct Camera;
	struct Grid;
}

class Renderer
{
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

	void DrawComponents(const std::vector<std::unique_ptr<Diagram::ComponentBase>>& componentList, const Diagram::Camera& camera) const noexcept {
		int rendererWidth, rendererHeight;
		SDL_GetRendererOutputSize(rendererPtr, &rendererWidth, &rendererHeight);
		const glm::vec2 screenSize {static_cast<float>(rendererWidth), static_cast<float>(rendererHeight)};

		for(const auto& item: componentList) {
			item->Render(rendererPtr, camera, screenSize);
		}
	}

	void Present() const noexcept;

	SDL_Renderer* GetSDLRenderer() const noexcept;

private:
	SDL_Renderer* rendererPtr = nullptr;
};
