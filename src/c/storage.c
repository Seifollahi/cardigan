#include "wallet.h"

// Live card set, loaded once at init. 12 * 240 = 2880 B static RAM —
// well inside the app heap on every target (basalt 64K .. emery 128K+).
static Card    s_cards[MAX_CARDS];
static uint8_t s_count = 0;
static int8_t  s_last_used = -1;

// Staging area for an incoming sync. To avoid a second 2.8 KB buffer we
// stage straight into persist under the live keys, then reload. A sync
// that dies halfway is repaired on next connect (phone is the source of
// truth and re-pushes the full set).
static uint8_t s_staged = 0;

static void load_all(void) {
  s_count = 0;
  int32_t stored = persist_exists(PKEY_COUNT) ? persist_read_int(PKEY_COUNT) : 0;
  if (stored < 0) stored = 0;
  if (stored > MAX_CARDS) stored = MAX_CARDS;
  for (int32_t i = 0; i < stored; i++) {
    const uint32_t key = PKEY_CARD_BASE + i;
    if (!persist_exists(key)) continue;
    int read = persist_read_data(key, &s_cards[s_count], sizeof(Card));
    if (read == (int)sizeof(Card)) {
      // defensive NUL-termination
      s_cards[s_count].name[CARD_NAME_LEN - 1] = '\0';
      s_cards[s_count].code[CARD_CODE_LEN - 1] = '\0';
      if (s_cards[s_count].data_len > CARD_DATA_MAX) {
        s_cards[s_count].data_len = CARD_DATA_MAX;
      }
      s_count++;
    }
  }
  s_last_used = persist_exists(PKEY_LAST_USED)
              ? (int8_t)persist_read_int(PKEY_LAST_USED) : -1;
  if (s_last_used >= (int8_t)s_count) s_last_used = s_count ? 0 : -1;
}

void storage_init(void) {
  const int32_t version = persist_exists(PKEY_VERSION)
                        ? persist_read_int(PKEY_VERSION) : 0;
  if (version != STORE_VERSION) {
    // schema change: wipe cards, phone re-syncs on connect
    for (uint32_t i = 0; i < MAX_CARDS; i++) persist_delete(PKEY_CARD_BASE + i);
    persist_write_int(PKEY_COUNT, 0);
    persist_write_int(PKEY_VERSION, STORE_VERSION);
  }
  load_all();
}

uint8_t storage_count(void) { return s_count; }

Card *storage_get(uint8_t index) {
  return (index < s_count) ? &s_cards[index] : NULL;
}

int8_t storage_last_used(void) {
  return (s_last_used >= 0 && s_last_used < (int8_t)s_count) ? s_last_used : -1;
}

void storage_set_last_used(int8_t index) {
  if (index < 0 || index >= (int8_t)s_count) return;
  s_last_used = index;
  persist_write_int(PKEY_LAST_USED, index);
}

void storage_begin_sync(void) { s_staged = 0; }

bool storage_stage_card(uint8_t index, const Card *card) {
  if (index >= MAX_CARDS) return false;
  const int wrote = persist_write_data(PKEY_CARD_BASE + index, card, sizeof(Card));
  if (wrote == (int)sizeof(Card)) {
    if (index + 1 > s_staged) s_staged = index + 1;
    return true;
  }
  APP_LOG(APP_LOG_LEVEL_ERROR, "persist write failed (%d) for card %u", wrote, index);
  return false;
}

void storage_commit_sync(uint8_t total) {
  if (total > MAX_CARDS) total = MAX_CARDS;
  if (total > s_staged)  total = s_staged;
  for (uint32_t i = total; i < MAX_CARDS; i++) persist_delete(PKEY_CARD_BASE + i);
  persist_write_int(PKEY_COUNT, total);
  load_all();
}

static void save_all(void) {
  for (uint8_t i = 0; i < s_count; i++) {
    persist_write_data(PKEY_CARD_BASE + i, &s_cards[i], sizeof(Card));
  }
  for (uint32_t i = s_count; i < MAX_CARDS; i++) persist_delete(PKEY_CARD_BASE + i);
  persist_write_int(PKEY_COUNT, s_count);
}

void storage_delete_card(uint8_t index) {
  if (index >= s_count) return;
  for (uint8_t i = index; i + 1 < s_count; i++) s_cards[i] = s_cards[i + 1];
  s_count--;
  if (s_last_used == (int8_t)index)      s_last_used = s_count ? 0 : -1;
  else if (s_last_used > (int8_t)index)  s_last_used--;
  persist_write_int(PKEY_LAST_USED, s_last_used);
  save_all();
}

void storage_toggle_fav(uint8_t index) {
  Card *c = storage_get(index);
  if (!c) return;
  c->flags ^= CARD_FLAG_FAV;
  storage_persist_card(index);
}

void storage_persist_card(uint8_t index) {
  if (index >= s_count) return;
  persist_write_data(PKEY_CARD_BASE + index, &s_cards[index], sizeof(Card));
}
