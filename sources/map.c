#include "../headers/map.h"

static pix_pos_t to_pix(geo_pos_t geo_pos);
static void start_tile_loading(map_t* map);
static int load_tile_async(void* ptr_tile); /* SDL_ThreadFunction */
static size_t count_digits(Uint32 number);
static char* generate_request_path(const tile_t* tile);
static _Bool is_belong(int x, int y, const SDL_Rect* rect);
static void shift_map_grid_data(map_t* map, Sint8 shift_x, Sint8 shift_y);

/* ---------------------- header functions definition ---------------------- */

map_t* map_init(SDL_Renderer* renderer, geo_pos_t map_center, Uint8 zoom) {
    map_t* map = malloc(sizeof(map_t));
    if (map == NULL) {
        SDL_SetError("memory allocation failed\n%s()", __func__);
        return NULL;
    }

    for (int i = 0; i < MAP_GRID_SIZE; i++) {
        for (int j = 0; j < MAP_GRID_SIZE; j++) {
            map->grid[i][j] = NULL;
            map->grid_loading_status[i][j] = 0;
        }
    }
    map->renderer = renderer;
    map->is_loaded = 0;
    map->center = to_pix(map_center);
    map->center_tile.MAP_TILE_LOADED_EVENT = SDL_RegisterEvents(1);
    if (map->center_tile.MAP_TILE_LOADED_EVENT == (Uint32)-1) {
        SDL_SetError("event registration failed\n%s()", __func__);
        map_deinit(map);
        return NULL;
    }
    map->center_tile.size = MAP_TILE_SIZE * (1 << MAP_MAX_ZOOM-zoom);
    map->center_tile.x = map->center.x / map->center_tile.size;
    map->center_tile.y = map->center.y / map->center_tile.size;
    map->center_tile.zoom = zoom;

    start_tile_loading(map);

    return map;
}

void map_deinit(map_t* map) {
    for (int i = 0; i < MAP_GRID_SIZE; i++) {
        for (int j = 0; j < MAP_GRID_SIZE; j++)
            SDL_DestroyTexture(map->grid[i][j]);
    }
    free(map);
}

void map_draw(const map_t* map, const SDL_Rect* area) {
    pix_pos_t grid_begin = {
        .x = (map->center_tile.x - MAP_GRID_SIZE/2) * map->center_tile.size,
        .y = (map->center_tile.y - MAP_GRID_SIZE/2) * map->center_tile.size
    };
    Uint32 scale = map->center_tile.size / MAP_TILE_SIZE;
    Sint32 begin_x = area->x + area->w/2 - (map->center.x-grid_begin.x)/scale;
    Sint32 begin_y = area->y + area->h/2 - (map->center.y-grid_begin.y)/scale;

    for (int i = 0; i < MAP_GRID_SIZE; i++) {
        for (int j = 0; j < MAP_GRID_SIZE; j++) {
            if (!map->grid_loading_status[i][j])
                continue;

            Sint32 x = begin_x + j*MAP_TILE_SIZE;
            Sint32 y = begin_y + i*MAP_TILE_SIZE;
            if (x + MAP_TILE_SIZE < area->x || x >= area->x + area->w)
                continue;
            if (y + MAP_TILE_SIZE < area->y || y >= area->y + area->h)
                continue;

            SDL_Rect srcrect = {
                .x = 0,
                .y = 0,
                .w = MAP_TILE_SIZE,
                .h = MAP_TILE_SIZE
            };

            if (x < area->x)
                srcrect.x += area->x - x;
            if (x + MAP_TILE_SIZE >= area->x + area->w)
                srcrect.w -= (x + MAP_TILE_SIZE) - (area->x + area->w);
            if (y < area->y)
                srcrect.y += area->y - y;
            if (y + MAP_TILE_SIZE >= area->y + area->h)
                srcrect.h -= (y + MAP_TILE_SIZE) - (area->y + area->h);

            SDL_Rect dstrect = {
                .x = x + srcrect.x,
                .y = y + srcrect.y,
                .w = srcrect.w - srcrect.x,
                .h = srcrect.h - srcrect.y
            };

            SDL_RenderCopy(map->renderer, map->grid[i][j], &srcrect, &dstrect);
        }
    }
}

void map_handle_event(map_t* map,
                      const SDL_Event* event,
                      const SDL_Rect* area) {
    if (event->type == map->center_tile.MAP_TILE_LOADED_EVENT) {
        tile_t* tile = event->user.data1;
        SDL_Surface* surface = event->user.data2;

        int i = tile->y - (map->center_tile.y - MAP_GRID_SIZE/2);
        int j = tile->x - (map->center_tile.x - MAP_GRID_SIZE/2);
        SDL_Rect grid = { 0, 0, MAP_GRID_SIZE, MAP_GRID_SIZE };

        if (is_belong(i, j, &grid) && map->center_tile.zoom == tile->zoom) {
            map->grid_loading_status[i][j] = 1;
            SDL_Texture* texture = NULL;
            if (surface != NULL)
                texture = SDL_CreateTextureFromSurface(map->renderer, surface);
            map->grid[i][j] = texture;
        }

        free(tile);
        SDL_FreeSurface(surface);
        if (!map->is_loaded)
            start_tile_loading(map);
    }

    else if (event->type == SDL_MOUSEMOTION) {
        if (!is_belong(event->motion.x, event->motion.y, area))
            return;
        if (event->motion.state & SDL_BUTTON_LMASK) {
            Uint32 scale = map->center_tile.size / MAP_TILE_SIZE;
            map->center.x -= event->motion.xrel * scale;
            map->center.y -= event->motion.yrel * scale;

            Uint32 tile_x = map->center.x / map->center_tile.size;
            Uint32 tile_y = map->center.y / map->center_tile.size;
            shift_map_grid_data(
                map,
                map->center_tile.x - tile_x,
                map->center_tile.y - tile_y
            );
            map->center_tile.x = tile_x;
            map->center_tile.y = tile_y;
        }
    }

    else if (event->type == SDL_MOUSEWHEEL) {
        Uint8 zoom = map->center_tile.zoom + event->wheel.y;
        if (zoom < MAP_MIN_ZOOM || zoom > MAP_MAX_ZOOM)
            return;

        map->center_tile.size = MAP_TILE_SIZE * (1 << MAP_MAX_ZOOM-zoom);
        map->center_tile.x = map->center.x / map->center_tile.size;
        map->center_tile.y = map->center.y / map->center_tile.size;
        map->center_tile.zoom = zoom;

        for (int i = 0; i < MAP_GRID_SIZE; i++) {
            for (int j = 0; j < MAP_GRID_SIZE; j++) {
                SDL_DestroyTexture(map->grid[i][j]);
                map->grid_loading_status[i][j] = 0;
            }
        }

        if (!map->is_loaded)
            return;
        map->is_loaded = 0;
        start_tile_loading(map);
    }
}

/* ---------------------- static functions definition ---------------------- */

static pix_pos_t to_pix(geo_pos_t geo_pos) {
    double lat_radian = geo_pos.lat * M_PI/180;
    double x = (geo_pos.lon + 180)/360 * (1<<MAP_MAX_ZOOM);
    double y = (1 - asinh(tan(lat_radian))/M_PI) * (1 << MAP_MAX_ZOOM-1);
    return (pix_pos_t){ x*MAP_TILE_SIZE, y*MAP_TILE_SIZE };
}

static void start_tile_loading(map_t* map) {
    Uint8 i = MAP_GRID_SIZE/2;
    Uint8 j = i;
    Uint8 found = 0;

    if (!map->grid_loading_status[i][j])
        found = 1;

    /* spiral */
    for (Uint8 size = 3; !found && size <= MAP_GRID_SIZE; size += 2) {
        for (Uint8 k = 1; !found && k < size; k++) {
            i = (MAP_GRID_SIZE-size)/2;
            j = i + k;
            if (!map->grid_loading_status[i][j])
                found = 1;
        }

        for (Uint8 k = 1; !found && k < size; k++) {
            i = (MAP_GRID_SIZE-size)/2 + k;
            j = (MAP_GRID_SIZE-size)/2 + size - 1;
            if (!map->grid_loading_status[i][j])
                found = 1;
        }

        for (Uint8 k = size-1; !found && k > 0; k--) {
            i = (MAP_GRID_SIZE-size)/2 + size - 1;
            j = i - (size-k);
            if (!map->grid_loading_status[i][j])
                found = 1;
        }

        for (Uint8 k = size-1; !found && k > 0; k--) {
            i = (MAP_GRID_SIZE-size)/2 + k - 1;
            j = (MAP_GRID_SIZE-size)/2;
            if (!map->grid_loading_status[i][j])
                found = 1;
        }
    }

    if (!found) {
        map->is_loaded = 1;
        return;
    }

    tile_t* tile = malloc(sizeof(tile_t));
    if (tile == NULL)
        return;
    memcpy(tile, &map->center_tile, sizeof(tile_t));
    tile->x = map->center_tile.x - MAP_GRID_SIZE/2 + j;
    tile->y = map->center_tile.y - MAP_GRID_SIZE/2 + i;

    SDL_Thread* thread = SDL_CreateThread(load_tile_async, NULL, tile);
    SDL_DetachThread(thread);
}

static int load_tile_async(void* ptr_tile) {
    /* SDL_ThreadFunction */
    tile_t* tile = ptr_tile;
    SDL_Surface* surface = NULL;

    char* path = generate_request_path(tile);
    if (path != NULL) {
        response_t response = http_get("api.mapbox.com", path);
        if (response.size) {
            SDL_RWops* rw = SDL_RWFromMem(response.data, response.size);
            surface = IMG_LoadTyped_RW(rw, 0, "JPG");
            SDL_RWclose(rw);
        }
        free(response.data);
        free(path);
    }

    SDL_Event event;
    memset(&event, 0, sizeof(SDL_Event));
    event.type = tile->MAP_TILE_LOADED_EVENT;
    event.user.data1 = tile;
    event.user.data2 = surface;
    SDL_PushEvent(&event);

    return 0;
}

static size_t count_digits(Uint32 number) {
    size_t count = 0;
    for (; number; number /= 10)
        count++;
    return count;
}

static char* generate_request_path(const tile_t* tile) {
    static const char* path_base =
        "/v4/mapbox.satellite/%d/%d/%d.jpg90?access_token=%s";
    const char* token = CONFIG_MAPBOX_ACCESS_TOKEN;

    size_t path_lenght =
        strlen(path_base)-8 +
        count_digits(tile->zoom) +
        count_digits(tile->x) +
        count_digits(tile->y) +
        strlen(token);
    char* path = malloc(path_lenght+1);
    if (path != NULL)
        sprintf(path, path_base, tile->zoom, tile->x, tile->y, token);
    return path;
}

static _Bool is_belong(int x, int y, const SDL_Rect* rect) {
    return x >= rect->x && x < rect->x + rect->w
        && y >= rect->y && y < rect->y + rect->h;
}

static void shift_map_grid_data(map_t* map, Sint8 shift_x, Sint8 shift_y) {
    if (shift_x == 0 && shift_y == 0)
        return;

    if (shift_x > 0 && shift_x < MAP_GRID_SIZE) {
        for (int i = 0; i < MAP_GRID_SIZE; i++) {
            for (int j = MAP_GRID_SIZE - shift_x; j < MAP_GRID_SIZE; j++)
                SDL_DestroyTexture(map->grid[i][j]);
            for (int j = MAP_GRID_SIZE-1; j >= shift_x; j--) {
                map->grid_loading_status[i][j] =
                    map->grid_loading_status[i][j-shift_x];
                map->grid[i][j] = map->grid[i][j-shift_x];
            }
            for (int j = 0; j < shift_x; j++) {
                map->grid_loading_status[i][j] = 0;
                map->grid[i][j] = NULL;
            }
        }
    }

    else if (shift_x < 0 && shift_x > -MAP_GRID_SIZE) {
        shift_x *= -1;
        for (int i = 0; i < MAP_GRID_SIZE; i++) {
            for (int j = 0; j < shift_x; j++)
                SDL_DestroyTexture(map->grid[i][j]);
            for (int j = 0; j < MAP_GRID_SIZE - shift_x; j++) {
                map->grid_loading_status[i][j] =
                    map->grid_loading_status[i][j+shift_x];
                map->grid[i][j] = map->grid[i][j+shift_x];
            }
            for (int j = MAP_GRID_SIZE - shift_x; j < MAP_GRID_SIZE; j++) {
                map->grid_loading_status[i][j] = 0;
                map->grid[i][j] = NULL;
            }
        }
    }

    if (shift_y > 0 && shift_y < MAP_GRID_SIZE) {
        for (int j = 0; j < MAP_GRID_SIZE; j++) {
            for (int i = MAP_GRID_SIZE - shift_y; i < MAP_GRID_SIZE; i++)
                SDL_DestroyTexture(map->grid[i][j]);
            for (int i = MAP_GRID_SIZE-1; i >= shift_y; i--) {
                map->grid_loading_status[i][j] =
                    map->grid_loading_status[i-shift_y][j];
                map->grid[i][j] = map->grid[i-shift_y][j];
            }
            for (int i = 0; i < shift_y; i++) {
                map->grid_loading_status[i][j] = 0;
                map->grid[i][j] = NULL;
            }
        }
    }

    else if (shift_y < 0 && shift_y > -MAP_GRID_SIZE) {
        shift_y *= -1;
        for (int j = 0; j < MAP_GRID_SIZE; j++) {
            for (int i = 0; i < shift_y; i++)
                SDL_DestroyTexture(map->grid[i][j]);
            for (int i = 0; i < MAP_GRID_SIZE - shift_y; i++) {
                map->grid_loading_status[i][j] =
                    map->grid_loading_status[i+shift_y][j];
                map->grid[i][j] = map->grid[i+shift_y][j];
            }
            for (int i = MAP_GRID_SIZE - shift_y; i < MAP_GRID_SIZE; i++) {
                map->grid_loading_status[i][j] = 0;
                map->grid[i][j] = NULL;
            }
        }
    }

    if (shift_x < 0)
        shift_x = -shift_x;
    if (shift_y < 0)
        shift_y = -shift_y;

    if (shift_x >= MAP_GRID_SIZE || shift_y >= MAP_GRID_SIZE) {
        for (int i = 0; i < MAP_GRID_SIZE; i++) {
            for (int j = 0; j < MAP_GRID_SIZE; j++) {
                SDL_DestroyTexture(map->grid[i][j]);
                map->grid_loading_status[i][j] = 0;
            }
        }
    }

    if (!map->is_loaded)
        return;
    map->is_loaded = 0;
    start_tile_loading(map);
}
