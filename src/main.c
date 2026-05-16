#include <pebble.h>

static Window      *s_window;
static BitmapLayer *s_bg_layer;
static GBitmap     *s_bg_bitmap;
static TextLayer   *s_time_layer;
static TextLayer   *s_date_layer;

static void update_time(struct tm *t) {
  static char time_buf[6];
  static char date_buf[10];

  strftime(time_buf, sizeof(time_buf),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  strftime(date_buf, sizeof(date_buf), "%a %d", t);

  text_layer_set_text(s_time_layer, time_buf);
  text_layer_set_text(s_date_layer, date_buf);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
}

static void window_load(Window *window) {
  Layer *root   = window_get_root_layer(window);
  GRect  bounds = layer_get_bounds(root);  /* 144 x 168 */

  /* full-screen background */
  s_bg_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GTA2MAP);
  s_bg_layer  = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bg_layer, s_bg_bitmap);
  bitmap_layer_set_compositing_mode(s_bg_layer, GCompOpAssign);
  layer_add_child(root, bitmap_layer_get_layer(s_bg_layer));

  /* date — bottom-right, above time, 58px wide */
  s_date_layer = text_layer_create(GRect(86, 122, 58, 16));
  text_layer_set_background_color(s_date_layer, GColorBlack);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer,
      fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_date_layer));

  /* time — bottom-right corner, 70px wide x 30px tall */
  s_time_layer = text_layer_create(GRect(74, 138, 70, 30));
  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer, GColorYellow);
  text_layer_set_font(s_time_layer,
      fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_time_layer));

  /* initial draw */
  time_t now = time(NULL);
  struct tm *tm_now = localtime(&now);
  update_time(tm_now);
}

static void window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  bitmap_layer_destroy(s_bg_layer);
  gbitmap_destroy(s_bg_bitmap);
}

static void init(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNITS, tick_handler);
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
