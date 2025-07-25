#include <SDL.h>
#include <vector>
#include <string>
#include <cstdio>
#include <algorithm>

struct Block {
    SDL_FRect rect;
    std::string label;
    bool dragging = false;
    SDL_FPoint dragOffset{};
};

struct Camera {
    float x = 0, y = 0;    // przesunięcie świata
    bool panning = false;
    SDL_Point panStart{}, mouseStart{};
};

static bool pointIn(const SDL_FRect& r, float x, float y) {
    return x >= r.x && y >= r.y && x <= r.x + r.w && y <= r.y + r.h;
}

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Diagram (ugui demo)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!win || !ren) return 2;

    std::vector<Block> blocks = {
        {{100,100,120,60},"Start"},
        {{320,180,140,60},"Process"},
        {{560,120,120,60},"End"}
    };

    Camera cam;
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;

            if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_MIDDLE) {
                    cam.panning = true;
                    cam.panStart = { (int)cam.x, (int)cam.y };
                    cam.mouseStart = { e.button.x, e.button.y };
                }
                if (e.button.button == SDL_BUTTON_LEFT) {
                    float wx = e.button.x + cam.x;
                    float wy = e.button.y + cam.y;

                    for (int i = (int)blocks.size()-1; i >= 0; --i) {
                        auto& b = blocks[i];
                        if (pointIn(b.rect, wx, wy)) {
                            b.dragging = true;
                            b.dragOffset = { wx - b.rect.x, wy - b.rect.y };

                            std::rotate(blocks.begin()+i, blocks.begin()+i+1, blocks.end());
                            break;
                        }
                    }
                }
            }
            if (e.type == SDL_MOUSEBUTTONUP) {
                if (e.button.button == SDL_BUTTON_MIDDLE) cam.panning = false;
                if (e.button.button == SDL_BUTTON_LEFT)
                    for (auto& b: blocks) b.dragging = false;
            }
            if (e.type == SDL_MOUSEMOTION) {
                if (cam.panning) {
                    cam.x = cam.panStart.x - (e.motion.x - cam.mouseStart.x);
                    cam.y = cam.panStart.y - (e.motion.y - cam.mouseStart.y);
                }
                float wx = e.motion.x + cam.x;
                float wy = e.motion.y + cam.y;
                for (auto& b: blocks) if (b.dragging) {
                    b.rect.x = wx - b.dragOffset.x;
                    b.rect.y = wy - b.dragOffset.y;
                }
            }
            if (e.type == SDL_MOUSEWHEEL) {

            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) running = false;
        }

        SDL_SetRenderDrawColor(ren, 30,30,30,255);
        SDL_RenderClear(ren);

        SDL_SetRenderDrawColor(ren, 50,50,50,255);
        const int step = 50;
        int w, h; SDL_GetRendererOutputSize(ren, &w, &h);
        for (int x = -((int)cam.x % step); x < w; x += step)
            SDL_RenderDrawLine(ren, x, 0, x, h);
        for (int y = -((int)cam.y % step); y < h; y += step)
            SDL_RenderDrawLine(ren, 0, y, w, y);

        for (auto& b: blocks) {
            SDL_FRect screen = { b.rect.x - cam.x, b.rect.y - cam.y, b.rect.w, b.rect.h };
            SDL_SetRenderDrawColor(ren, 90,120,200,255);
            SDL_RenderFillRectF(ren, &screen);
            SDL_SetRenderDrawColor(ren, 255,255,255,255);
            SDL_RenderDrawRectF(ren, &screen);
        }

        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
