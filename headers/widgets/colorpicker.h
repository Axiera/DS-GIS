#ifndef COLORPICKER_H
#define COLORPICKER_H

#include <SDL2/SDL.h>
#include "../isbelong.h"
#include "../../config.h"

#define COLORPICKER_COLOR_COUNT 8
#define COLORPICKER_INDENT 2

typedef struct {
    Uint8 color : 3;
} colorpicker_t;

void colorpicker_draw(const colorpicker_t* colorpicker,
                      SDL_Renderer* renderer,
                      int x,
                      int y,
                      int w);
void colorpicker_handle_event(colorpicker_t* colorpicker,
                              const SDL_Event* event,
                              int x,
                              int y,
                              int w);
SDL_Color colorpicker_get_color(const colorpicker_t* colorpicker);

#endif
