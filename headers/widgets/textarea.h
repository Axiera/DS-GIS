#ifndef TEXTAREA_H
#define TEXTAREA_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../../config.h"

typedef struct {
    SDL_Texture* text_texture;
    TTF_Font* font;
    int w, h;
} textarea_t;

textarea_t* textarea_init(void);
void textarea_deinit(textarea_t* textarea);
void textarea_draw(const textarea_t* textarea,
                   SDL_Renderer* renderer,
                   int x,
                   int y,
                   int h);
void textarea_set_text(textarea_t* textarea,
                       SDL_Renderer* renderer,
                       const char* text,
                       int w);

/*
    SDL ttf must be initialized

    textarea_init()
        returns pointer to textarea_t on success
        returns NULL on error, call SDL_GetError() for more information
*/

#endif
