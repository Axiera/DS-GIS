#include "../../headers/widgets/colorpicker.h"

static const SDL_Color COLORS[COLORPICKER_COLOR_COUNT] = {
    255, 255, 255, 255,
      0, 255, 255, 255,
      0, 255,   0, 255,
    255,   0, 255, 255,
    253, 233,   0, 255,
    255,   0,  60, 255,
      0,   0, 255, 255,
    65,   65,  65, 255
};

/* ---------------------- header functions definition ---------------------- */

void colorpicker_draw(const colorpicker_t* colorpicker,
                      SDL_Renderer* renderer,
                      int x,
                      int y,
                      int w) {
    int colorpicker_width =
        (2*COLORPICKER_COLOR_COUNT - 1) * CONFIG_COLORPICKER_HEIGHT;
    x += (w - colorpicker_width) / 2;

    for (int i = 0; i < COLORPICKER_COLOR_COUNT; i++) {
        SDL_Rect area = {
            .x = x + 2*i*CONFIG_COLORPICKER_HEIGHT,
            .y = y,
            .w = CONFIG_COLORPICKER_HEIGHT,
            .h = CONFIG_COLORPICKER_HEIGHT
        };

        SDL_SetRenderDrawColor(renderer, CONFIG_COLOR_BORDER);
        SDL_RenderDrawRect(renderer, &area);

        SDL_Color color = COLORS[i];
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(
            renderer,
            &(SDL_Rect){ area.x + 1, area.y + 1, area.w - 2, area.h - 2 }
        );

        if (i == colorpicker->color) {
            SDL_SetRenderDrawColor(renderer, CONFIG_COLOR_ACTIVE);
            SDL_RenderDrawRect(
                renderer,
                &(SDL_Rect){ area.x - 4, area.y - 4, area.w + 8, area.h + 8 }
            );
        }
    }
}

void colorpicker_handle_event(colorpicker_t* colorpicker,
                              const SDL_Event* event,
                              int x,
                              int y,
                              int w) {
    int colorpicker_width =
        (2*COLORPICKER_COLOR_COUNT - 1) * CONFIG_COLORPICKER_HEIGHT;
    int colorpicker_height = CONFIG_COLORPICKER_HEIGHT;
    x += (w - colorpicker_width) / 2;

    SDL_Rect area = {
        .x = x,
        .y = y,
        .w = CONFIG_COLORPICKER_HEIGHT * (2*COLORPICKER_COLOR_COUNT - 1),
        .h = CONFIG_COLORPICKER_HEIGHT
    };

    if (event->type == SDL_MOUSEBUTTONDOWN) {
        if (!is_belong(event->button.x, event->button.y, &area))
            return;
        int item = (event->button.x - x) / colorpicker_height;
        if (item % 2)
            return;
        colorpicker->color = item/2;
    }
}

SDL_Color colorpicker_get_color(const colorpicker_t* colorpicker) {
    return COLORS[colorpicker->color];
}
