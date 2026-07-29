#include "wallet.h"

// =====================================================================
// Pebble Wallet — entry point
//
// One-press checkout: the app launches straight into the last-used
// card ("quick card") on top of the list, so Select from the launcher
// puts a scannable code on screen in a single interaction. Back
// reveals the full list; Back again exits.
// =====================================================================

static void init(void) {
  storage_init();
  comm_init();

  menu_window_push();

  const int8_t last = storage_last_used();
  if (last >= 0) {
    card_window_push((uint8_t)last, true /* quick mode */);
  } else if (storage_count() == 0) {
    comm_request_sync();     // fresh install: ask the phone for cards
  }
}

static void deinit(void) {
  comm_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}
