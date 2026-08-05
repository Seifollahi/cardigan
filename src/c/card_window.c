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

static AppTimer *s_light_timer;

static void light_timer_cb(void *data) {
  light_enable(false);
  s_light_timer = NULL;
  if (s_layer) layer_mark_dirty(s_layer);
}

static void set_till_mode(bool enable) {
  if (s_light_timer) {
    app_timer_cancel(s_light_timer);
    s_light_timer = NULL;
  }
  if (enable && s_page == 0) {
    light_enable(true);
    s_light_timer = app_timer_register(30000, light_timer_cb, NULL);
  } else {
    light_enable(false);
  }
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  if (s_page == 0) {
    set_till_mode(true);
    if (s_layer) layer_mark_dirty(s_layer);
  }
}

#if defined(PBL_ROUND)
// Integer square root (no FPU on Pebble; avoids linking libm).
static int isqrt_i(int n) {
  if (n <= 0) return 0;
  int x = n, y = (x + 1) / 2;
  while (y < x) { x = y; y = (x + n / x) / 2; }
  return x;
}
#endif

// On a circular display the usable width is the chord at the widest row
// the content occupies — not the diameter. Shrink `area` to the largest
// rectangle of the same height that clears the bezel.
static GRect fit_round(GRect bounds, GRect area) {
#if defined(PBL_ROUND)
  const int r  = bounds.size.w / 2 - 3;             // 3px bezel margin
  const int cy = bounds.size.h / 2;
  // farthest edge of the content from the vertical centre
  int dy = area.origin.y - cy;
  if (dy < 0) dy = -dy;
  int dy2 = (area.origin.y + area.size.h) - cy;
  if (dy2 < 0) dy2 = -dy2;
  if (dy2 > dy) dy = dy2;
  if (dy >= r) dy = r - 1;

  const int half = isqrt_i(r * r - dy * dy);
  const int max_w = half * 2;
  if (area.size.w > max_w) {
    area.origin.x += (area.size.w - max_w) / 2;
    area.size.w = max_w;
  }
#else
  (void)bounds;
#endif
  return area;
}

// Largest code rectangle (w x h, centred on the display) that clears the
// bezel. On round displays the diagonal is what matters: a rectangle fits
// the circle iff (w/2)^2 + (h/2)^2 <= r^2. Returns the max width for the
// given height, capped by the rectangular area.
static int max_code_w(GRect bounds, GRect area, int h) {
#if defined(PBL_ROUND)
  const int r = bounds.size.w / 2 - 3;
  int half_h = h / 2;
  if (half_h >= r) return 0;
  const int w = isqrt_i(r * r - half_h * half_h) * 2;
  return w < area.size.w ? w : area.size.w;
#else
  (void)bounds; (void)h;
  return area.size.w;
#endif
}

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

// Draws the barcode at the largest scale that fits *without clipping*,
// choosing horizontal or rotated orientation. A clipped barcode is worse
// than none — a scanner may read a truncated number — so this returns
// false and draws nothing when the code cannot fit the display.
static bool draw_barcode(GContext *ctx, const Card *c, GRect area, GRect bounds) {
  const int units = barcode_units(c);
  if (units <= 0) return false;

  const int pad = 6;

  // --- horizontal: bars across, thickness = a share of the area height
  int bar_h = (area.size.h * 6) / 10;
  if (bar_h > 72) bar_h = 72;
  const int avail_w = max_code_w(bounds, area, bar_h) - 2 * pad;
  int h_scale = (avail_w > 0) ? avail_w / units : 0;

  // --- rotated: bars stacked, thickness = a share of the area width
  int bar_w = (area.size.w * 7) / 10;
  if (bar_w > 84) bar_w = 84;
  // for the rotated case the code's length runs vertically: the circle
  // constraint swaps, so ask for the max "width" at thickness bar_w and
  // use it as the available length.
  int avail_h = max_code_w(bounds, GRect(0, 0, area.size.h, 0), bar_w) - 2 * pad;
  if (avail_h > area.size.h - 2 * pad) avail_h = area.size.h - 2 * pad;
  int v_scale = (avail_h > 0) ? avail_h / units : 0;

  const bool rotated = (h_scale < 1) && (v_scale >= 1);
  int scale = rotated ? v_scale : h_scale;
  if (scale < 1) return false;            // cannot render legibly
  if (scale > 3) scale = 3;               // wider bars gain nothing

  graphics_context_set_fill_color(ctx, GColorBlack);
  const int cx = bounds.size.w / 2;
  const int total = units * scale;

  if (!rotated) {
    int x = cx - total / 2;
    const int y = area.origin.y + (area.size.h - bar_h) / 2;
    for (uint16_t i = 0; i < c->data_len; i++) {
      const int w = c->data[i] * scale;
      if (!(i & 1)) graphics_fill_rect(ctx, GRect(x, y, w, bar_h), 0, GCornerNone);
      x += w;
    }
  } else {
    int y = area.origin.y + (area.size.h - total) / 2;
    const int x = cx - bar_w / 2;
    for (uint16_t i = 0; i < c->data_len; i++) {
      const int h = c->data[i] * scale;
      if (!(i & 1)) graphics_fill_rect(ctx, GRect(x, y, bar_w, h), 0, GCornerNone);
      y += h;
    }
  }
  return true;
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
  if (s_light_timer && s_page == 0) {
    int sx = bounds.size.w - 14;
    if (c->flags & CARD_FLAG_FAV) sx -= 12;
    graphics_context_set_stroke_color(ctx, fg);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_circle(ctx, GPoint(sx, hh / 2), 3);
    graphics_draw_line(ctx, GPoint(sx, hh/2 - 5), GPoint(sx, hh/2 - 4));
    graphics_draw_line(ctx, GPoint(sx, hh/2 + 5), GPoint(sx, hh/2 + 4));
    graphics_draw_line(ctx, GPoint(sx - 5, hh/2), GPoint(sx - 4, hh/2));
    graphics_draw_line(ctx, GPoint(sx + 5, hh/2), GPoint(sx + 4, hh/2));
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
    const int units = barcode_units(c);
    // Would the code fit in the normal layout (either orientation)?
    const GRect std = fit_round(bounds,
        GRect(2, hh + 4, bounds.size.w - 14, bounds.size.h - hh - 30));
    int bar_h = (std.size.h * 6) / 10;
    if (bar_h > 72) bar_h = 72;
    const int fits_w = max_code_w(bounds, std, bar_h) - 12;
    const int fits_h = std.size.h - 12;
    if (units > fits_w && units > fits_h) { tall = true; hh = 16; }
  }

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  draw_header(ctx, c, bounds, hh);

  const GRect body = GRect(0, hh, bounds.size.w - 10, bounds.size.h - hh);

  if (s_page == 0 && tall) {
    const GRect code_area = fit_round(bounds,
        GRect(2, hh + 2, bounds.size.w - 14, bounds.size.h - hh - 4));
    draw_barcode(ctx, c, code_area, bounds);
  } else if (s_page == 0) {
    const int text_h = 20;
    const GRect code_area = fit_round(bounds,
        GRect(body.origin.x + 2, body.origin.y + 4,
              body.size.w - 4, body.size.h - text_h - 10));
    bool drawn;
    if (c->type == CARD_TYPE_QR) { draw_qr(ctx, c, code_area); drawn = true; }
    else                         drawn = draw_barcode(ctx, c, code_area, bounds);

    graphics_context_set_text_color(ctx, GColorBlack);
    if (!drawn) {
      // Too long for this display — show the number large enough to read
      // aloud rather than a truncated, mis-scannable barcode.
      graphics_draw_text(ctx, c->code,
                         fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                         GRect(6, code_area.origin.y + code_area.size.h / 2 - 20,
                               bounds.size.w - 12, 56),
                         GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
      graphics_draw_text(ctx, "Code too long for display",
                         fonts_get_system_font(FONT_KEY_GOTHIC_14),
                         GRect(0, code_area.origin.y + code_area.size.h / 2 + 10,
                               bounds.size.w, 16),
                         GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }
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
  set_till_mode(s_page == 0);
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
  set_till_mode(true);
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
  accel_tap_service_subscribe(accel_tap_handler);
  set_till_mode(true);
}

static void window_unload(Window *window) {
  set_till_mode(false);
  accel_tap_service_unsubscribe();
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
