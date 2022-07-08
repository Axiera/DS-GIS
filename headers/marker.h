#ifndef MARKER_H
#define MARKER_H

#include <SDL2/SDL.h>
#include "list.h"
#include "isbelong.h"
#define MARKER_PIXEL_SIZE 15 /* odd number */

typedef struct {
    Uint32 x, y;
} marker_t;

const Uint8* marker_get_pixels(void);
void marker_cut(list_t* dstlist, const list_t* srclist, const SDL_Rect* area);

/*
    marker_cut()
        dstlist - list of marker_t
        srclist - list of pointers to marker_t
*/

#endif
