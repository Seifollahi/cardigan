// Behavioral Pebble API stub for host-side functional testing.
// Implements just enough of the OS contract: persist storage,
// a grayscale framebuffer GContext, window stack + click routing,
// AppMessage delivery, and ActionMenu capture.
#define _POSIX_C_SOURCE 200809L   /* strdup with -std=c11 */
#include "stub.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ---------------- display / framebuffer ----------------
static int s_w = 200, s_h = 228;
#define FB_MAX (240 * 240)
static uint8_t s_fb[FB_MAX];

struct GContext { GColor fill; GColor text; };
static struct GContext s_ctx;

void stub_set_display(int w, int h) { s_w = w; s_h = h; }

static uint8_t argb_to_gray(GColor c) {
  const int r = (c.argb >> 4) & 3, g = (c.argb >> 2) & 3, b = c.argb & 3;
  return (uint8_t)(((r * 299 + g * 587 + b * 114) * 255) / (3 * 1000));
}

void graphics_context_set_fill_color(GContext *ctx, GColor c) { ctx->fill = c; }
void graphics_context_set_text_color(GContext *ctx, GColor c) { ctx->text = c; }

void graphics_fill_rect(GContext *ctx, GRect r, uint16_t rad, GCornerMask m) {
  (void)rad; (void)m;
  const uint8_t v = argb_to_gray(ctx->fill);
  for (int y = r.origin.y; y < r.origin.y + r.size.h; y++) {
    if (y < 0 || y >= s_h) continue;
    for (int x = r.origin.x; x < r.origin.x + r.size.w; x++) {
      if (x < 0 || x >= s_w) continue;
      s_fb[y * s_w + x] = v;
    }
  }
}

void graphics_fill_circle(GContext *ctx, GPoint p, uint16_t rad) {
  const uint8_t v = argb_to_gray(ctx->fill);
  for (int y = p.y - rad; y <= p.y + rad; y++) {
    if (y < 0 || y >= s_h) continue;
    for (int x = p.x - rad; x <= p.x + rad; x++) {
      if (x < 0 || x >= s_w) continue;
      const int dx = x - p.x, dy = y - p.y;
      if (dx * dx + dy * dy <= (int)(rad * rad)) s_fb[y * s_w + x] = v;
    }
  }
}

GFont fonts_get_system_font(const char *key) { (void)key; return NULL; }
void graphics_draw_text(GContext *ctx, const char *text, GFont f, const GRect box,
                        GTextOverflowMode om, GTextAlignment al, GTextAttributes *a) {
  (void)ctx; (void)text; (void)f; (void)box; (void)om; (void)al; (void)a;
  // text is not rasterized on host; scanner tests only need the code pixels
}

// ---------------- layers ----------------
struct Layer { GRect bounds; LayerUpdateProc proc; bool alive; };
#define MAX_LAYERS 32
static Layer *s_layers[MAX_LAYERS];
static int s_nlayers = 0;

Layer *layer_create(GRect bounds) {
  Layer *l = calloc(1, sizeof(Layer));
  l->bounds = bounds; l->alive = true;
  if (s_nlayers < MAX_LAYERS) s_layers[s_nlayers++] = l;
  return l;
}
void layer_destroy(Layer *l) { if (l) l->alive = false; }   // keep slot; freed at exit
void layer_set_update_proc(Layer *l, LayerUpdateProc p) { l->proc = p; }
void layer_add_child(Layer *p, Layer *c) { (void)p; (void)c; }
void layer_mark_dirty(Layer *l) { (void)l; }
GRect layer_get_bounds(const Layer *l) { return l->bounds; }

// ---------------- windows / clicks ----------------
struct Window {
  WindowHandlers handlers;
  ClickConfigProvider provider;
  Layer *root;
  GColor bg;
};
#define MAX_STACK 8
static Window *s_stack[MAX_STACK];
static int s_depth = 0;
static ClickHandler s_click[4];
static ClickHandler s_long[4];

Window *window_create(void) { return calloc(1, sizeof(Window)); }
void window_destroy(Window *w) { if (w && w->root) layer_destroy(w->root); free(w); }
void window_set_window_handlers(Window *w, WindowHandlers h) { w->handlers = h; }
void window_set_background_color(Window *w, GColor c) { w->bg = c; }
void window_set_click_config_provider(Window *w, ClickConfigProvider p) { w->provider = p; }
Layer *window_get_root_layer(const Window *w) {
  Window *mw = (Window *)w;
  if (!mw->root) mw->root = layer_create(GRect(0, 0, s_w, s_h));
  return mw->root;
}
void window_single_click_subscribe(ButtonId b, ClickHandler h) { s_click[b] = h; }
void window_long_click_subscribe(ButtonId b, uint16_t ms, ClickHandler down, ClickHandler up) {
  (void)ms; (void)up; s_long[b] = down;
}
static void apply_click_config(Window *w) {
  memset(s_click, 0, sizeof(s_click));
  memset(s_long, 0, sizeof(s_long));
  if (w && w->provider) w->provider(NULL);
}
void window_stack_push(Window *w, bool anim) {
  (void)anim;
  if (s_depth < MAX_STACK) s_stack[s_depth++] = w;
  if (w->handlers.load) w->handlers.load(w);
  apply_click_config(w);
}
bool window_stack_remove(Window *w, bool anim) {
  (void)anim;
  for (int i = 0; i < s_depth; i++) {
    if (s_stack[i] == w) {
      memmove(&s_stack[i], &s_stack[i + 1], (s_depth - i - 1) * sizeof(Window *));
      s_depth--;
      if (w->handlers.unload) w->handlers.unload(w);  // typically destroys w
      apply_click_config(s_depth ? s_stack[s_depth - 1] : NULL);
      return true;
    }
  }
  return false;
}
int  stub_stack_depth(void) { return s_depth; }
void stub_pop_top(void)     { if (s_depth) window_stack_remove(s_stack[s_depth - 1], false); }
void stub_click(ButtonId b)      { if (s_click[b]) s_click[b](NULL, NULL); }
void stub_long_click(ButtonId b) { if (s_long[b])  s_long[b](NULL, NULL); }

int stub_render_pgm(const char *path) {
  memset(s_fb, 0x77, sizeof(s_fb));            // gray: exposes unpainted px
  for (int i = 0; i < s_nlayers; i++) {
    Layer *l = s_layers[i];
    if (l && l->alive && l->proc) l->proc(l, &s_ctx);
  }
  FILE *f = fopen(path, "wb");
  if (!f) return -1;
  fprintf(f, "P5\n%d %d\n255\n", s_w, s_h);
  for (int y = 0; y < s_h; y++) fwrite(&s_fb[y * s_w], 1, s_w, f);
  fclose(f);
  return 0;
}

// ---------------- menu layer / status bar (inert) ----------------
struct MenuLayer { GRect bounds; MenuLayerCallbacks cb; Layer *layer; };
MenuLayer *menu_layer_create(GRect b) {
  MenuLayer *m = calloc(1, sizeof(MenuLayer));
  m->bounds = b; m->layer = layer_create(b);
  return m;
}
void menu_layer_destroy(MenuLayer *m) { if (m) { layer_destroy(m->layer); free(m); } }
void menu_layer_set_callbacks(MenuLayer *m, void *ctx, MenuLayerCallbacks cb) { (void)ctx; m->cb = cb; }
void menu_layer_set_click_config_onto_window(MenuLayer *m, Window *w) { (void)m; (void)w; }
void menu_layer_set_highlight_colors(MenuLayer *m, GColor a, GColor b) { (void)m; (void)a; (void)b; }
void menu_layer_reload_data(MenuLayer *m) { (void)m; }
bool menu_layer_is_index_selected(MenuLayer *m, MenuIndex *i) { (void)m; (void)i; return false; }
Layer *menu_layer_get_layer(const MenuLayer *m) { return m->layer; }
bool menu_cell_layer_is_highlighted(const Layer *l) { (void)l; return false; }

StatusBarLayer *status_bar_layer_create(void) { return calloc(1, 64); }
void status_bar_layer_destroy(StatusBarLayer *s) { free(s); }
void status_bar_layer_set_colors(StatusBarLayer *s, GColor a, GColor b) { (void)s; (void)a; (void)b; }
void status_bar_layer_set_separator_mode(StatusBarLayer *s, StatusBarLayerSeparatorMode m) { (void)s; (void)m; }
static Layer s_sb_layer = { {{0,0},{0,0}}, NULL, false };
Layer *status_bar_layer_get_layer(StatusBarLayer *s) { (void)s; return &s_sb_layer; }

// ---------------- action menu capture ----------------
struct ActionMenuItem { const char *label; ActionMenuPerformActionCb cb; void *data; };
struct ActionMenuLevel { int n; struct ActionMenuItem items[8]; };
static ActionMenuConfig s_am_config;
static bool s_am_open = false;

ActionMenuLevel *action_menu_level_create(uint16_t n) { (void)n; return calloc(1, sizeof(ActionMenuLevel)); }
ActionMenuItem *action_menu_level_add_action(ActionMenuLevel *lvl, const char *label,
                                             ActionMenuPerformActionCb cb, void *data) {
  struct ActionMenuItem *it = &lvl->items[lvl->n++];
  it->label = label; it->cb = cb; it->data = data;
  return it;
}
ActionMenu *action_menu_open(ActionMenuConfig *cfg) {
  s_am_config = *cfg; s_am_open = true;
  return (ActionMenu *)&s_am_config;
}
void *action_menu_item_get_action_data(const ActionMenuItem *it) { return it->data; }
ActionMenuLevel *action_menu_get_root_level(ActionMenu *m) {
  (void)m; return (ActionMenuLevel *)s_am_config.root_level;
}
void action_menu_hierarchy_destroy(const ActionMenuLevel *lvl, ActionMenuEachItemCb cb, void *ctx) {
  (void)cb; (void)ctx; free((void *)lvl);
}
int stub_action_count(void) {
  return s_am_open ? ((ActionMenuLevel *)s_am_config.root_level)->n : 0;
}
const char *stub_action_label(int i) {
  return ((ActionMenuLevel *)s_am_config.root_level)->items[i].label;
}
void stub_action_invoke(int i) {
  ActionMenuLevel *lvl = (ActionMenuLevel *)s_am_config.root_level;
  struct ActionMenuItem *it = &lvl->items[i];
  it->cb((ActionMenu *)&s_am_config, it, s_am_config.context);
  if (s_am_config.did_close)
    s_am_config.did_close((ActionMenu *)&s_am_config, it, s_am_config.context);
  s_am_open = false;
}

// ---------------- persist ----------------
typedef struct { uint32_t key; bool used; bool is_int; int32_t iv; uint8_t data[256]; size_t len; } PSlot;
#define PSLOTS 64
static PSlot s_persist[PSLOTS];

static PSlot *pfind(uint32_t key, bool create) {
  for (int i = 0; i < PSLOTS; i++)
    if (s_persist[i].used && s_persist[i].key == key) return &s_persist[i];
  if (!create) return NULL;
  for (int i = 0; i < PSLOTS; i++)
    if (!s_persist[i].used) { s_persist[i].used = true; s_persist[i].key = key; return &s_persist[i]; }
  return NULL;
}
bool persist_exists(uint32_t key) { return pfind(key, false) != NULL; }
int32_t persist_read_int(uint32_t key) { PSlot *p = pfind(key, false); return p ? p->iv : 0; }
void persist_write_int(uint32_t key, int32_t v) { PSlot *p = pfind(key, true); if (p) { p->is_int = true; p->iv = v; } }
int persist_read_data(uint32_t key, void *out, size_t n) {
  PSlot *p = pfind(key, false);
  if (!p) return -1;
  const size_t c = n < p->len ? n : p->len;
  memcpy(out, p->data, c);
  return (int)c;
}
int persist_write_data(uint32_t key, const void *in, size_t n) {
  if (n > 256) return -2;   // E_INVALID_ARGUMENT on real HW
  PSlot *p = pfind(key, true);
  if (!p) return -3;
  memcpy(p->data, in, n); p->len = n;
  return (int)n;
}
void persist_delete(uint32_t key) { PSlot *p = pfind(key, false); if (p) p->used = false; }

// ---------------- app message ----------------
static AppMessageInboxReceived s_inbox_cb;
void app_message_register_inbox_received(AppMessageInboxReceived cb) { s_inbox_cb = cb; }
void app_message_register_inbox_dropped(AppMessageInboxDropped cb) { (void)cb; }
void app_message_deregister_callbacks(void) { s_inbox_cb = NULL; }
AppMessageResult app_message_open(uint32_t in, uint32_t out) { (void)in; (void)out; return APP_MSG_OK; }
static struct DictionaryIterator s_outbox;
AppMessageResult app_message_outbox_begin(DictionaryIterator **it) { s_outbox.n = 0; *it = &s_outbox; return APP_MSG_OK; }
int dict_write_uint8(DictionaryIterator *it, uint32_t key, uint8_t v) { (void)it; (void)key; (void)v; return 0; }
AppMessageResult app_message_outbox_send(void) { return APP_MSG_OK; }

Tuple *dict_find(const DictionaryIterator *it, uint32_t key) {
  for (int i = 0; i < it->n; i++) if (it->t[i]->key == key) return it->t[i];
  return NULL;
}
void stub_deliver_inbox(DictionaryIterator *it) { if (s_inbox_cb) s_inbox_cb(it, NULL); }

Tuple *mock_tuple_cstring(uint32_t key, const char *s) {
  Tuple *t = calloc(1, sizeof(Tuple) + strlen(s) + 1);
  t->key = key; t->length = (uint16_t)(strlen(s) + 1);
  t->value->cstring = strdup(s);
  return t;
}
Tuple *mock_tuple_uint(uint32_t key, uint32_t v) {
  Tuple *t = calloc(1, sizeof(Tuple) + 4);
  t->key = key; t->length = 4;
  memcpy((void *)t->value, &v, 4);   // LE: uint8/uint16/uint32 reads all valid
  return t;
}
Tuple *mock_tuple_data(uint32_t key, const uint8_t *d, uint16_t len) {
  Tuple *t = calloc(1, sizeof(Tuple) + len);
  t->key = key; t->length = len;
  memcpy((void *)t->value->data, d, len);
  return t;
}
void mock_dict_free(DictionaryIterator *it) {
  for (int i = 0; i < it->n; i++) free(it->t[i]);
  it->n = 0;
}

// ---------------- misc ----------------
void vibes_short_pulse(void) {}
void light_enable_interaction(void) {}
void app_event_loop(void) {}
