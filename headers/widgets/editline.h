#ifndef EDITLINE_H
#define EDITLINE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>

#include "../list.h"
#include "../isbelong.h"
#include "../../config.h"

#define EDITLINE_HORIZONTAL_INDENT 6
#define EDITLINE_VERTICAL_INDENT 3

typedef struct {
    TTF_Font* font;
    SDL_Texture* hint_texture;
    SDL_Texture* text_texture;
    list_t text;
    list_t text_characters_sizes;
    Uint32 hint_texture_width;
    Uint32 text_texture_width;
    Uint32 text_texture_height;
    Uint8 active;
} editline_t;

editline_t* editline_init(const char* hint_text, SDL_Renderer* renderer);
void editline_deinit(editline_t* editline);
void editline_draw(const editline_t* editline,
                   SDL_Renderer* renderer,
                   int x,
                   int y,
                   int w);
void editline_handle_event(editline_t* editline,
                           const SDL_Event* event,
                           SDL_Renderer* renderer,
                           int x,
                           int y,
                           int w);
const char* editline_get_text(const editline_t* editline);

/*
    SDL ttf must be initialized

    editline_t
        text - list of char
        text_characters_sizes - list of Uint32

    editline_init()
        returns pointer to editline_t on success
        returns NULL on error, call SDL_GetError() for more information
*/

#endif
