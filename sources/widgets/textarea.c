#include "../../headers/widgets/textarea.h"

textarea_t* textarea_init(void) {
    textarea_t* textarea = malloc(sizeof(textarea_t));
    if (textarea == NULL) {
        SDL_SetError("memory allocation failed\n%s()", __func__);
        return NULL;
    }

    textarea->font = TTF_OpenFont(CONFIG_FONT_PATH, CONFIG_FONT_SIZE);
    if (textarea->font == NULL) {
        free(textarea);
        SDL_SetError("can not open %s\n%s()", CONFIG_FONT_PATH, __func__);
        return NULL;
    }

    return textarea;
}

void textarea_deinit(textarea_t* textarea) {
    TTF_CloseFont(textarea->font);
    SDL_DestroyTexture(textarea->text_texture);
    free(textarea);
}

void textarea_draw(const textarea_t* textarea,
                   SDL_Renderer* renderer,
                   int x,
                   int y,
                   int h) {
    SDL_Rect srcrect = {
        .x = 0,
        .y = 0,
        .w = textarea->w,
        .h = textarea->h <= h ? textarea->h : h
    };
    SDL_Rect dstrect = {
        .x = x,
        .y = y,
        .w = srcrect.w,
        .h = srcrect.h
    };
    SDL_RenderCopy(renderer, textarea->text_texture, &srcrect, &dstrect);
}

void textarea_set_text(textarea_t* textarea,
                       SDL_Renderer* renderer,
                       const char* text,
                       int w) {
    SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(
        textarea->font,
        text,
        (SDL_Color){ CONFIG_COLOR_TEXT },
        w
    );
    if (surface == NULL)
        return;
    textarea->w = surface->w;
    textarea->h = surface->h;
    if (textarea->text_texture != NULL)
        SDL_DestroyTexture(textarea->text_texture);
    textarea->text_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
}
