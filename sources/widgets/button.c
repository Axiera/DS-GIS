#include "../../headers/widgets/button.h"

static SDL_Texture* create_transparent_texture(SDL_Renderer* renderer,
                                               int width,
                                               int height);
static SDL_Texture* render_text_texture(SDL_Renderer* renderer,
                                        const char* text,
                                        int width,
                                        int height);

/* ---------------------- header functions definition ---------------------- */

button_t* button_init(const char* text,
                      void (*on_clicked)(void* data),
                      void* data,
                      const SDL_Rect* area,
                      SDL_Renderer* renderer) {
    button_t* button = malloc(sizeof(button_t));
    if (button == NULL) {
        SDL_SetError("memory allocation failed\n%s()", __func__);
        return NULL;
    }

    button->text_texture =
        render_text_texture(renderer, text, area->w, area->h);
    if (button->text_texture == NULL)
        return NULL;
    button->on_clicked = on_clicked;
    button->data = data;
    button->area = area;
    button->status = BUTTON_STATUS_NORMAL;

    return button;
}

void button_deinit(button_t* button) {
    SDL_DestroyTexture(button->text_texture);
    free(button);
}

void button_draw(const button_t* button, SDL_Renderer* renderer) {
    switch (button->status) {
        case BUTTON_STATUS_NORMAL:
            SDL_SetRenderDrawColor(renderer, BUTTON_COLOR_NORMAL);
            break;
        case BUTTON_STATUS_CONTAINS_NOUSE:
            SDL_SetRenderDrawColor(renderer, BUTTON_COLOR_CONTAINS_MOUSE);
            break;
        case BUTTON_STATUS_DOWN:
            SDL_SetRenderDrawColor(renderer, BUTTON_COLOR_DOWN);
            break;
    }
    SDL_RenderFillRect(renderer, button->area);
    SDL_SetRenderDrawColor(renderer, BUTTON_BORDER_COLOR);
    SDL_RenderDrawRect(renderer, button->area);
    SDL_RenderCopy(renderer, button->text_texture, NULL, button->area);
}

void button_handle_event(button_t* button, const SDL_Event* event) {
    if (event->type == SDL_MOUSEMOTION) {
        if (!is_belong(event->motion.x, event->motion.y, button->area))
            button->status = BUTTON_STATUS_NORMAL;
        else if (button->status != BUTTON_STATUS_DOWN)
            button->status = BUTTON_STATUS_CONTAINS_NOUSE;
    }

    else if (event->type == SDL_MOUSEBUTTONDOWN) {
        if (is_belong(event->button.x, event->button.y, button->area))
            button->status = BUTTON_STATUS_DOWN;
    }

    else if (event->type == SDL_MOUSEBUTTONUP) {
        _Bool has_on_clicked_function = button->on_clicked != NULL;
        if (button->status == BUTTON_STATUS_DOWN && has_on_clicked_function)
            button->on_clicked(button->data);
        else if (is_belong(event->button.x, event->button.y, button->area))
            button->status = BUTTON_STATUS_CONTAINS_NOUSE;
        else
            button->status = BUTTON_STATUS_NORMAL;
    }
}

/* ---------------------- static functions definition ---------------------- */

static SDL_Texture* create_transparent_texture(SDL_Renderer* renderer,
                                               int width,
                                               int height) {
    SDL_Texture* texture = texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width,
        height
    );
    if (texture == NULL)
        return NULL;

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    SDL_SetRenderTarget(renderer, texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, NULL);

    return texture;
}

static SDL_Texture* render_text_texture(SDL_Renderer* renderer,
                                        const char* text,
                                        int width,
                                        int height) {
    SDL_Texture* texture =
        create_transparent_texture(renderer, width, height);
    if (texture == NULL)
        return NULL;

    TTF_Font* font = TTF_OpenFont(FONT_PATH, height - 8);
    if (font == NULL) {
        SDL_DestroyTexture(texture);
        SDL_SetError("can not open %s\n%s()", FONT_PATH, __func__);
        return NULL;
    }
    SDL_Surface* surface =
        TTF_RenderUTF8_Blended(font, text, (SDL_Color){ BUTTON_TEXT_COLOR });
    if (surface == NULL) {
        TTF_CloseFont(font);
        SDL_DestroyTexture(texture);
        SDL_SetError("text rendering failed\n%s()", __func__);
        return NULL;
    }
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (text_texture == NULL) {
        TTF_CloseFont(font);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
        return NULL;
    }

    SDL_Rect dstrect = {
        .x = (width - surface->w)/2,
        .y = (height - surface->h)/2 + 1,
        .w = surface->w,
        .h = surface->h
    };
    SDL_SetRenderTarget(renderer, texture);
    SDL_RenderCopy(renderer, text_texture, NULL, &dstrect);
    SDL_SetRenderTarget(renderer, NULL);

    TTF_CloseFont(font);
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(surface);

    return texture;
}
