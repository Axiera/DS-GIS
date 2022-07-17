#ifndef EDITLINE_H
#define EDITLINE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>

#include "../list.h"
#include "../isbelong.h"

#define EDITLINE_TEXT_COLOR            0,   0,   0, 255
#define EDITLINE_HINT_COLOR          171, 168, 165, 255
#define EDITLINE_BORDER_COLOR        171, 168, 165, 255
#define EDITLINE_ACTIVE_BORDER_COLOR  58, 124, 166, 255

#define EDITLINE_HORIZONTAL_INDENT 6
#define EDITLINE_VERTICAL_INDENT 3
#define EDITLINE_HEIGHT 24
#define FONT_PATH "C:/Windows/Fonts/Arial.ttf"

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
                   int width);
void editline_handle_event(editline_t* editline,
                           const SDL_Event* event,
                           SDL_Renderer* renderer,
                           int x,
                           int y,
                           int width);
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
