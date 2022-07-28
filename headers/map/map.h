#ifndef MAP_H
#define MAP_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_Image.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h> /* sprintf only */
#include <string.h>

#include "../../config.h"
#include "../http.h"
#include "../isbelong.h"
#include "../list.h"
#include "../widgets/colorpicker.h"
#include "../widgets/textarea.h"
#include "marker.h"
#include "panel.h"

#define MAP_GRID_SIZE 9 /* odd number */
#define MAP_TILE_SIZE 256
#define MAP_MIN_ZOOM 0
#define MAP_MAX_ZOOM 19

typedef struct { Uint32 x, y;     } pix_pos_t;
typedef struct { double lat, lon; } geo_pos_t;

typedef struct {
    Uint32 MAP_TILE_LOADED_EVENT;
    Uint32 x, y, size;
    Uint8 zoom;
} tile_t;

typedef struct {
    SDL_Texture* grid[MAP_GRID_SIZE][MAP_GRID_SIZE];
    Sint8 grid_loading_status[MAP_GRID_SIZE][MAP_GRID_SIZE];
    list_t marker_grid[MAP_GRID_SIZE][MAP_GRID_SIZE];
    list_t markers;
    SDL_Renderer* renderer;
    panel_t* panel;
    textarea_t* marker_name_hover;
    unsigned int is_loaded : 1;
    pix_pos_t center;
    tile_t center_tile;
} map_t;

map_t* map_init(SDL_Renderer* renderer, geo_pos_t map_center, Uint8 zoom);
void map_deinit(map_t* map);
void map_draw(const map_t* map, SDL_Rect area);
void map_handle_event(map_t* map,
                      const SDL_Event* event,
                      SDL_Renderer* renderer,
                      SDL_Rect area);

/*
    SDL, SDL Image (JPG), http must be initialized

    map_t
        marker_grid - 2d array of lists of pointers to marker_t
        markers - list of marker_t

    map_init()
        returns pointer to map_t on success
        returns NULL on error, call SDL_GetError() for more information
*/

#endif
