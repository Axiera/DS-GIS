#include "../../headers/map/map.h"

#define MARKER_GRID_LIST_ALLOCATION_PORTION (16*sizeof(marker_t*))
#define MARKERS_LIST_ALLOCATION_PORTION (1024*sizeof(marker_t))

static pix_pos_t to_pix(geo_pos_t geo_pos);
static pix_pos_t to_pix_from_mouse(const map_t* map,
                                   int x,
                                   int y,
                                   const SDL_Rect* area);
static void start_tile_loading(map_t* map);
static int load_tile_async(void* ptr_tile); /* SDL_ThreadFunction */
static size_t count_digits(Uint32 number);
static char* generate_request_path(const tile_t* tile);
static void shift_map_grid_data(map_t* map, Sint8 shift_x, Sint8 shift_y);
static void free_map_grid_item(map_t* map, int i, int j);
static void copy_map_grid_item(map_t* map,
                               int destination_i,
                               int destination_j,
                               int source_i,
                               int source_j);
static void update_marker_grid_item(map_t* map, int i, int j);
static void draw_markers(const map_t* map, int i, int j, const SDL_Rect* area);

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
            list_init(
                &map->marker_grid[i][j],
                MARKER_GRID_LIST_ALLOCATION_PORTION
            );
        }
    }
    list_init(&map->markers, MARKERS_LIST_ALLOCATION_PORTION);
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
            free_map_grid_item(map, i, j);
    }
    list_free(&map->markers);
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

    for (int i = 0; i < MAP_GRID_SIZE; i++) {
        for (int j = 0; j < MAP_GRID_SIZE; j++) {
            if (!map->grid_loading_status[i][j])
                continue;
            draw_markers(map, i, j, area);
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

        if (is_belong(i, j, &grid) && !map->grid_loading_status[i][j]) {
            map->grid_loading_status[i][j] = 1;
            SDL_Texture* texture = NULL;
            if (surface != NULL)
                texture = SDL_CreateTextureFromSurface(map->renderer, surface);
            map->grid[i][j] = texture;
            update_marker_grid_item(map, i, j);
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
            pix_pos_t new_center = to_pix_from_mouse(
                map,
                area->x + area->w/2 - event->motion.xrel,
                area->y + area->h/2 - event->motion.yrel,
                area
            );

            Uint32 max_pix_pos = (1 << MAP_MAX_ZOOM)*MAP_TILE_SIZE - 1;

            if (new_center.x < 0)
                new_center.x = 0;
            if (new_center.x > max_pix_pos)
                new_center.x = max_pix_pos;
            if (new_center.y < 0)
                new_center.y = 0;
            if (new_center.y > max_pix_pos)
                new_center.y = max_pix_pos;

            map->center = new_center;
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
        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);
        if (!is_belong(mouse_x, mouse_y, area))
            return;
        Uint8 zoom = map->center_tile.zoom + event->wheel.y;
        if (zoom < MAP_MIN_ZOOM || zoom > MAP_MAX_ZOOM)
            return;

        map->center_tile.size = MAP_TILE_SIZE * (1 << MAP_MAX_ZOOM-zoom);
        map->center_tile.x = map->center.x / map->center_tile.size;
        map->center_tile.y = map->center.y / map->center_tile.size;
        map->center_tile.zoom = zoom;

        for (int i = 0; i < MAP_GRID_SIZE; i++) {
            for (int j = 0; j < MAP_GRID_SIZE; j++)
                free_map_grid_item(map, i, j);
        }

        if (!map->is_loaded)
            return;
        map->is_loaded = 0;
        start_tile_loading(map);
    }

    else if (event->type == SDL_MOUSEBUTTONDOWN) {
        if (!is_belong(event->button.x, event->button.y, area))
            return;
        Uint8 button = event->button.button;
        if (button == SDL_BUTTON_LEFT && event->button.clicks == 2) {
            pix_pos_t pos =
                to_pix_from_mouse(map, event->button.x, event->button.y, area);
            marker_t marker = {
                .x = pos.x,
                .y = pos.y
            };
            list_add(&map->markers, &marker, sizeof(marker_t));

            Uint32 tile_x = pos.x / map->center_tile.size;
            Uint32 tile_y = pos.y / map->center_tile.size;
            int i = tile_y - (map->center_tile.y - MAP_GRID_SIZE/2);
            int j = tile_x - (map->center_tile.x - MAP_GRID_SIZE/2);
            update_marker_grid_item(map, i, j);
        }
    }
}

/* ---------------------- static functions definition ---------------------- */

static pix_pos_t to_pix(geo_pos_t geo_pos) {
    double lat_radian = geo_pos.lat * M_PI/180;
    double x = (geo_pos.lon + 180)/360 * (1<<MAP_MAX_ZOOM);
    double y = (1 - asinh(tan(lat_radian))/M_PI) * (1 << MAP_MAX_ZOOM-1);
    return (pix_pos_t){ x*MAP_TILE_SIZE, y*MAP_TILE_SIZE };
}

static pix_pos_t to_pix_from_mouse(const map_t* map,
                                   int x,
                                   int y,
                                   const SDL_Rect* area) {
    Sint32 delta_x = x - (area->x + area->w/2);
    Sint32 delta_y = y - (area->y + area->h/2);
    Sint32 scale = map->center_tile.size / MAP_TILE_SIZE;
    return (pix_pos_t){
        .x = map->center.x + delta_x*scale,
        .y = map->center.y + delta_y*scale
    };
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

static void shift_map_grid_data(map_t* map, Sint8 shift_x, Sint8 shift_y) {
    if (shift_x == 0 && shift_y == 0)
        return;

    if (shift_x > 0 && shift_x < MAP_GRID_SIZE) {
        for (int i = 0; i < MAP_GRID_SIZE; i++) {
            for (int j = MAP_GRID_SIZE - shift_x; j < MAP_GRID_SIZE; j++)
                free_map_grid_item(map, i, j);
            for (int j = MAP_GRID_SIZE-1; j >= shift_x; j--)
                copy_map_grid_item(map, i, j, i, j-shift_x);
        }
    }

    else if (shift_x < 0 && shift_x > -MAP_GRID_SIZE) {
        shift_x *= -1;
        for (int i = 0; i < MAP_GRID_SIZE; i++) {
            for (int j = 0; j < shift_x; j++)
                free_map_grid_item(map, i, j);
            for (int j = 0; j < MAP_GRID_SIZE - shift_x; j++)
                copy_map_grid_item(map, i, j, i, j+shift_x);
        }
    }

    if (shift_y > 0 && shift_y < MAP_GRID_SIZE) {
        for (int j = 0; j < MAP_GRID_SIZE; j++) {
            for (int i = MAP_GRID_SIZE - shift_y; i < MAP_GRID_SIZE; i++)
                free_map_grid_item(map, i, j);
            for (int i = MAP_GRID_SIZE-1; i >= shift_y; i--)
                copy_map_grid_item(map, i, j, i-shift_y, j);
        }
    }

    else if (shift_y < 0 && shift_y > -MAP_GRID_SIZE) {
        shift_y *= -1;
        for (int j = 0; j < MAP_GRID_SIZE; j++) {
            for (int i = 0; i < shift_y; i++)
                free_map_grid_item(map, i, j);
            for (int i = 0; i < MAP_GRID_SIZE - shift_y; i++)
                copy_map_grid_item(map, i, j, i+shift_y, j);
        }
    }

    if (shift_x < 0)
        shift_x = -shift_x;
    if (shift_y < 0)
        shift_y = -shift_y;

    if (shift_x >= MAP_GRID_SIZE || shift_y >= MAP_GRID_SIZE) {
        for (int i = 0; i < MAP_GRID_SIZE; i++) {
            for (int j = 0; j < MAP_GRID_SIZE; j++)
                free_map_grid_item(map, i, j);
        }
    }

    if (!map->is_loaded)
        return;
    map->is_loaded = 0;
    start_tile_loading(map);
}

static void free_map_grid_item(map_t* map, int i, int j) {
    SDL_DestroyTexture(map->grid[i][j]);
    map->grid[i][j] = NULL;
    map->grid_loading_status[i][j] = 0;
    list_free(&map->marker_grid[i][j]);
}

static void copy_map_grid_item(map_t* map,
                               int destination_i,
                               int destination_j,
                               int source_i,
                               int source_j) {
    map->grid[destination_i][destination_j] = map->grid[source_i][source_j];
    map->grid[source_i][source_j] = NULL;

    map->grid_loading_status[destination_i][destination_j] =
        map->grid_loading_status[source_i][source_j];
    map->grid_loading_status[source_i][source_j] = 0;

    map->marker_grid[destination_i][destination_j] =
        map->marker_grid[source_i][source_j];
    map->marker_grid[source_i][source_j].begin = NULL;
    map->marker_grid[source_i][source_j].size = 0;
    map->marker_grid[source_i][source_j].allocated_size = 0;
}

static void update_marker_grid_item(map_t* map, int i, int j) {
    list_free(&map->marker_grid[i][j]);
    marker_cut(&map->marker_grid[i][j], &map->markers, &(SDL_Rect){
        .x = (map->center_tile.x - MAP_GRID_SIZE/2 + j) * map->center_tile.size,
        .y = (map->center_tile.y - MAP_GRID_SIZE/2 + i) * map->center_tile.size,
        .w = map->center_tile.size,
        .h = map->center_tile.size
    });
}

static void draw_markers(const map_t* map, int i, int j, const SDL_Rect* area) {
    pix_pos_t grid_begin = {
        .x = (map->center_tile.x - MAP_GRID_SIZE/2) * map->center_tile.size,
        .y = (map->center_tile.y - MAP_GRID_SIZE/2) * map->center_tile.size
    };
    Uint32 scale = map->center_tile.size / MAP_TILE_SIZE;
    Sint32 begin_x = area->x + area->w/2 - (map->center.x-grid_begin.x)/scale;
    Sint32 begin_y = area->y + area->h/2 - (map->center.y-grid_begin.y)/scale;

    const list_t* list = &map->marker_grid[i][j];
    for (int k = 0; k < list->size; k += sizeof(marker_t*)) {
        marker_t* marker = *(marker_t**)list_get(list, k);

        Sint32 x = begin_x + j*MAP_TILE_SIZE
            + (marker->x % map->center_tile.size)/scale
            - MARKER_PIXEL_SIZE/2;
        Sint32 y = begin_y + i*MAP_TILE_SIZE
            + (marker->y % map->center_tile.size)/scale
            - MARKER_PIXEL_SIZE/2;
        if (x + MARKER_PIXEL_SIZE < area->x || x >= area->x + area->w)
            continue;
        if (y + MARKER_PIXEL_SIZE < area->y || y >= area->y + area->h)
            continue;

        int indent;
        if (map->center_tile.zoom >= 17)
            indent = 0;
        else if (map->center_tile.zoom >= 15)
            indent = 5;
        else if (map->center_tile.zoom >= 13)
            indent = 6;
        else if (map->center_tile.zoom >= MAP_MIN_ZOOM)
            indent = 7;

        int pixel_i_begin = y + indent < area->y ? area->y - y : indent;
        int pixel_j_begin = x + indent < area->x ? area->x - x : indent;
        int pixel_i_end = y + MARKER_PIXEL_SIZE - indent > area->y + area->h ?
            area->y + area->h - y : MARKER_PIXEL_SIZE - indent;
        int pixel_j_end = x + MARKER_PIXEL_SIZE - indent > area->x + area->w ?
            area->x + area->w - x : MARKER_PIXEL_SIZE - indent;

        const Uint8* MARKER_PIXELS = marker_get_pixels();
        SDL_SetRenderDrawColor(map->renderer, 255, 255, 255, 255);
        for (int pi = pixel_i_begin; pi < pixel_i_end; pi++) {
            for (int pj = pixel_j_begin; pj < pixel_j_end; pj++) {
                const Uint8* pixel = MARKER_PIXELS + pi*MARKER_PIXEL_SIZE + pj;
                if (*pixel)
                    SDL_RenderDrawPoint(map->renderer, x+pj, y+pi);
            }
        }
    }
}
