// Host-test extensions to the pebble.h stub: behavioral hooks the
// harness uses to drive the app like the OS would.
#pragma once
#include <pebble.h>

// mock AppMessage dictionaries
struct DictionaryIterator { int n; Tuple *t[16]; };
Tuple *mock_tuple_cstring(uint32_t key, const char *s);
Tuple *mock_tuple_uint(uint32_t key, uint32_t v);
Tuple *mock_tuple_data(uint32_t key, const uint8_t *d, uint16_t len);
void   mock_dict_free(DictionaryIterator *it);
void   stub_deliver_inbox(DictionaryIterator *it);   // -> registered handler

// display / rendering
void stub_set_display(int w, int h);
int  stub_render_pgm(const char *path);              // draw top window layers

// input
void stub_click(ButtonId b);
void stub_long_click(ButtonId b);

// window stack
int  stub_stack_depth(void);
void stub_pop_top(void);                             // unload + remove top

// action menu capture
int  stub_action_count(void);
const char *stub_action_label(int i);
void stub_action_invoke(int i);                      // perform + did_close
