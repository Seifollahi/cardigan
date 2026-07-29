#pragma once
#include <pebble.h>

// =====================================================================
// Pebble Wallet — shared types & constants
//
// Constraint budget (why these numbers):
//  * persist_write_data() max value size = PERSIST_DATA_MAX_LENGTH (256 B)
//    -> one Card record must fit in a single persist key: sizeof(Card)=240 B
//  * App persist total budget ~4 KB -> MAX_CARDS 12 (12*240 = 2880 B)
//  * AppMessage inbox opened at 512 B -> one card per message fits with
//    dictionary overhead (~7 B/tuple)
//  * All code rendering data is PRE-ENCODED ON THE PHONE:
//      - barcode: run-length bar/space widths (1 byte each, starts with bar)
//      - QR:      row-major packed module bits, qr_size <= 29 (v3)
//    The watch only rasterizes integers. No tables, no float, tiny heap.
// =====================================================================

#define MAX_CARDS       12
#define CARD_NAME_LEN   24
#define CARD_CODE_LEN   28
#define CARD_DATA_MAX   180

#define CARD_TYPE_BARCODE 0   // data = bar/space widths
#define CARD_TYPE_QR      1   // data = packed module bits

#define CARD_FLAG_FAV   (1 << 0)

// Persist keys
#define PKEY_VERSION    1
#define PKEY_COUNT      2
#define PKEY_LAST_USED  3     // index of most recently used card
#define PKEY_CARD_BASE  100   // PKEY_CARD_BASE + i

#define STORE_VERSION   1

// AppMessage OP codes (phone -> watch)
#define OP_SYNC_BEGIN   1
#define OP_CARD         2
#define OP_SYNC_END     3
// (watch -> phone)
#define OP_REQUEST_SYNC 10
#define OP_CARD_USED    11    // notify phone so it can keep MRU order

typedef struct __attribute__((packed)) {
  char     name[CARD_NAME_LEN];   // 24  NUL-terminated
  char     code[CARD_CODE_LEN];   // 28  human-readable code text
  uint8_t  type;                  // 1   CARD_TYPE_*
  uint8_t  color;                 // 1   GColor8 .argb byte (0b11rrggbb)
  uint8_t  flags;                 // 1   CARD_FLAG_*
  uint8_t  qr_size;               // 1   QR module count (0 for barcode)
  uint16_t points;                // 2   loyalty balance (display only)
  uint16_t data_len;              // 2
  uint8_t  data[CARD_DATA_MAX];   // 180
} Card;                           // = 240 bytes  (fits one persist key)

_Static_assert(sizeof(Card) <= PERSIST_DATA_MAX_LENGTH,
               "Card must fit in a single persist key");

// ---------------- storage.c ----------------
void     storage_init(void);
uint8_t  storage_count(void);
Card    *storage_get(uint8_t index);
int8_t   storage_last_used(void);
void     storage_set_last_used(int8_t index);
void     storage_begin_sync(void);                 // stage incoming set
bool     storage_stage_card(uint8_t index, const Card *card);
void     storage_commit_sync(uint8_t total);       // swap staged -> live
void     storage_delete_card(uint8_t index);
void     storage_toggle_fav(uint8_t index);
void     storage_persist_card(uint8_t index);      // write one live card

// ---------------- comm.c ----------------
void comm_init(void);
void comm_deinit(void);
void comm_notify_used(uint8_t index);
void comm_request_sync(void);

// ---------------- menu_window.c ----------------
void menu_window_push(void);
void menu_window_refresh(void);

// ---------------- card_window.c ----------------
// quick=true: launched as "quick card" (up/down cycles favourites)
void card_window_push(uint8_t index, bool quick);
void card_window_refresh(void);

// utility
static inline GColor card_color(const Card *c) {
#if defined(PBL_COLOR)
  return (GColor){ .argb = (uint8_t)(0xC0 | c->color) };
#else
  (void)c;
  return GColorBlack;
#endif
}
