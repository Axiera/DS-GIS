#include "../headers/isbelong.h"

_Bool is_belong(int x, int y, const SDL_Rect* rect) {
    return x >= rect->x && x < rect->x + rect->w
        && y >= rect->y && y < rect->y + rect->h;
}
