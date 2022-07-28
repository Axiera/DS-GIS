#ifndef MARKER_H
#define MARKER_H

#include <SDL2/SDL.h>
#include "../list.h"
#include "../isbelong.h"
#define MARKER_PIXEL_SIZE 15 /* odd number */

typedef struct {
    char* name;
    char* description;
    Uint32 x, y;
    Uint32 color              :  3;
    Uint32 name_length        :  7;
    Uint32 description_length : 11;
} marker_t;

const Uint8* marker_get_pixels(void);
const Uint8* marker_get_pixels_hovered(void);
void marker_cut(list_t* dstlist, const list_t* srclist, const SDL_Rect* area);

/*
    marker_cut()
        dstlist - list of marker_t
        srclist - list of pointers to marker_t
*/

#endif
