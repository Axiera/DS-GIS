#include "../headers/list.h"

void list_init(list_t* list, size_t allocation_portion_byte) {
    list->begin = NULL;
    list->size = 0;
    list->allocated_size = 0;
    list->allocation_portion = allocation_portion_byte;
}

void list_free(list_t* list) {
    free(list->begin);
    list->begin = NULL;
    list->size = 0;
    list->allocated_size = 0;
}

int list_add(list_t* list, const void* data, size_t data_size) {
    if (list->size + data_size > list->allocated_size) {
        size_t realloc_size = list->allocated_size;
        while (realloc_size < list->size + data_size)
            realloc_size += list->allocation_portion;
        void* new_begin = realloc(list->begin, realloc_size);
        if (new_begin == NULL) {
            SDL_SetError("memory allocation failed\n%s()", __func__);
            return 1;
        }
        list->begin = new_begin;
        list->allocated_size = realloc_size;
    }

    memcpy(list->begin + list->size, data, data_size);
    list->size += data_size;
    return 0;
}

int list_insert(list_t* list,
                size_t index_byte,
                const void* data,
                size_t data_size) {
    if (list->size + data_size > list->allocated_size) {
        size_t realloc_size = list->allocated_size;
        while (realloc_size < list->size + data_size)
            realloc_size += list->allocation_portion;
        void* new_begin = realloc(list->begin, realloc_size);
        if (new_begin == NULL) {
            SDL_SetError("memory allocation failed\n%s()", __func__);
            return 1;
        }
        list->begin = new_begin;
        list->allocated_size = realloc_size;
    }

    memmove(
        list->begin + index_byte + data_size,
        list->begin + index_byte,
        list->size - index_byte
    );
    memcpy(list->begin + index_byte, data, data_size);
    list->size += data_size;
    return 0;
}

void* list_get(const list_t* list, size_t index_byte) {
    return list->begin + index_byte;
}
