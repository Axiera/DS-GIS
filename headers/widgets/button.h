#ifndef BUTTON_H
#define BUTTON_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../isbelong.h"
#include "../../config.h"

#define BUTTON_COLOR_NORMAL         248, 247, 245, 255
#define BUTTON_COLOR_CONTAINS_MOUSE 238, 237, 235, 255
#define BUTTON_COLOR_DOWN           228, 227, 225, 255

enum {
    BUTTON_STATUS_NORMAL,
    BUTTON_STATUS_CONTAINS_NOUSE,
    BUTTON_STATUS_DOWN
};

typedef struct {
    SDL_Texture* text_texture;
    void (*on_clicked)(void* data);
    void* data;
    Uint8 status;
} button_t;

button_t* button_init(const char* text,
                      SDL_Renderer* renderer,
                      void (*on_clicked)(void* data),
                      void* data);
void button_deinit(button_t* button);
void button_draw(const button_t* button, SDL_Renderer* renderer, int x, int y);
void button_handle_event(button_t* button,
                         const SDL_Event* event,
                         int x,
                         int y);

/*s
    SDL ttf must be initialized

    button_init()
        returns pointer to button_t on success
        returns NULL on error, call SDL_GetError() for more information
*/

#endif
