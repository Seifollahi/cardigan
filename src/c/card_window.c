#include "wallet.h"

// =====================================================================
// Card window — full-screen custom Layer, integer-only rasterizer.
//
// e-paper friendly: static frame, redrawn only on page flips / card
// switches. Codes render 1-bit black-on-white for maximum scanner
// contrast. Barcodes auto-rotate 90° when the pixel budget of the
// display width can't fit one module per unit (emery: 200 wide, but
// 228 tall rotated).
// =====================================================================

static Window *s_window;
static Layer  *s_layer;
static uint8_t s_index;
static uint8_t s_page;      // 0 = code, 1 = info
static bool    s_quick;
static ActionMenuLevel *s_menu_root;

static int header_h(GRect b) { return b.size.h >= 200 ? 40 : 34; }

static bool color_is_dark(GColor c) {
#if defined(PBL_COLOR)
  // perceived luminance on 2-bit channels
  const int r = (c.argb >> 4) & 3, g = (c.argb >> 2) & 3, b = c.argb & 3;
  return (r * 30 + g * 59 + b * 11) < 150;   // max = 300
#else
  (void)c;
  return true;
#endif
}

// ---------- code rasterizers ----------

static int barcode_units(const Card *c) {
  int units = 0;
  for (uint16_t i = 0; i < c->data_len; i++) units += c->data[i];
  return units;
}

static void draw_barcode(GContext *ctx, const Card *c, GRect area) {
  const int units = barcode_units(c);
  if (units <= 0) return;

  const int pad = 6;
  int scale = (area.size.w - 2 * pad) / units;
  bool rotated = false;
  if (scale < 1) {
    const int vscale = (area.size.h - 2 * pad) / units;
    if (vscale >= 1) { rotated = true; scale = vscale; }
    else scale = 1;                       // last resort: clip
  }
  if (scale > 3) scale = 3;               // don't waste quiet zone

  graphics_context_set_fill_color(ctx, GColorBlack);

  if (!rotated) {
    const int total = units * scale;
    const int bar_h = (area.size.h * 6) / 10;
    int x = area.origin.x + (area.size.w - total) / 2;
    if (x < area.origin.x) x = area.origin.x;
    const int y = area.origin.y + (area.size.h - bar_h) / 2;
    for (uint16_t i = 0; i < c->data_len; i++) {
      const int w = c->data[i] * scale;
      if (!(i & 1)) graphics_fill_rect(ctx, GRect(x, y, w, bar_h), 0, GCornerNone);
      x += w;
      if (x > area.origin.x + area.size.w) break;
    }
  } else {
    const int total = units * scale;
    const int bar_w = (area.size.w * 7) / 10;
    int y = area.origin.y + (area.size.h - total) / 2;
    if (y < area.origin.y) y = area.origin.y;
    const int x = area.origin.x + (area.size.w - bar_w) / 2;
    for (uint16_t i = 0; i < c->data_len; i++) {
      const int h = c->data[i] * scale;
      if (!(i & 1)) graphics_fill_rect(ctx, GRect(x, y, bar_w, h), 0, GCornerNone);
      y += h;
      if (y > area.origin.y + area.size.h) break;
    }
  }
}

static bool qr_module(const Card *c, int row, int col) {
  const int bit = row * c->qr_size + col;
  if ((bit >> 3) >= (int)c->data_len) return false;
  return (c->data[bit >> 3] >> (7 - (bit & 7))) & 1;
}

static void draw_qr(GContext *ctx, const Card *c, GRect area) {
  const int n = c->qr_size;
  if (n < 21 || n > 29) return;
  const int quiet = 8;                          // >= 4 modules of quiet zone
  int m = (area.size.w - quiet) / n;
  const int mv = (area.size.h - quiet) / n;
  if (mv < m) m = mv;
  if (m < 2) m = 2;                             // readability floor
  const int total = n * m;
  const int x0 = area.origin.x + (area.size.w - total) / 2;
  const int y0 = area.origin.y + (area.size.h - total) / 2;
  graphics_context_set_fill_color(ctx, GColorBlack);
  for (int r = 0; r < n; r++) {
    for (int col = 0; col < n; col++) {
      if (qr_module(c, r, col)) {
        graphics_fill_rect(ctx, GRect(x0 + col * m, y0 + r * m, m, m), 0, GCornerNone);
      }
    }
  }
}

// ---------- layer update ----------

static void draw_header(GContext *ctx, const Card *c, GRect bounds, int hh) {
  const GColor bg = card_color(c);
  const GColor fg = color_is_dark(bg) ? GColorWhite : GColorBlack;
  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, hh), 0, GCornerNone);
  graphics_context_set_text_color(ctx, fg);
  const bool small = hh < 24;
  graphics_draw_text(ctx, c->name,
                     fonts_get_system_font(small ? FONT_KEY_GOTHIC_14
                                                 : FONT_KEY_GOTHIC_18_BOLD),
                     small ? GRect(6, -2, bounds.size.w - 24, 16)
                           : GRect(6, (hh - 24) / 2, bounds.size.w - 34, 24),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  if (c->flags & CARD_FLAG_FAV) {
    graphics_context_set_fill_color(ctx, fg);
    graphics_fill_circle(ctx, GPoint(bounds.size.w - 14, hh / 2), 4);
  }
  if (s_quick) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(0, hh, 58, 13), 0, GCornerNone);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, "QUICK",
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(0, hh - 3, 58, 14),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }
}

static void draw_dots(GContext *ctx, GRect bounds) {
  const int cx = bounds.size.w - 5;
  const int cy = bounds.size.h / 2;
  for (int i = 0; i < 2; i++) {
    graphics_context_set_fill_color(ctx, i == s_page ? GColorBlack
                                    : PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack));
    const int r = i == s_page ? 3 : 2;
    graphics_fill_circle(ctx, GPoint(cx, cy - 7 + i * 14), r);
  }
}

static void layer_update(Layer *layer, GContext *ctx) {
  const Card *c = storage_get(s_index);
  if (!c) return;
  const GRect bounds = layer_get_bounds(layer);
  int hh = header_h(bounds);

  // "Tall mode": if a barcode fits neither horizontally nor rotated in
  // the standard code area, shrink the chrome (thin header, no caption)
  // and give the rotated barcode nearly the full screen height. This is
  // what saves long set-B codes on 144x168 displays.
  bool tall = false;
  if (s_page == 0 && c->type == CARD_TYPE_BARCODE) {
    const int units  = barcode_units(c);
    const int std_w  = (bounds.size.w - 10 - 4) - 12;             // code area - pads
    const int std_h  = (bounds.size.h - hh - 20 - 10) - 12;
    if (units > std_w && units > std_h) { tall = true; hh = 16; }
  }

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  draw_header(ctx, c, bounds, hh);

  const GRect body = GRect(0, hh, bounds.size.w - 10, bounds.size.h - hh);

  if (s_page == 0 && tall) {
    const GRect code_area = GRect(2, hh + 2, bounds.size.w - 14,
                                  bounds.size.h - hh - 4);
    draw_barcode(ctx, c, code_area);
  } else if (s_page == 0) {
    const int text_h = 20;
    const GRect code_area = GRect(body.origin.x + 2, body.origin.y + 4,
                                  body.size.w - 4, body.size.h - text_h - 10);
    if (c->type == CARD_TYPE_QR) draw_qr(ctx, c, code_area);
    else                         draw_barcode(ctx, c, code_area);

    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, c->code,
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(0, bounds.size.h - text_h - 2, body.size.w, text_h),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  } else {
    static char pts[8];
    snprintf(pts, sizeof(pts), "%u", (unsigned)c->points);
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, "BALANCE",
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(0, hh + 8, body.size.w, 16),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    graphics_draw_text(ctx, pts,
                       fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS),
                       GRect(0, hh + 24, body.size.w, 42),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    graphics_draw_text(ctx, "points",
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(0, hh + 68, body.size.w, 16),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    graphics_draw_text(ctx, c->code,
                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(4, bounds.size.h - 22, body.size.w - 8, 20),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
  draw_dots(ctx, bounds);
}

// ---------- actions ----------

static void action_perform(ActionMenu *menu, const ActionMenuItem *item, void *context) {
  const intptr_t action = (intptr_t)action_menu_item_get_action_data(item);
  switch (action) {
    case 0:  // toggle favourite
      storage_toggle_fav(s_index);
      break;
    case 1:  // set as quick card
      storage_set_last_used((int8_t)s_index);
      comm_notify_used(s_index);
      break;
    case 2:  // delete
      storage_delete_card(s_index);
      menu_window_refresh();
      window_stack_remove(s_window, false);
      return;
  }
  menu_window_refresh();
  layer_mark_dirty(s_layer);
}

static void action_menu_closed(ActionMenu *menu, const ActionMenuItem *item, void *context) {
  action_menu_hierarchy_destroy(action_menu_get_root_level(menu), NULL, NULL);
  s_menu_root = NULL;
}

static void open_action_menu(void) {
  const Card *c = storage_get(s_index);
  if (!c) return;
  s_menu_root = action_menu_level_create(3);
  action_menu_level_add_action(s_menu_root,
      (c->flags & CARD_FLAG_FAV) ? "Unfavourite" : "Favourite",
      action_perform, (void *)(intptr_t)0);
  action_menu_level_add_action(s_menu_root, "Set as quick card",
      action_perform, (void *)(intptr_t)1);
  action_menu_level_add_action(s_menu_root, "Delete card",
      action_perform, (void *)(intptr_t)2);

  ActionMenuConfig config = {
    .root_level = s_menu_root,
    .colors = {
      .background = PBL_IF_COLOR_ELSE(card_color(c), GColorBlack),
      .foreground = GColorWhite,
    },
    .align = ActionMenuAlignCenter,
    .did_close = action_menu_closed,
  };
  action_menu_open(&config);
}

// ---------- input ----------

static void flip_page(void) {
  s_page = 1 - s_page;
  light_enable_interaction();
  layer_mark_dirty(s_layer);
}

static void cycle_fav(int dir) {
  const uint8_t n = storage_count();
  if (n < 2) return;
  // collect favourites; fall back to all cards if < 2 favourites
  uint8_t favs[MAX_CARDS], nf = 0;
  for (uint8_t i = 0; i < n; i++) {
    const Card *c = storage_get(i);
    if (c && (c->flags & CARD_FLAG_FAV)) favs[nf++] = i;
  }
  if (nf < 2) { for (nf = 0; nf < n; nf++) favs[nf] = nf; }

  int cur = 0;
  for (int i = 0; i < nf; i++) if (favs[i] == s_index) { cur = i; break; }
  s_index = favs[(cur + dir + nf) % nf];
  s_page = 0;
  storage_set_last_used((int8_t)s_index);
  comm_notify_used(s_index);
  light_enable_interaction();
  layer_mark_dirty(s_layer);
}

static void up_click(ClickRecognizerRef ref, void *ctx)   { if (s_quick) cycle_fav(-1); else flip_page(); }
static void down_click(ClickRecognizerRef ref, void *ctx) { if (s_quick) cycle_fav(+1); else flip_page(); }
static void sel_click(ClickRecognizerRef ref, void *ctx)  { if (s_quick) flip_page(); else open_action_menu(); }
static void sel_long(ClickRecognizerRef ref, void *ctx)   { open_action_menu(); }

static void click_config(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, sel_click);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, sel_long, NULL);
}

// ---------- window lifecycle ----------

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_layer, layer_update);
  layer_add_child(root, s_layer);
  light_enable_interaction();     // brief backlight for the till
}

static void window_unload(Window *window) {
  layer_destroy(s_layer);
  window_destroy(s_window);
  s_window = NULL;
  s_layer = NULL;
}

void card_window_push(uint8_t index, bool quick) {
  if (!storage_get(index)) return;
  s_index = index;
  s_page = 0;
  s_quick = quick;
  storage_set_last_used((int8_t)index);
  comm_notify_used(index);

  s_window = window_create();
  window_set_background_color(s_window, GColorWhite);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load, .unload = window_unload,
  });
  window_set_click_config_provider(s_window, click_config);
  window_stack_push(s_window, true);
}

void card_window_refresh(void) {
  if (!s_window) return;
  if (!storage_count()) { window_stack_remove(s_window, false); return; }
  if (s_index >= storage_count()) s_index = 0;
  layer_mark_dirty(s_layer);
}
