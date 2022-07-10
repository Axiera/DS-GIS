#ifndef BUTTON_H
#define BUTTON_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "../isbelong.h"

#define BUTTON_COLOR_NORMAL         248, 247, 245, 255
#define BUTTON_COLOR_CONTAINS_MOUSE 238, 237, 235, 255
#define BUTTON_COLOR_DOWN           228, 227, 225, 255
#define BUTTON_BORDER_COLOR         171, 168, 165, 255
#define BUTTON_TEXT_COLOR            31,  28,  25, 255
#define FONT_PATH "C:/Windows/Fonts/Arial.ttf"

enum {
    BUTTON_STATUS_NORMAL,
    BUTTON_STATUS_CONTAINS_NOUSE,
    BUTTON_STATUS_DOWN
};

typedef struct {
    SDL_Texture* text_texture;
    void (*on_clicked)(void* data);
    void* data;
    const SDL_Rect* area;
    Uint8 status;
} button_t;

button_t* button_init(const char* text,
                      void (*on_clicked)(void* data),
                      void* data,
                      const SDL_Rect* area,
                      SDL_Renderer* renderer);
void button_deinit(button_t* button);
void button_draw(const button_t* button, SDL_Renderer* renderer);
void button_handle_event(button_t* button, const SDL_Event* event);

/*
    SDL ttf must be initialized

    button_init()
        returns pointer to button_t on success
        returns NULL on error, call SDL_GetError() for more information
*/

#endif
