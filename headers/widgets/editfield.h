#ifndef EDITFIELD_H
#define EDITFIELD_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../list.h"
#include "../isbelong.h"
#include "../../config.h"

#define EDITFIELD_HORIZONTAL_INDENT 6
#define EDITFIELD_VERTICAL_INDENT 4

typedef struct {
    TTF_Font* font;
    SDL_Texture* hint_texture;
    SDL_Texture* text_texture;
    list_t text;
    list_t text_characters_sizes;
    Uint32 hint_texture_width;
    Uint32 hint_texture_height;
    Uint32 text_texture_width;
    Uint32 text_texture_height;
    Uint16 max_text_char_size;
    Uint8 active;
} editfield_t;

editfield_t* editfield_init(const char* hint_text,
                            SDL_Renderer* renderer,
                            Uint16 max_text_char_size,
                            int width);
void editfield_deinit(editfield_t* editfield);
void editfield_draw(const editfield_t* editfield,
                    SDL_Renderer* renderer,
                    int x,
                    int y,
                    int w,
                    int h);
void editfield_handle_event(editfield_t* editfield,
                            const SDL_Event* event,
                            SDL_Renderer* renderer,
                            int x,
                            int y,
                            int w,
                            int h);
const char* editfield_get_text(const editfield_t* editfield);

/*
    SDL ttf must be initialized

    editfield_t
        text - list of char
        text_characters_sizes - list of Uint32

    editfield_init()
        returns pointer to editfield_t on success
        returns NULL on error, call SDL_GetError() for more information
*/

#endif
