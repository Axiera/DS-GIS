#include "../../headers/widgets/editline.h"

#define TEXT_LIST_ALLOCATION_PORTION (64*sizeof(char))
#define TEXT_CHARACTERS_SIZES_LIST_ALLOCATION_PORTION (64*sizeof(Uint32))

static SDL_Texture* render_text(SDL_Renderer* renderer,
                                TTF_Font* font,
                                const char* text,
                                const SDL_Color* color,
                                int* width,
                                int* height);

/* ---------------------- header functions definition ---------------------- */

editline_t* editline_init(const char* hint_text, SDL_Renderer* renderer) {
    editline_t* editline = malloc(sizeof(editline_t));
    if (editline == NULL) {
        SDL_SetError("memory allocation failed\n%s()", __func__);
        return NULL;
    }

    editline->font =
        TTF_OpenFont(FONT_PATH, EDITLINE_HEIGHT - 2*EDITLINE_VERTICAL_INDENT);
    if (editline->font == NULL) {
        free(editline);
        SDL_SetError("can not open %s\n%s()", FONT_PATH, __func__);
        return NULL;
    }

    editline->hint_texture = render_text(
        renderer,
        editline->font,
        hint_text,
        &(SDL_Color){ EDITLINE_HINT_COLOR },
        &editline->hint_texture_width,
        NULL
    );
    editline->text_texture = NULL;

    list_init(&editline->text, TEXT_LIST_ALLOCATION_PORTION);
    list_add(&editline->text, "\0", sizeof(char));
    list_init(
        &editline->text_characters_sizes,
        TEXT_CHARACTERS_SIZES_LIST_ALLOCATION_PORTION
    );

    editline->text_texture_width = 0;
    editline->active = 0;

    return editline;
}

void editline_deinit(editline_t* editline) {
    list_free(&editline->text);
    list_free(&editline->text_characters_sizes);
    TTF_CloseFont(editline->font);
    free(editline);
}

void editline_draw(const editline_t* editline,
                   SDL_Renderer* renderer,
                   int x,
                   int y,
                   int width) {
    if (editline->active)
        SDL_SetRenderDrawColor(renderer, EDITLINE_ACTIVE_BORDER_COLOR);
    else
        SDL_SetRenderDrawColor(renderer, EDITLINE_BORDER_COLOR);
    SDL_RenderDrawRect(renderer, &(SDL_Rect){ x, y, width, EDITLINE_HEIGHT });

    if (editline->text.size <= 1) {
        SDL_Rect dstrect = {
            .x = x + EDITLINE_HORIZONTAL_INDENT,
            .y = y + EDITLINE_VERTICAL_INDENT,
            .w = editline->hint_texture_width,
            .h = EDITLINE_HEIGHT - 2*EDITLINE_VERTICAL_INDENT
        };
        SDL_RenderCopy(renderer, editline->hint_texture, NULL, &dstrect);
    }

    SDL_Rect srcrect = {
        .x = 0,
        .y = 0,
        .w = editline->text_texture_width,
        .h = editline->text_texture_height
    };

    SDL_Rect dstrect = {
        .x = x + EDITLINE_HORIZONTAL_INDENT,
        .y = y + EDITLINE_VERTICAL_INDENT,
        .w = editline->text_texture_width,
        .h = EDITLINE_HEIGHT - 2*EDITLINE_VERTICAL_INDENT
    };

    int max_width = width - 2*EDITLINE_HORIZONTAL_INDENT;
    if (dstrect.w > max_width) {
        srcrect.x = srcrect.w - max_width;
        srcrect.w = max_width;
        dstrect.w = max_width;
    }

    SDL_RenderCopy(renderer, editline->text_texture, &srcrect, &dstrect);
}

void editline_handle_event(editline_t* editline,
                           const SDL_Event* event,
                           SDL_Renderer* renderer,
                           int x,
                           int y,
                           int width) {
    if (event->type == SDL_MOUSEBUTTONDOWN) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            int mouse_x = event->button.x;
            int mouse_y = event->button.y;
            SDL_Rect area = { x, y, width, EDITLINE_HEIGHT };
            editline->active = is_belong(mouse_x, mouse_y, &area);
        }
    }

    else if (editline->active && event->type == SDL_TEXTINPUT) {
        const char* text = event->text.text;
        Uint32 text_size = strlen(text);
        list_insert(&editline->text, editline->text.size-1, text, text_size);
        list_add(&editline->text_characters_sizes, &text_size, sizeof(Uint32));
        if (editline->text_texture != NULL)
            SDL_DestroyTexture(editline->text_texture);
        editline->text_texture = render_text(
            renderer,
            editline->font,
            editline->text.begin,
            &(SDL_Color){ EDITLINE_TEXT_COLOR },
            &editline->text_texture_width,
            &editline->text_texture_height
        );
    }

    else if (editline->active && event->type == SDL_KEYDOWN) {
        if (event->key.keysym.sym == SDLK_BACKSPACE) {
            list_t* list = &editline->text_characters_sizes;
            if (!list->size)
                return;
            size_t index = list->size - sizeof(Uint32);
            Uint32 character_size = *(Uint32*)list_get(list, index);
            list_erase(list, index, sizeof(Uint32));
            list_erase(
                &editline->text,
                editline->text.size - 1 - character_size,
                character_size
            );
            editline->text_texture = render_text(
                renderer,
                editline->font,
                editline->text.begin,
                &(SDL_Color){ EDITLINE_TEXT_COLOR },
                &editline->text_texture_width,
                &editline->text_texture_height
            );
        }
        else if (event->key.keysym.sym == SDLK_RETURN) {
            editline->active = 0;
        }
    }
}

const char* editline_get_text(const editline_t* editline) {
    return editline->text.begin;
}

/* ---------------------- static functions definition ---------------------- */

static SDL_Texture* render_text(SDL_Renderer* renderer,
                                TTF_Font* font,
                                const char* text,
                                const SDL_Color* color,
                                int* width,
                                int* height) {
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text, *color);
    if (surface == NULL)
        return NULL;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        SDL_FreeSurface(surface);
        return NULL;
    }

    if (width != NULL)
        *width = surface->w;
    if (height != NULL)
        *height = surface->h;
    SDL_FreeSurface(surface);

    return texture;
}
