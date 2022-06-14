#include <SDL2/SDL.h>
#include <windows.h>
#include <stdlib.h>
#include "headers/http.h"

const char TITLE[7] = "DS GIS";
const int WINDOW_WIDTH = 720;
const int WINDOW_HEIGHT = 400;

int init(SDL_Window** window, SDL_Renderer** renderer);
void deinit(SDL_Window* window, SDL_Renderer* renderer);

int main(int argc, char* argv[]) {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    if (init(&window, &renderer)) {
        MessageBox(GetActiveWindow(), SDL_GetError(), NULL, MB_OK);
        exit(EXIT_FAILURE);
    }

    int event_received_successfully;
    SDL_Event event;
    while (event_received_successfully = SDL_WaitEvent(&event)) {
        if (event.type == SDL_QUIT)
            break;
    }

    if (!event_received_successfully) {
        MessageBox(GetActiveWindow(), SDL_GetError(), NULL, MB_OK);
        deinit(window, renderer);
        exit(EXIT_FAILURE);
    }

    deinit(window, renderer);
    return 0;
}

int init(SDL_Window** window, SDL_Renderer** renderer) {
    if (SDL_Init(SDL_INIT_VIDEO))
        return 1;

    *window = SDL_CreateWindow(
        TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        SDL_Quit();
        return 1;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return 1;
    }

    if (http_init()) {
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        SDL_Quit();
        return 1;
    }

    return 0;
}

void deinit(SDL_Window* window, SDL_Renderer* renderer) {
    http_deinit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
