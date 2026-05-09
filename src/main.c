/*
 * LofiWatch — Pebble Time Steel (basalt) only
 * Background: lo-fi cozy night scene bitmap
 *
 * Clock screen canvas (144x168 coords):
 *   top-left:     (71, 116)
 *   bottom-right: (134, 136)
 *   width: 63px, height: 20px
 *
 * Layout:
 *   Time "HH:MM" — DS-Digital 17pt, flush left, vertically centred
 *   "MON"        — DejaVu Regular 7pt, flush right, upper
 *   "MM/DD"      — DejaVu Regular 7pt, flush right, lower
 */

#include <pebble.h>

#define SCR_X1    71
#define SCR_Y1   116
#define SCR_X2   134
#define SCR_Y2   136
#define SCR_W    (SCR_X2 - SCR_X1)   /* 63 */
#define SCR_H    (SCR_Y2 - SCR_Y1)   /* 20 */

static Window      *s_win;
static Layer       *s_canvas;
static BitmapLayer *s_bg_layer;
static GBitmap     *s_bg_bitmap;

static GFont        s_font_time;   /* DS-Digital 17 */
static GFont        s_font_date;   /* DejaVu Regular 7 — system font closest match */

static char s_time_buf[6];    /* "HH:MM" */
static char s_day_buf[4];     /* "MON"   */
static char s_date_buf[6];    /* "MM/DD" */

static void canvas_update(Layer *layer, GContext *ctx) {
  graphics_context_set_text_color(ctx, GColorBlack);

  /* ── Time — DS-Digital 17, flush left, vertically centred ── */
  GSize time_size = graphics_text_layout_get_content_size(
    s_time_buf, s_font_time,
    GRect(0, 0, SCR_W, SCR_H),
    GTextOverflowModeWordWrap, GTextAlignmentLeft);

  int time_y = SCR_Y1 + (SCR_H - time_size.h) / 2;

  graphics_draw_text(ctx, s_time_buf, s_font_time,
    GRect(SCR_X1, time_y, 40, SCR_H),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  /* ── Day + Date — DejaVu 7, flush right, stacked, vertically centred ── */
  /* Each line ~7px tall, gap 2px, total ~16px, centre in 20px = 2px top pad */
  int date_y1 = SCR_Y1 + 2;
  int date_y2 = date_y1 + 9;   /* 7px text + 2px gap */

  graphics_draw_text(ctx, s_day_buf, s_font_date,
    GRect(SCR_X1 + 40, date_y1, SCR_W - 40 - 1, 9),
    GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);

  graphics_draw_text(ctx, s_date_buf, s_font_date,
    GRect(SCR_X1 + 40, date_y2, SCR_W - 40 - 1, 9),
    GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
}

static void update_time(struct tm *t) {
  strftime(s_time_buf, sizeof(s_time_buf), "%H:%M", t);

  /* Day abbreviation — uppercase */
  strftime(s_day_buf, sizeof(s_day_buf), "%a", t);
  for (int i = 0; s_day_buf[i]; i++)
    if (s_day_buf[i] >= 'a' && s_day_buf[i] <= 'z')
      s_day_buf[i] -= 32;

  strftime(s_date_buf, sizeof(s_date_buf), "%m/%d", t);

  layer_mark_dirty(s_canvas);
}

static void tick_handler(struct tm *t, TimeUnits u) { update_time(t); }

static void window_load(Window *w) {
  Layer *root  = window_get_root_layer(w);
  GRect  bounds = layer_get_bounds(root);

  /* Background bitmap */
  s_bg_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMG_LOFI_BG);
  s_bg_layer  = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bg_layer, s_bg_bitmap);
  bitmap_layer_set_compositing_mode(s_bg_layer, GCompOpAssign);
  layer_add_child(root, bitmap_layer_get_layer(s_bg_layer));

  /* Canvas on top */
  s_canvas = layer_create(bounds);
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);

  /* Fonts
   * CloudPebble resources:
   *   DS-DIGI.TTF        -> FONT_DS_17   size 17
   *   Use system font    -> fonts_get_system_font(FONT_KEY_GOTHIC_14) for date
   *   OR add DejaVu      -> FONT_DATE_7  size 7
   */
  s_font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DS_17));
  s_font_date = fonts_get_system_font(FONT_KEY_GOTHIC_14);

  time_t now = time(NULL);
  update_time(localtime(&now));
}

static void window_unload(Window *w) {
  layer_destroy(s_canvas);
  bitmap_layer_destroy(s_bg_layer);
  gbitmap_destroy(s_bg_bitmap);
  fonts_unload_custom_font(s_font_time);
}

static void init(void) {
  s_win = window_create();
  window_set_background_color(s_win, GColorWhite);
  window_set_window_handlers(s_win, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_win, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_win);
}

int main(void) { init(); app_event_loop(); deinit(); }
