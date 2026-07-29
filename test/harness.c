// Functional test harness: drives the real watchapp code (storage.c,
// comm.c, menu_window.c, card_window.c) through the behavioral stub.
//
//   harness <width> <height> <out_prefix> [--logic]
//
// 1. Replays the AppMessage stream captured from the real pkjs code
//    (test/out/cards.txt) through comm.c's inbox handler.
// 2. Renders every card's code page to PGM for scanner verification.
// 3. With --logic: exercises clicks, quick-mode cycling, the action
//    menu (favourite/quick/delete), and persistence across "reboots".
#include "stub.h"
#include "wallet.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int s_checks = 0, s_fails = 0;
#define CHECK(cond, ...) do { s_checks++; \
  if (!(cond)) { s_fails++; printf("  FAIL: " __VA_ARGS__); printf("\n"); } \
  else { printf("  ok: " __VA_ARGS__); printf("\n"); } } while (0)

static void deliver_op(uint8_t op, int total) {
  struct DictionaryIterator d = { .n = 0 };
  d.t[d.n++] = mock_tuple_uint(MESSAGE_KEY_OP, op);
  if (total >= 0) d.t[d.n++] = mock_tuple_uint(MESSAGE_KEY_TOTAL, (uint32_t)total);
  stub_deliver_inbox(&d);
  mock_dict_free(&d);
}

static int hexval(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 0;
}

// Replays cards.txt; returns number of CARD messages delivered.
static int replay_sync(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) { fprintf(stderr, "cannot open %s\n", path); exit(2); }
  char line[1024];
  int ncards = 0;
  while (fgets(line, sizeof(line), f)) {
    line[strcspn(line, "\r\n")] = 0;
    if (!strncmp(line, "BEGIN ", 6)) {
      deliver_op(OP_SYNC_BEGIN, atoi(line + 6));
    } else if (!strcmp(line, "END")) {
      deliver_op(OP_SYNC_END, -1);
    } else if (!strncmp(line, "CARD ", 5)) {
      // CARD idx|name|code|type|color|flags|qrsize|points|hex
      char *fields[9] = {0};
      int nf = 0;
      for (char *tok = strtok(line, "|"); tok && nf < 9; tok = strtok(NULL, "|"))
        fields[nf++] = tok;
      if (nf != 9) { fprintf(stderr, "bad CARD line (%d fields)\n", nf); continue; }

      static uint8_t data[512];
      const char *hex = fields[8];
      size_t dlen = strlen(hex) / 2;
      if (dlen > sizeof(data)) dlen = sizeof(data);
      for (size_t i = 0; i < dlen; i++)
        data[i] = (uint8_t)((hexval(hex[2 * i]) << 4) | hexval(hex[2 * i + 1]));

      struct DictionaryIterator d = { .n = 0 };
      d.t[d.n++] = mock_tuple_uint(MESSAGE_KEY_OP, OP_CARD);
      d.t[d.n++] = mock_tuple_uint(MESSAGE_KEY_INDEX, (uint32_t)atoi(fields[0] + 5));
      d.t[d.n++] = mock_tuple_cstring(MESSAGE_KEY_NAME, fields[1]);
      d.t[d.n++] = mock_tuple_cstring(MESSAGE_KEY_CODE, fields[2]);
      d.t[d.n++] = mock_tuple_uint(MESSAGE_KEY_TYPE, (uint32_t)atoi(fields[3]));
      d.t[d.n++] = mock_tuple_uint(MESSAGE_KEY_COLOR, (uint32_t)atoi(fields[4]));
      d.t[d.n++] = mock_tuple_uint(MESSAGE_KEY_FLAGS, (uint32_t)atoi(fields[5]));
      d.t[d.n++] = mock_tuple_uint(MESSAGE_KEY_QRSIZE, (uint32_t)atoi(fields[6]));
      d.t[d.n++] = mock_tuple_uint(MESSAGE_KEY_POINTS, (uint32_t)atoi(fields[7]));
      d.t[d.n++] = mock_tuple_data(MESSAGE_KEY_DATA, data, (uint16_t)dlen);
      stub_deliver_inbox(&d);
      mock_dict_free(&d);
      ncards++;
    }
  }
  fclose(f);
  return ncards;
}

static void render_card(int i, const char *prefix) {
  char path[256];
  card_window_push((uint8_t)i, false);
  snprintf(path, sizeof(path), "%scard_%d.pgm", prefix, i);
  stub_render_pgm(path);
  stub_pop_top();
}

int main(int argc, char **argv) {
  if (argc < 4) { fprintf(stderr, "usage: %s W H out_prefix [--logic]\n", argv[0]); return 2; }
  const int W = atoi(argv[1]), H = atoi(argv[2]);
  const char *prefix = argv[3];
  const bool logic = argc > 4 && !strcmp(argv[4], "--logic");

  stub_set_display(W, H);
  printf("== boot (%dx%d) ==\n", W, H);
  storage_init();
  comm_init();
  menu_window_push();
  CHECK(storage_count() == 0, "fresh install has 0 cards");

  printf("== sync from phone (replay of real pkjs stream) ==\n");
  const int sent = replay_sync("test/out/cards.txt");
  CHECK(storage_count() == sent, "all %d cards committed (have %u)", sent, storage_count());

  printf("== render code pages ==\n");
  for (int i = 0; i < sent; i++) render_card(i, prefix);
  printf("  wrote %d PGMs to %s*\n", sent, prefix);

  if (logic) {
    printf("== quick mode: favourites carousel ==\n");
    card_window_push(0, true);
    const int8_t before = storage_last_used();
    stub_click(BUTTON_ID_DOWN);
    const int8_t after = storage_last_used();
    CHECK(before == 0, "opening card 0 marks it last-used");
    CHECK(after != before, "Down hops to next favourite (now %d)", after);
    Card *fav_card = storage_get((uint8_t)after);
    CHECK(fav_card && (fav_card->flags & CARD_FLAG_FAV), "landed on a favourite");
    char qpath[256]; snprintf(qpath, sizeof(qpath), "%squick.pgm", prefix);
    stub_render_pgm(qpath);
    stub_pop_top();

    printf("== page flip ==\n");
    card_window_push(1, false);
    stub_click(BUTTON_ID_UP);                       // -> info page
    char ipath[256]; snprintf(ipath, sizeof(ipath), "%sinfo.pgm", prefix);
    stub_render_pgm(ipath);
    stub_pop_top();

    printf("== action menu ==\n");
    card_window_push(2, false);
    stub_click(BUTTON_ID_SELECT);
    CHECK(stub_action_count() == 3, "menu has 3 actions");
    CHECK(!strcmp(stub_action_label(2), "Delete card"), "third action is delete");
    const uint8_t flags_before = storage_get(2)->flags;
    stub_action_invoke(0);                          // toggle favourite
    CHECK(storage_get(2)->flags != flags_before, "favourite toggled");
    stub_click(BUTTON_ID_SELECT);                   // reopen
    stub_action_invoke(2);                          // delete
    CHECK(storage_count() == (uint8_t)(sent - 1), "card deleted (%u left)", storage_count());
    CHECK(stub_stack_depth() == 1, "card window closed after delete");

    printf("== reboot persistence ==\n");
    storage_init();                                 // reload from persist
    CHECK(storage_count() == (uint8_t)(sent - 1), "deletion survived reboot");
    Card *c0 = storage_get(0);
    CHECK(c0 && c0->data_len > 0, "card 0 intact after reboot (%s)", c0 ? c0->name : "?");

    printf("== re-sync restores full set ==\n");
    replay_sync("test/out/cards.txt");
    CHECK(storage_count() == sent, "back to %d cards", sent);

    printf("== interrupted sync self-heals ==\n");
    deliver_op(OP_SYNC_BEGIN, sent);                // begin, then... nothing
    storage_init();                                 // watch reboots mid-sync
    CHECK(storage_count() == sent, "live set unharmed by dangling BEGIN");
    replay_sync("test/out/cards.txt");
    CHECK(storage_count() == sent, "next full sync lands cleanly");
  }

  printf("\n%d checks, %d failures\n", s_checks, s_fails);
  return s_fails ? 1 : 0;
}
