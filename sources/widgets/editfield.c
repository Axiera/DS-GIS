#include "../../headers/widgets/editfield.h"

#define TEXT_LIST_ALLOCATION_PORTION (128*sizeof(char))
#define TEXT_CHARACTERS_SIZES_LIST_ALLOCATION_PORTION (128*sizeof(Uint32))

static SDL_Texture* render_text(SDL_Renderer* renderer,
                                TTF_Font* font,
                                const char* text,
                                const SDL_Color* color,
                                int max_width,
                                int* width,
                                int* height);

/* ---------------------- header functions definition ---------------------- */

editfield_t* editfield_init(const char* hint_text,
                            SDL_Renderer* renderer,
                            Uint16 max_text_char_size,
                            int width) {
    editfield_t* editfield = malloc(sizeof(editfield_t));
    if (editfield == NULL) {
        SDL_SetError("memory allocation failed\n%s()", __func__);
        return NULL;
    }

    int font_size = CONFIG_EDITLINE_HEIGHT - 2*EDITFIELD_VERTICAL_INDENT;
    editfield->font =
        TTF_OpenFont(CONFIG_FONT_PATH, font_size);
    if (editfield->font == NULL) {
        free(editfield);
        SDL_SetError("can not open %s\n%s()", CONFIG_FONT_PATH, __func__);
        return NULL;
    }

    editfield->hint_texture = render_text(
        renderer,
        editfield->font,
        hint_text,
        &(SDL_Color){ CONFIG_COLOR_BORDER },
        width - 2*EDITFIELD_HORIZONTAL_INDENT,
        &editfield->hint_texture_width,
        &editfield->hint_texture_height
    );
    editfield->text_texture = NULL;

    list_init(&editfield->text, TEXT_LIST_ALLOCATION_PORTION);
    list_add(&editfield->text, "\0", sizeof(char));
    list_init(
        &editfield->text_characters_sizes,
        TEXT_CHARACTERS_SIZES_LIST_ALLOCATION_PORTION
    );

    editfield->text_texture_width = 0;
    editfield->text_texture_height = 0;
    editfield->max_text_char_size = max_text_char_size;
    editfield->active = 0;

    return editfield;
}

void editfield_deinit(editfield_t* editfield) {
    TTF_CloseFont(editfield->font);
    SDL_DestroyTexture(editfield->hint_texture);
    SDL_DestroyTexture(editfield->text_texture);
    list_free(&editfield->text);
    list_free(&editfield->text_characters_sizes);
    free(editfield);
}

void editfield_draw(const editfield_t* editfield,
                    SDL_Renderer* renderer,
                    int x,
                    int y,
                    int w,
                    int h) {
    if (editfield->active)
        SDL_SetRenderDrawColor(renderer, CONFIG_COLOR_ACTIVE);
    else
        SDL_SetRenderDrawColor(renderer, CONFIG_COLOR_BORDER);
    SDL_Rect border = { x, y, w, h };
    SDL_RenderDrawRect(renderer, &border);

    if (editfield->text.size <= 1) {
        SDL_Rect dstrect = {
            .x = x + EDITFIELD_HORIZONTAL_INDENT,
            .y = y + EDITFIELD_VERTICAL_INDENT,
            .w = editfield->hint_texture_width,
            .h = editfield->hint_texture_height
        };
        SDL_RenderCopy(renderer, editfield->hint_texture, NULL, &dstrect);
    }

    SDL_Rect srcrect = {
        .x = 0,
        .y = 0,
        .w = editfield->text_texture_width,
        .h = editfield->text_texture_height
    };

    SDL_Rect dstrect = {
        .x = x + EDITFIELD_HORIZONTAL_INDENT,
        .y = y + EDITFIELD_VERTICAL_INDENT,
        .w = editfield->text_texture_width,
        .h = editfield->text_texture_height,
    };

    int max_height = h - 2*EDITFIELD_VERTICAL_INDENT;
    if (dstrect.h > max_height) {
        srcrect.y = srcrect.h - max_height;
        srcrect.h = max_height;
        dstrect.h = max_height;
    }

    SDL_RenderCopy(renderer, editfield->text_texture, &srcrect, &dstrect);
}

void editfield_handle_event(editfield_t* editfield,
                            const SDL_Event* event,
                            SDL_Renderer* renderer,
                            int x,
                            int y,
                            int w,
                            int h) {
    if (event->type == SDL_MOUSEBUTTONDOWN) {
        if (event->button.button == SDL_BUTTON_LEFT) {
            int mouse_x = event->button.x;
            int mouse_y = event->button.y;
            SDL_Rect area = { x, y, w, h };
            editfield->active = is_belong(mouse_x, mouse_y, &area);
        }
    }

    else if (editfield->active && event->type == SDL_TEXTINPUT) {
        const char* text = event->text.text;
        Uint32 text_size = strlen(text);
        if (editfield->text.size-1 + text_size > editfield->max_text_char_size)
            return;
        list_insert(&editfield->text, editfield->text.size-1, text, text_size);
        list_add(&editfield->text_characters_sizes, &text_size, sizeof(Uint32));
        if (editfield->text_texture != NULL)
            SDL_DestroyTexture(editfield->text_texture);
        editfield->text_texture = render_text(
            renderer,
            editfield->font,
            editfield->text.begin,
            &(SDL_Color){ CONFIG_COLOR_TEXT },
            w - 2*EDITFIELD_HORIZONTAL_INDENT,
            &editfield->text_texture_width,
            &editfield->text_texture_height
        );
    }

    else if (editfield->active && event->type == SDL_KEYDOWN) {
        if (event->key.keysym.sym == SDLK_BACKSPACE) {
            list_t* list = &editfield->text_characters_sizes;
            if (!list->size)
                return;
            size_t index = list->size - sizeof(Uint32);
            Uint32 character_size = *(Uint32*)list_get(list, index);
            list_erase(list, index, sizeof(Uint32));
            list_erase(
                &editfield->text,
                editfield->text.size - 1 - character_size,
                character_size
            );
            editfield->text_texture = render_text(
                renderer,
                editfield->font,
                editfield->text.begin,
                &(SDL_Color){ CONFIG_COLOR_TEXT },
                w - 2*EDITFIELD_HORIZONTAL_INDENT,
                &editfield->text_texture_width,
                &editfield->text_texture_height
            );
        }
        else {
            char* symbol = NULL;
            if (event->key.keysym.sym == SDLK_RETURN)
                symbol = "\n";
            else if (event->key.keysym.sym == SDLK_TAB)
                symbol = "    ";

            if (symbol == NULL)
                return;

            SDL_Event event;
            event.type = SDL_TEXTINPUT;
            memcpy(event.text.text, symbol, strlen(symbol)+1);
            SDL_PushEvent(&event);
        }
    }
}

const char* editfield_get_text(const editfield_t* editfield) {
    return editfield->text.begin;
}

/* ---------------------- static functions definition ---------------------- */

static SDL_Texture* render_text(SDL_Renderer* renderer,
                                TTF_Font* font,
                                const char* text,
                                const SDL_Color* color,
                                int max_width,
                                int* width,
                                int* height) {
    SDL_Surface* surface =
        TTF_RenderUTF8_Blended_Wrapped(font, text, *color, max_width);
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
