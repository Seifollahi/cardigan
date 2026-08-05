// Minimal Pebble SDK header stub for HOST-SIDE TESTING ONLY.
// The real build uses the SDK's pebble.h; this mirrors the subset of
// the API the app uses so the logic can be compiled and tested on a
// dev machine (see test/pebble_stub.c for the behavioral half).
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#define PERSIST_DATA_MAX_LENGTH 256
#define STATUS_BAR_LAYER_HEIGHT 16

typedef union GColor8 { uint8_t argb; } GColor8;
typedef GColor8 GColor;
#define GColorBlack     ((GColor){.argb=0xC0})
#define GColorWhite     ((GColor){.argb=0xFF})
#define GColorLightGray ((GColor){.argb=0xEA})

#ifdef PBL_COLOR
#define PBL_IF_COLOR_ELSE(a,b) (a)
#else
#define PBL_IF_COLOR_ELSE(a,b) (b)
#endif

typedef struct { int16_t x, y; } GPoint;
typedef struct { int16_t w, h; } GSize;
typedef struct { GPoint origin; GSize size; } GRect;
#define GPoint(x,y) ((GPoint){(x),(y)})
#define GSize(w,h) ((GSize){(w),(h)})
#define GRect(x,y,w,h) ((GRect){{(x),(y)},{(w),(h)}})
typedef enum { GCornerNone=0, GCornersAll=15 } GCornerMask;

typedef struct Window Window;
typedef struct Layer Layer;
typedef struct GContext GContext;
typedef struct MenuLayer MenuLayer;
typedef struct StatusBarLayer StatusBarLayer;
typedef struct ActionMenu ActionMenu;
typedef struct ActionMenuLevel ActionMenuLevel;
typedef struct ActionMenuItem ActionMenuItem;
typedef struct GFont_t *GFont;
typedef void *ClickRecognizerRef;

typedef enum { BUTTON_ID_BACK, BUTTON_ID_UP, BUTTON_ID_SELECT, BUTTON_ID_DOWN } ButtonId;
typedef enum { GTextOverflowModeWordWrap, GTextOverflowModeTrailingEllipsis, GTextOverflowModeFill } GTextOverflowMode;
typedef enum { GTextAlignmentLeft, GTextAlignmentCenter, GTextAlignmentRight } GTextAlignment;
typedef struct { uint16_t section; uint16_t row; } MenuIndex;

#define FONT_KEY_GOTHIC_14 "g14"
#define FONT_KEY_GOTHIC_18 "g18"
#define FONT_KEY_GOTHIC_18_BOLD "g18b"
#define FONT_KEY_GOTHIC_24_BOLD "g24b"
#define FONT_KEY_LECO_36_BOLD_NUMBERS "l36"
GFont fonts_get_system_font(const char *key);

void graphics_context_set_fill_color(GContext*, GColor);
void graphics_context_set_text_color(GContext*, GColor);
void graphics_fill_rect(GContext*, GRect, uint16_t, GCornerMask);
void graphics_fill_circle(GContext*, GPoint, uint16_t);
void graphics_context_set_stroke_color(GContext*, GColor);
void graphics_context_set_stroke_width(GContext*, uint8_t);
void graphics_draw_circle(GContext*, GPoint, uint16_t);
void graphics_draw_line(GContext*, GPoint, GPoint);
typedef struct GTextAttributes GTextAttributes;
void graphics_draw_text(GContext*, const char*, GFont, const GRect,
                        GTextOverflowMode, GTextAlignment, GTextAttributes*);

typedef void (*LayerUpdateProc)(Layer*, GContext*);
Layer *layer_create(GRect);
void layer_destroy(Layer*);
void layer_set_update_proc(Layer*, LayerUpdateProc);
void layer_add_child(Layer*, Layer*);
void layer_mark_dirty(Layer*);
GRect layer_get_bounds(const Layer*);

typedef struct { void (*load)(Window*); void (*appear)(Window*);
                 void (*disappear)(Window*); void (*unload)(Window*); } WindowHandlers;
typedef void (*ClickConfigProvider)(void*);
typedef void (*ClickHandler)(ClickRecognizerRef, void*);
Window *window_create(void);
void window_destroy(Window*);
void window_set_window_handlers(Window*, WindowHandlers);
void window_set_background_color(Window*, GColor);
void window_set_click_config_provider(Window*, ClickConfigProvider);
Layer *window_get_root_layer(const Window*);
void window_stack_push(Window*, bool);
bool window_stack_remove(Window*, bool);
void window_single_click_subscribe(ButtonId, ClickHandler);
void window_long_click_subscribe(ButtonId, uint16_t, ClickHandler, ClickHandler);

typedef struct {
  uint16_t (*get_num_sections)(MenuLayer*, void*);
  uint16_t (*get_num_rows)(MenuLayer*, uint16_t, void*);
  int16_t (*get_cell_height)(MenuLayer*, MenuIndex*, void*);
  int16_t (*get_header_height)(MenuLayer*, uint16_t, void*);
  void (*draw_row)(GContext*, const Layer*, MenuIndex*, void*);
  void (*draw_header)(GContext*, const Layer*, uint16_t, void*);
  void (*select_click)(MenuLayer*, MenuIndex*, void*);
  void (*select_long_click)(MenuLayer*, MenuIndex*, void*);
  void (*selection_changed)(MenuLayer*, MenuIndex, MenuIndex, void*);
} MenuLayerCallbacks;
MenuLayer *menu_layer_create(GRect);
void menu_layer_destroy(MenuLayer*);
void menu_layer_set_callbacks(MenuLayer*, void*, MenuLayerCallbacks);
void menu_layer_set_click_config_onto_window(MenuLayer*, Window*);
void menu_layer_set_highlight_colors(MenuLayer*, GColor, GColor);
void menu_layer_reload_data(MenuLayer*);
bool menu_layer_is_index_selected(MenuLayer*, MenuIndex*);
Layer *menu_layer_get_layer(const MenuLayer*);
bool menu_cell_layer_is_highlighted(const Layer*);

typedef enum { StatusBarLayerSeparatorModeNone, StatusBarLayerSeparatorModeDotted } StatusBarLayerSeparatorMode;
StatusBarLayer *status_bar_layer_create(void);
void status_bar_layer_destroy(StatusBarLayer*);
void status_bar_layer_set_colors(StatusBarLayer*, GColor, GColor);
void status_bar_layer_set_separator_mode(StatusBarLayer*, StatusBarLayerSeparatorMode);
Layer *status_bar_layer_get_layer(StatusBarLayer*);

typedef void (*ActionMenuPerformActionCb)(ActionMenu*, const ActionMenuItem*, void*);
typedef void (*ActionMenuDidCloseCb)(ActionMenu*, const ActionMenuItem*, void*);
typedef enum { ActionMenuAlignTop, ActionMenuAlignCenter } ActionMenuAlign;
typedef struct {
  const ActionMenuLevel *root_level; void *context;
  struct { GColor background; GColor foreground; } colors;
  ActionMenuDidCloseCb will_close; ActionMenuDidCloseCb did_close;
  ActionMenuAlign align;
} ActionMenuConfig;
ActionMenuLevel *action_menu_level_create(uint16_t);
ActionMenuItem *action_menu_level_add_action(ActionMenuLevel*, const char*, ActionMenuPerformActionCb, void*);
ActionMenu *action_menu_open(ActionMenuConfig*);
void *action_menu_item_get_action_data(const ActionMenuItem*);
ActionMenuLevel *action_menu_get_root_level(ActionMenu*);
typedef void (*ActionMenuEachItemCb)(const ActionMenuItem*, void*);
void action_menu_hierarchy_destroy(const ActionMenuLevel*, ActionMenuEachItemCb, void*);

bool persist_exists(uint32_t);
int32_t persist_read_int(uint32_t);
void persist_write_int(uint32_t, int32_t);
int persist_read_data(uint32_t, void*, size_t);
int persist_write_data(uint32_t, const void*, size_t);
void persist_delete(uint32_t);

typedef struct DictionaryIterator DictionaryIterator;
typedef struct Tuple {
  uint32_t key; uint16_t length;
  union { const char *cstring; uint8_t data[1];
          uint8_t uint8; uint16_t uint16; uint32_t uint32;
          int8_t int8; int16_t int16; int32_t int32; } value[1];
} Tuple;
typedef enum { APP_MSG_OK = 0, APP_MSG_SEND_TIMEOUT } AppMessageResult;
typedef void (*AppMessageInboxReceived)(DictionaryIterator*, void*);
typedef void (*AppMessageInboxDropped)(AppMessageResult, void*);
Tuple *dict_find(const DictionaryIterator*, uint32_t);
int dict_write_uint8(DictionaryIterator*, uint32_t, uint8_t);
void app_message_register_inbox_received(AppMessageInboxReceived);
void app_message_register_inbox_dropped(AppMessageInboxDropped);
void app_message_deregister_callbacks(void);
AppMessageResult app_message_open(uint32_t, uint32_t);
AppMessageResult app_message_outbox_begin(DictionaryIterator**);
AppMessageResult app_message_outbox_send(void);

static const uint32_t MESSAGE_KEY_OP=10000, MESSAGE_KEY_INDEX=10001,
  MESSAGE_KEY_TOTAL=10002, MESSAGE_KEY_NAME=10003, MESSAGE_KEY_CODE=10004,
  MESSAGE_KEY_TYPE=10005, MESSAGE_KEY_COLOR=10006, MESSAGE_KEY_FLAGS=10007,
  MESSAGE_KEY_QRSIZE=10008, MESSAGE_KEY_POINTS=10009, MESSAGE_KEY_DATA=10010;

#define APP_LOG_LEVEL_ERROR 1
#define APP_LOG_LEVEL_WARNING 2
#define APP_LOG(level, fmt, ...) ((void)printf(fmt "\n", ##__VA_ARGS__))
void vibes_short_pulse(void);
void light_enable_interaction(void);
void light_enable(bool enable);
void app_event_loop(void);

typedef struct AppTimer AppTimer;
typedef void (*AppTimerCallback)(void*);
AppTimer *app_timer_register(uint32_t timeout_ms, AppTimerCallback callback, void *callback_data);
bool app_timer_cancel(AppTimer *timer);

typedef enum { ACCEL_AXIS_X, ACCEL_AXIS_Y, ACCEL_AXIS_Z } AccelAxisType;
typedef void (*AccelTapHandler)(AccelAxisType axis, int32_t direction);
void accel_tap_service_subscribe(AccelTapHandler handler);
void accel_tap_service_unsubscribe(void);

