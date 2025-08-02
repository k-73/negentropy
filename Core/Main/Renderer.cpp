#include "Renderer.hpp"
#include <imgui.h>

bool Renderer::Initialize(SDL_Window* window) noexcept {
	const int AUTO_INDEX = -1;
	rendererPtr = SDL_CreateRenderer(window, AUTO_INDEX, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if(!rendererPtr) {
		rendererPtr = SDL_CreateRenderer(window, AUTO_INDEX, SDL_RENDERER_SOFTWARE);
		if(!rendererPtr) return false;
	}
	SDL_SetRenderDrawBlendMode(rendererPtr, SDL_BLENDMODE_BLEND);
	return true;
}

void Renderer::Clear() const noexcept {
	SDL_SetRenderDrawColor(rendererPtr, 30, 30, 30, 255);
	SDL_RenderClear(rendererPtr);
}

void Renderer::Present() const noexcept {
	SDL_RenderPresent(rendererPtr);
}

SDL_Renderer* Renderer::GetSDLRenderer() const noexcept {
	return rendererPtr;
}