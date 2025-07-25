#pragma once

#include <SDL.h>
#include <string>

namespace Diagram {
    struct Block {
        SDL_FRect rect;
        std::string label;
        bool dragging = false;
        SDL_FPoint dragOffset{};
    };


    inline bool PointInRect(const SDL_FRect& r, float x, float y) {
        return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
    }

}
