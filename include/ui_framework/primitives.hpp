#pragma once

#include <SDL3/SDL.h>

namespace primitives
{
    bool rectangleRGBA(SDL_Renderer *renderer, float x1, float y1, float x2, float y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    bool roundedRectangleRGBA(SDL_Renderer *renderer, float x1, float y1, float x2, float y2, float rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    bool boxRGBA(SDL_Renderer *renderer, float x1, float y1, float x2, float y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    bool roundedBoxRGBA(SDL_Renderer *renderer, float x1, float y1, float x2, float y2, float rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    bool lineRGBA(SDL_Renderer *renderer, float x1, float y1, float x2, float y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    bool vlineRGBA(SDL_Renderer *renderer, float x, float y1, float y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    bool hlineRGBA(SDL_Renderer *renderer, float x1, float x2, float y, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    bool pixelRGBA(SDL_Renderer *renderer, float x, float y, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    bool arcRGBA(SDL_Renderer *renderer, float x, float y, float rad, Sint16 start, Sint16 end, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    bool filledCircleRGBA(SDL_Renderer *renderer, float x, float y, float rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
}
