#include "wallet.h"

// =====================================================================
// Card list — MenuLayer with brand-coloured highlight that follows the
// selection (menu_layer_set_highlight_colors on selection change).
// =====================================================================

static Window    *s_window;
static MenuLayer *s_menu;
static StatusBarLayer *s_status;

static bool color_is_dark(GColor c) {
#if defined(PBL_COLOR)
  const int r = (c.argb >> 4) & 3, g = (c.argb >> 2) & 3, b = c.argb & 3;
  return (r * 30 + g * 59 + b * 11) < 150;
#else
  (void)c;
  return true;
#endif
}

static uint16_t get_num_rows(MenuLayer *menu, uint16_t section, void *ctx) {
  const uint8_t n = storage_count();
  return n ? n : 1;                     // one row for the empty state
}

static int16_t get_cell_height(MenuLayer *menu, MenuIndex *index, void *ctx) {
  // Constant height: MenuLayer caches cell heights and does not
  // re-measure them on selection change, so selection-dependent
  // heights make the highlight drift out of sync while scrolling.
  if (!storage_count()) return 60;
  return 46;
}

static void draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *index, void *data) {
  const GRect bounds = layer_get_bounds(cell_layer);

  if (!storage_count()) {
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, "No cards yet\nAdd from the phone app",
                       fonts_get_system_font(FONT_KEY_GOTHIC_18),
                       GRect(4, 4, bounds.size.w - 8, bounds.size.h - 8),
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    return;
  }

  const Card *c = storage_get((uint8_t)index->row);
  if (!c) return;
  const bool sel = menu_cell_layer_is_highlighted(cell_layer);
  const GColor brand = card_color(c);
  const GColor fg = sel ? (color_is_dark(brand) ? GColorWhite : GColorBlack)
                        : GColorBlack;

  // brand chip
  const int chip = 28;
  const int cy = (bounds.size.h - chip) / 2;
  graphics_context_set_fill_color(ctx, sel ? fg : brand);
  graphics_fill_rect(ctx, GRect(6, cy, chip, chip), 6, GCornersAll);

  // initial letter on the chip
  static char ini[2];
  ini[0] = c->name[0] ? c->name[0] : '?';
  ini[1] = '\0';
  graphics_context_set_text_color(ctx, sel ? brand
      : (color_is_dark(brand) ? GColorWhite : GColorBlack));
  graphics_draw_text(ctx, ini,
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(6, cy + 1, chip, chip),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // name + code preview
  graphics_context_set_text_color(ctx, fg);
  graphics_draw_text(ctx, c->name,
                     fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(chip + 12, cy - 4, bounds.size.w - chip - 30, 22),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, c->code,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(chip + 12, cy + 15, bounds.size.w - chip - 30, 16),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // favourite marker
  if (c->flags & CARD_FLAG_FAV) {
    graphics_context_set_fill_color(ctx, fg);
    graphics_fill_circle(ctx, GPoint(bounds.size.w - 10, bounds.size.h / 2), 3);
  }
}

static void selection_changed(MenuLayer *menu, MenuIndex new_index,
                              MenuIndex old_index, void *ctx) {
#if defined(PBL_COLOR)
  const Card *c = storage_get((uint8_t)new_index.row);
  if (c) {
    const GColor brand = card_color(c);
    menu_layer_set_highlight_colors(menu, brand,
        color_is_dark(brand) ? GColorWhite : GColorBlack);
  }
#endif
}

static void select_click(MenuLayer *menu, MenuIndex *index, void *ctx) {
  if (!storage_count()) { comm_request_sync(); return; }
  card_window_push((uint8_t)index->row, false);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_status = status_bar_layer_create();
  status_bar_layer_set_colors(s_status, GColorBlack, GColorWhite);
  status_bar_layer_set_separator_mode(s_status, StatusBarLayerSeparatorModeNone);
  layer_add_child(root, status_bar_layer_get_layer(s_status));

  const GRect menu_bounds = GRect(0, STATUS_BAR_LAYER_HEIGHT, bounds.size.w,
                                  bounds.size.h - STATUS_BAR_LAYER_HEIGHT);
  s_menu = menu_layer_create(menu_bounds);
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
    .get_num_rows = get_num_rows,
    .get_cell_height = get_cell_height,
    .draw_row = draw_row,
    .select_click = select_click,
    .selection_changed = selection_changed,
  });
#if defined(PBL_COLOR)
  {
    const Card *c = storage_get(0);
    if (c) {
      const GColor brand = card_color(c);
      menu_layer_set_highlight_colors(s_menu, brand,
          color_is_dark(brand) ? GColorWhite : GColorBlack);
    }
  }
#endif
  menu_layer_set_click_config_onto_window(s_menu, window);
  layer_add_child(root, menu_layer_get_layer(s_menu));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu);
  status_bar_layer_destroy(s_status);
  window_destroy(s_window);
  s_window = NULL;
  s_menu = NULL;
}

void menu_window_push(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load, .unload = window_unload,
  });
  window_stack_push(s_window, true);
}

void menu_window_refresh(void) {
  if (s_menu) menu_layer_reload_data(s_menu);
}
