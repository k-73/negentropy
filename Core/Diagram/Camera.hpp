#pragma once

#include <SDL.h>

namespace Diagram {

    struct Camera {
        float x = 0, y = 0;
        bool panning = false;
        SDL_Point panStart{};
        SDL_Point mouseStart{};
    };

}
