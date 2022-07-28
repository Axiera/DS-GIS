#ifndef PANELS_H
#define PANELS_H

#include <SDL2/SDL.h>
#include <windows.h>

#include "../widgets/editline.h"
#include "../widgets/editfield.h"
#include "../widgets/button.h"
#include "../widgets/colorpicker.h"

#define PANEL_INDENT 10
#define PANEL_EDITFIELD_HEIGHT 160

enum { PANEL_CREATE_MARKER };

typedef struct {
    Sint16 type;
    Sint16 canceled;
    void* map;
    void (*on_executed)(void*);
    const char* (*check)(void*);
} panel_parameters_t;

typedef struct {
    panel_parameters_t parameters;
    editline_t* editline;
    editfield_t* editfield;
    button_t* button_cancel;
    button_t* button_create;
    colorpicker_t colorpicker;
} panel_create_marker;

typedef union {
    panel_parameters_t parameters;
    panel_create_marker create_marker;
} panel_t;

panel_t* panel_init(int type,
                    SDL_Renderer* renderer,
                    void* map,
                    void (*on_executed)(void*),
                    const char* (*check)(void*));
void panel_deinit(panel_t* panel);
void panel_draw(const panel_t* panel,
                SDL_Renderer* renderer,
                int x,
                int y,
                int h);
void panel_handle_event(panel_t* panel,
                        const SDL_Event* event,
                        SDL_Renderer* renderer,
                        int x,
                        int y,
                        int h);

/*
    SDL ttf must be initialized

    panel_init()
        returns pointer to panel_t on success
        returns NULL on error, call SDL_GetError() for more information
*/

#endif
