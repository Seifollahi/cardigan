#include "wallet.h"

// AppMessage sizes: one full card per message.
// Payload worst case: name 24 + code 28 + data 180 + 6 small ints
// + dictionary overhead (~7 B/tuple * 9) ≈ 330 B  ->  512 B inbox.
#define INBOX_SIZE  512
#define OUTBOX_SIZE 64

static uint8_t s_expected_total = 0;

static void handle_card(DictionaryIterator *iter) {
  Tuple *t_index = dict_find(iter, MESSAGE_KEY_INDEX);
  Tuple *t_name  = dict_find(iter, MESSAGE_KEY_NAME);
  Tuple *t_code  = dict_find(iter, MESSAGE_KEY_CODE);
  Tuple *t_type  = dict_find(iter, MESSAGE_KEY_TYPE);
  Tuple *t_color = dict_find(iter, MESSAGE_KEY_COLOR);
  Tuple *t_flags = dict_find(iter, MESSAGE_KEY_FLAGS);
  Tuple *t_qrsz  = dict_find(iter, MESSAGE_KEY_QRSIZE);
  Tuple *t_pts   = dict_find(iter, MESSAGE_KEY_POINTS);
  Tuple *t_data  = dict_find(iter, MESSAGE_KEY_DATA);
  if (!t_index || !t_name || !t_code || !t_data) return;

  static Card card;                       // static: keep stack shallow
  memset(&card, 0, sizeof(card));
  strncpy(card.name, t_name->value->cstring, CARD_NAME_LEN - 1);
  strncpy(card.code, t_code->value->cstring, CARD_CODE_LEN - 1);
  card.type    = t_type  ? (uint8_t)t_type->value->uint8   : CARD_TYPE_BARCODE;
  card.color   = t_color ? (uint8_t)t_color->value->uint8  : 0x03;
  card.flags   = t_flags ? (uint8_t)t_flags->value->uint8  : 0;
  card.qr_size = t_qrsz  ? (uint8_t)t_qrsz->value->uint8   : 0;
  card.points  = t_pts   ? (uint16_t)t_pts->value->uint16  : 0;

  uint16_t len = t_data->length;
  if (len > CARD_DATA_MAX) len = CARD_DATA_MAX;
  card.data_len = len;
  memcpy(card.data, t_data->value->data, len);

  storage_stage_card((uint8_t)t_index->value->uint8, &card);
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *t_op = dict_find(iter, MESSAGE_KEY_OP);
  if (!t_op) return;
  switch (t_op->value->uint8) {
    case OP_SYNC_BEGIN: {
      Tuple *t_total = dict_find(iter, MESSAGE_KEY_TOTAL);
      s_expected_total = t_total ? t_total->value->uint8 : 0;
      storage_begin_sync();
      break;
    }
    case OP_CARD:
      handle_card(iter);
      break;
    case OP_SYNC_END:
      storage_commit_sync(s_expected_total);
      menu_window_refresh();
      card_window_refresh();
      vibes_short_pulse();
      break;
    default:
      break;
  }
}

static void inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "inbox dropped: %d", (int)reason);
}

static void send_simple(uint8_t op, int8_t index) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) return;
  dict_write_uint8(iter, MESSAGE_KEY_OP, op);
  if (index >= 0) dict_write_uint8(iter, MESSAGE_KEY_INDEX, (uint8_t)index);
  app_message_outbox_send();
}

void comm_notify_used(uint8_t index) { send_simple(OP_CARD_USED, (int8_t)index); }
void comm_request_sync(void)         { send_simple(OP_REQUEST_SYNC, -1); }

void comm_init(void) {
  app_message_register_inbox_received(inbox_received);
  app_message_register_inbox_dropped(inbox_dropped);
  app_message_open(INBOX_SIZE, OUTBOX_SIZE);
}

void comm_deinit(void) {
  app_message_deregister_callbacks();
}
