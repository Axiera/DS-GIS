#include <SDL2/SDL.h>
#include <SDL2/SDL_Image.h>
#include <SDL2/SDL_ttf.h>
#include <windows.h>
#include <stdlib.h>

#include "headers/http.h"
#include "headers/map/map.h"

const char   TITLE[7]              = "DS GIS";
const int    INITIAL_WINDOW_WIDTH  = 720;
const int    INITIAL_WINDOW_HEIGHT = 400;
const double INITIAL_LATITUDE      = 62.779147;
const double INITIAL_LONGITUDE     = 40.334442;
const Uint8  INITIAL_ZOOM          = 15;

int init(SDL_Window** window, SDL_Renderer** renderer, map_t** map);
void deinit(SDL_Window* window, SDL_Renderer* renderer, map_t* map);

int main(int argc, char* argv[]) {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    map_t* map = NULL;
    if (init(&window, &renderer, &map)) {
        MessageBox(GetActiveWindow(), SDL_GetError(), NULL, MB_OK);
        exit(EXIT_FAILURE);
    }

    int window_width = INITIAL_WINDOW_WIDTH;
    int window_height = INITIAL_WINDOW_HEIGHT;

    int event_received_successfully;
    SDL_Event event;
    while (event_received_successfully = SDL_WaitEvent(&event)) {
        SDL_Rect map_area = { 0, 0, window_width, window_height };
        map_handle_event(map, &event, renderer, map_area);

        if (event.type == SDL_QUIT) {
            break;
        } else if (event.type == SDL_WINDOWEVENT) {
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                window_width = event.window.data1;
                window_height = event.window.data2;
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        map_draw(map, map_area);
        SDL_RenderPresent(renderer);
    }

    if (!event_received_successfully) {
        MessageBox(GetActiveWindow(), SDL_GetError(), NULL, MB_OK);
        deinit(window, renderer, map);
        exit(EXIT_FAILURE);
    }

    deinit(window, renderer, map);
    return 0;
}

int init(SDL_Window** window, SDL_Renderer** renderer, map_t** map) {
    if (SDL_Init(SDL_INIT_VIDEO))
        return 1;

    if (!IMG_Init(IMG_INIT_JPG)) {
        SDL_SetError(IMG_GetError());
        SDL_Quit();
        return 1;
    }

    if (TTF_Init()) {
        SDL_SetError(TTF_GetError());
        IMG_Quit();
        SDL_Quit();
    }

    *window = SDL_CreateWindow(
        TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        INITIAL_WINDOW_WIDTH,
        INITIAL_WINDOW_HEIGHT,
        SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        SDL_DestroyWindow(*window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    if (http_init()) {
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    geo_pos_t map_center = { INITIAL_LATITUDE, INITIAL_LONGITUDE };
    *map = map_init(*renderer, map_center, INITIAL_ZOOM);
    if (*map == NULL) {
        http_deinit();
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    return 0;
}

void deinit(SDL_Window* window, SDL_Renderer* renderer, map_t* map) {
    map_deinit(map);
    http_deinit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}
