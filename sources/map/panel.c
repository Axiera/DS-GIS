#include "../../headers/map/panel.h"

static void cancel(void* data);

static void init_create_marker(panel_t* panel, SDL_Renderer* renderer);
static void deinit_create_marker(panel_t* panel);
static void draw_create_marker(const panel_t* panel,
                               SDL_Renderer* renderer,
                               int x,
                               int y,
                               int h);
static void handle_event_create_marker(panel_t* panel,
                                       const SDL_Event* event,
                                       SDL_Renderer* renderer,
                                       int x,
                                       int y,
                                       int h);

/* ---------------------- header functions definition ---------------------- */

panel_t* panel_init(int type,
                    SDL_Renderer* renderer,
                    void* map,
                    void (*on_executed)(void*)) {
    panel_t* panel = malloc(sizeof(panel_t));
    if (panel == NULL) {
        SDL_SetError("memory allocation failed\n%s()", __func__);
        return NULL;
    }

    panel->parameters.type = type;
    panel->parameters.canceled = 0;
    panel->parameters.map = map;
    panel->parameters.on_executed = on_executed;

    if (type == PANEL_CREATE_MARKER)
        init_create_marker(panel, renderer);

    return panel;
}

void panel_deinit(panel_t* panel) {
    if (panel->parameters.type == PANEL_CREATE_MARKER)
        deinit_create_marker(panel);
    free(panel);
}

void panel_draw(const panel_t* panel,
                SDL_Renderer* renderer,
                int x,
                int y,
                int h) {
    if (panel->parameters.type == PANEL_CREATE_MARKER)
        draw_create_marker(panel, renderer, x, y, h);
}

void panel_handle_event(panel_t* panel,
                        const SDL_Event* event,
                        SDL_Renderer* renderer,
                        int x,
                        int y,
                        int h) {
    if (panel->parameters.type == PANEL_CREATE_MARKER)
        handle_event_create_marker(panel, event, renderer, x, y, h);
}

/* ---------------------- static functions definition ---------------------- */

static void cancel(void* data) {
    panel_t* panel = data;
    panel->parameters.canceled = 1;
    panel->parameters.on_executed(panel);
}

static void init_create_marker(panel_t* panel, SDL_Renderer* renderer) {
    panel->create_marker.editline = editline_init("name", renderer);
    panel->create_marker.editfield = editfield_init(
        "description",
        renderer,
        CONFIG_MAP_PANEL_WIDTH - 2*PANEL_INDENT
    );
    panel->create_marker.button_cancel =
        button_init("Cancel", renderer, cancel, panel);
    panel->create_marker.button_create =
        button_init("Create", renderer, panel->parameters.on_executed, panel);
    panel->create_marker.colorpicker.color = 0;
}

static void deinit_create_marker(panel_t* panel) {
    editline_deinit(panel->create_marker.editline);
    editfield_deinit(panel->create_marker.editfield);
    button_deinit(panel->create_marker.button_cancel);
    button_deinit(panel->create_marker.button_create);
}

static void draw_create_marker(const panel_t* panel,
                               SDL_Renderer* renderer,
                               int x,
                               int y,
                               int h) {
    x += PANEL_INDENT;
    int w = CONFIG_MAP_PANEL_WIDTH - 2*PANEL_INDENT;

    const editline_t* editline = panel->create_marker.editline;
    const colorpicker_t* colorpicker = &panel->create_marker.colorpicker;
    const editfield_t* editfield = panel->create_marker.editfield;
    const button_t* button_create = panel->create_marker.button_create;
    const button_t* button_cancel = panel->create_marker.button_cancel;

    int editline_y = y + PANEL_INDENT;
    int colorpicker_y = editline_y + CONFIG_EDITLINE_HEIGHT + 2*PANEL_INDENT;

    int editfield_y =
        colorpicker_y + CONFIG_COLORPICKER_HEIGHT + 2*PANEL_INDENT;
    int editfield_h = PANEL_EDITFIELD_HEIGHT;
    int editfield_text_h = editfield->text_texture_height;
    if (editfield_text_h + 2*EDITFIELD_VERTICAL_INDENT > editfield_h)
        editfield_h = editfield_text_h + 2*EDITFIELD_VERTICAL_INDENT;
    int editfield_max_h =
        h - editfield_y - 2*PANEL_INDENT - CONFIG_BUTTON_HEIGHT;
    if (editfield_h > editfield_max_h)
        editfield_h = editfield_max_h;

    int button_create_y = editfield_y + editfield_h + PANEL_INDENT;
    int button_create_x = x + w - CONFIG_BUTTON_WIDTH;
    int button_cancel_y = button_create_y;
    int button_cancel_x = button_create_x - CONFIG_BUTTON_WIDTH - PANEL_INDENT;

    editline_draw(editline, renderer, x, editline_y, w);
    colorpicker_draw(colorpicker, renderer, x, colorpicker_y, w);
    editfield_draw(editfield, renderer, x, editfield_y, w, editfield_h);
    button_draw(button_create, renderer, button_create_x, button_create_y);
    button_draw(button_cancel, renderer, button_cancel_x, button_cancel_y);
}

static void handle_event_create_marker(panel_t* panel,
                                       const SDL_Event* event,
                                       SDL_Renderer* renderer,
                                       int x,
                                       int y,
                                       int h) {
    x += PANEL_INDENT;
    int w = CONFIG_MAP_PANEL_WIDTH - 2*PANEL_INDENT;

    editline_t* editline = panel->create_marker.editline;
    colorpicker_t* colorpicker = &panel->create_marker.colorpicker;
    editfield_t* editfield = panel->create_marker.editfield;
    button_t* button_create = panel->create_marker.button_create;
    button_t* button_cancel = panel->create_marker.button_cancel;

    int editline_y = y + PANEL_INDENT;
    int colorpicker_y = editline_y + CONFIG_EDITLINE_HEIGHT + 2*PANEL_INDENT;

    int editfield_y =
        colorpicker_y + CONFIG_COLORPICKER_HEIGHT + 2*PANEL_INDENT;
    int editfield_h = PANEL_EDITFIELD_HEIGHT;
    int editfield_text_h = editfield->text_texture_height;
    if (editfield_text_h + 2*EDITFIELD_VERTICAL_INDENT > editfield_h)
        editfield_h = editfield_text_h + 2*EDITFIELD_VERTICAL_INDENT;
    int editfield_max_h =
        h - editfield_y - 2*PANEL_INDENT - CONFIG_BUTTON_HEIGHT;
    if (editfield_h > editfield_max_h)
        editfield_h = editfield_max_h;

    int button_create_y = editfield_y + editfield_h + PANEL_INDENT;
    int button_create_x = x + w - CONFIG_BUTTON_WIDTH;
    int button_cancel_y = button_create_y;
    int button_cancel_x = button_create_x - CONFIG_BUTTON_WIDTH - PANEL_INDENT;

    editline_handle_event(editline, event, renderer, x, editline_y, w);
    colorpicker_handle_event(colorpicker, event, x, colorpicker_y, w);
    editfield_handle_event(
        editfield, event, renderer, x, editfield_y, w, editfield_h);
    button_handle_event(button_create, event, button_create_x, button_create_y);
    button_handle_event(button_cancel, event, button_cancel_x, button_cancel_y);
}
