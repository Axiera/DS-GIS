#ifndef HTTP_H
#define HTTP_H

#include <SDL2/SDL.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    size_t size;
    void* data;
} response_t;

int http_init(void);
void http_deinit(void);
response_t http_get(const char* hostname, const char* path);

/*
    http_init()
        returns 0 on success
        returns non-0 value on error, call SDL_GetError() for more information

    http_get()
        path must begins with "/"
        response_t.data needs to free
*/

#endif
