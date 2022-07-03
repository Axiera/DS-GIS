#ifndef LIST_H
#define LIST_H

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void* begin;
    size_t size;
    size_t allocated_size;
    size_t allocation_portion;
} list_t;

void list_init(list_t* list, size_t allocation_portion_byte);
void list_free(list_t* list);
int list_add(list_t* list, void* data, size_t data_size);
void* list_get(const list_t* list, size_t index_byte);

/*
    list_add()
        returns 0 on success
        returns non-0 value on error, call SDL_GetError() for more information
*/

#endif
