#include <pebble.h>

/*
 * HUD layout — 30x14px total in top-left corner
 *
 *  x=1   x=9
 *  [icon][bar 22px]   y=1  (steps  - blue)
 *  [icon][bar 22px]   y=6  (heart  - yellow)
 *  [icon][bar 22px]   y=11 (batt   - grey)
 *
 *  Each row = 4px tall, 1px gap between rows
 *  icon=7px wide, gap=1px, bar=22px wide => 30px total
 */
#define ICON_W   7
#define BAR_W   22
#define ROW_H    4
#define ROW_GAP  1
#define LEFT     1
#define BAR_X   (LEFT + ICON_W + 1)   /* 9 */

#define ROW1_Y   1
#define ROW2_Y  (ROW1_Y + ROW_H + ROW_GAP)   /* 6  */
#define ROW3_Y  (ROW2_Y + ROW_H + ROW_GAP)   /* 11 */

static Window      *s_window;
static BitmapLayer *s_bg_layer;
static GBitmap     *s_bg_bitmap;

static BitmapLayer *s_icon_steps_layer, *s_icon_heart_layer, *s_icon_batt_layer;
static GBitmap     *s_icon_steps_bmp,   *s_icon_heart_bmp,   *s_icon_batt_bmp;
static BitmapLayer *s_bar_steps_layer,  *s_bar_heart_layer,  *s_bar_batt_layer;
static GBitmap     *s_bar_steps_bmp,    *s_bar_heart_bmp,    *s_bar_batt_bmp;

static TextLayer   *s_time_layer, *s_date_layer;
static char         s_time_buf[6], s_date_buf[10];

static void update_time(struct tm *t) {
  strftime(s_time_buf, sizeof(s_time_buf),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  strftime(s_date_buf, sizeof(s_date_buf), "%a %d", t);
  text_layer_set_text(s_time_layer, s_time_buf);
  text_layer_set_text(s_date_layer, s_date_buf);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
}

static void make_bmp(BitmapLayer **bl, GBitmap **bmp,
                     uint32_t res, GRect frame, Layer *root) {
  *bmp = gbitmap_create_with_resource(res);
  *bl  = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(*bl, *bmp);
  bitmap_layer_set_compositing_mode(*bl, GCompOpSet);
  layer_add_child(root, bitmap_layer_get_layer(*bl));
}

static void window_load(Window *window) {
  Layer *root   = window_get_root_layer(window);
  GRect  bounds = layer_get_bounds(root);

  /* background */
  s_bg_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GTA2MAP);
  s_bg_layer  = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bg_layer, s_bg_bitmap);
  bitmap_layer_set_compositing_mode(s_bg_layer, GCompOpAssign);
  layer_add_child(root, bitmap_layer_get_layer(s_bg_layer));

  /* row 1 — steps */
  make_bmp(&s_icon_steps_layer, &s_icon_steps_bmp,
           RESOURCE_ID_IMAGE_HUD_ICON_STEPS,
           GRect(LEFT, ROW1_Y, ICON_W, ROW_H), root);
  make_bmp(&s_bar_steps_layer, &s_bar_steps_bmp,
           RESOURCE_ID_IMAGE_HUD_BAR_STEPS,
           GRect(BAR_X, ROW1_Y, BAR_W, ROW_H), root);

  /* row 2 — heart rate */
  make_bmp(&s_icon_heart_layer, &s_icon_heart_bmp,
           RESOURCE_ID_IMAGE_HUD_ICON_HEART,
           GRect(LEFT, ROW2_Y, ICON_W, ROW_H), root);
  make_bmp(&s_bar_heart_layer, &s_bar_heart_bmp,
           RESOURCE_ID_IMAGE_HUD_BAR_HEART,
           GRect(BAR_X, ROW2_Y, BAR_W, ROW_H), root);

  /* row 3 — battery */
  make_bmp(&s_icon_batt_layer, &s_icon_batt_bmp,
           RESOURCE_ID_IMAGE_HUD_ICON_BATT,
           GRect(LEFT, ROW3_Y, ICON_W, ROW_H), root);
  make_bmp(&s_bar_batt_layer, &s_bar_batt_bmp,
           RESOURCE_ID_IMAGE_HUD_BAR_BATT,
           GRect(BAR_X, ROW3_Y, BAR_W, ROW_H), root);

  /* date — bottom-right */
  s_date_layer = text_layer_create(GRect(86, 122, 58, 16));
  text_layer_set_background_color(s_date_layer, GColorBlack);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_date_layer));

  /* time — bottom-right */
  s_time_layer = text_layer_create(GRect(74, 138, 70, 30));
  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer, GColorYellow);
  text_layer_set_font(s_time_layer,
      fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_time_layer));

  time_t now = time(NULL);
  update_time(localtime(&now));
}

static void window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  bitmap_layer_destroy(s_bg_layer);
  bitmap_layer_destroy(s_icon_steps_layer);
  bitmap_layer_destroy(s_bar_steps_layer);
  bitmap_layer_destroy(s_icon_heart_layer);
  bitmap_layer_destroy(s_bar_heart_layer);
  bitmap_layer_destroy(s_icon_batt_layer);
  bitmap_layer_destroy(s_bar_batt_layer);
  gbitmap_destroy(s_bg_bitmap);
  gbitmap_destroy(s_icon_steps_bmp);
  gbitmap_destroy(s_bar_steps_bmp);
  gbitmap_destroy(s_icon_heart_bmp);
  gbitmap_destroy(s_bar_heart_bmp);
  gbitmap_destroy(s_icon_batt_bmp);
  gbitmap_destroy(s_bar_batt_bmp);
}

static void init(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
