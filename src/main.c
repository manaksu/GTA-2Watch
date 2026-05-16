#include <pebble.h>

/* ── layers ── */
static Window      *s_window;
static BitmapLayer *s_bg_layer;
static GBitmap     *s_bg_bitmap;

/* icon bitmaps */
static BitmapLayer *s_icon_steps_layer;
static BitmapLayer *s_icon_heart_layer;
static BitmapLayer *s_icon_batt_layer;
static GBitmap     *s_icon_steps_bmp;
static GBitmap     *s_icon_heart_bmp;
static GBitmap     *s_icon_batt_bmp;

/* bar bitmaps */
static BitmapLayer *s_bar_steps_layer;
static BitmapLayer *s_bar_heart_layer;
static BitmapLayer *s_bar_batt_layer;
static GBitmap     *s_bar_steps_bmp;
static GBitmap     *s_bar_heart_bmp;
static GBitmap     *s_bar_batt_bmp;

/* stat text layers (overlaid on bars) */
static TextLayer   *s_steps_label;
static TextLayer   *s_heart_label;
static TextLayer   *s_batt_label;

/* time */
static TextLayer   *s_time_layer;
static TextLayer   *s_date_layer;

/* ── layout constants ── */
/* Each HUD row: icon 20x18, then bar 96x20, stacked top-left */
#define ICON_W   20
#define ICON_H   18
#define BAR_W    96
#define BAR_H    20
#define ROW_PAD  4   /* vertical gap between rows */

#define ROW1_Y   4
#define ROW2_Y   (ROW1_Y + BAR_H + ROW_PAD)
#define ROW3_Y   (ROW2_Y + BAR_H + ROW_PAD)

/* ── update health data ── */
static void update_stats(void) {
  static char steps_buf[10];
  static char heart_buf[8];
  static char batt_buf[6];

  /* Steps */
  HealthMetric metric = HealthMetricStepCount;
  if ( health_service_metric_accessible(metric, time(NULL) - 86400, time(NULL))) {
    int steps = (int)health_service_sum_today(metric);
    snprintf(steps_buf, sizeof(steps_buf), "%d", steps);
  } else {
    snprintf(steps_buf, sizeof(steps_buf), "--");
  }

  /* Heart rate */
  HealthMetric hr_metric = HealthMetricHeartRateBPM;
  if ( health_service_metric_accessible(hr_metric, time(NULL) - 60, time(NULL))) {
    int hr = (int)health_service_peek_current_value(hr_metric);
    snprintf(heart_buf, sizeof(heart_buf), "%d", hr);
  } else {
    snprintf(heart_buf, sizeof(heart_buf), "--");
  }

  /* Battery */
  BatteryChargeState bat = battery_state_service_peek();
  snprintf(batt_buf, sizeof(batt_buf), "%d%%", bat.charge_percent);

  text_layer_set_text(s_steps_label, steps_buf);
  text_layer_set_text(s_heart_label, heart_buf);
  text_layer_set_text(s_batt_label,  batt_buf);
}

/* ── time ── */
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
  /* update stats every 5 minutes */
  if (tick_time->tm_min % 5 == 0) {
    update_stats();
  }
}

/* ── helper: create a bitmap layer ── */
static void make_bitmap_layer(BitmapLayer **bl, GBitmap **bmp,
                               uint32_t res_id, GRect frame, Layer *root) {
  *bmp = gbitmap_create_with_resource(res_id);
  *bl  = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(*bl, *bmp);
  bitmap_layer_set_compositing_mode(*bl, GCompOpSet);
  layer_add_child(root, bitmap_layer_get_layer(*bl));
}

/* ── helper: stat text layer over a bar ── */
static TextLayer *make_stat_label(GRect frame, Layer *root) {
  TextLayer *tl = text_layer_create(frame);
  text_layer_set_background_color(tl, GColorClear);
  text_layer_set_text_color(tl, GColorWhite);
  text_layer_set_font(tl, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(tl, GTextAlignmentLeft);
  layer_add_child(root, text_layer_get_layer(tl));
  return tl;
}

/* ── window load ── */
static void window_load(Window *window) {
  Layer *root   = window_get_root_layer(window);
  GRect  bounds = layer_get_bounds(root);  /* 144 x 168 */

  /* background */
  s_bg_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GTA2MAP);
  s_bg_layer  = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bg_layer, s_bg_bitmap);
  bitmap_layer_set_compositing_mode(s_bg_layer, GCompOpAssign);
  layer_add_child(root, bitmap_layer_get_layer(s_bg_layer));

  /* --- ROW 1: Steps (blue bar) --- */
  make_bitmap_layer(&s_icon_steps_layer, &s_icon_steps_bmp,
    RESOURCE_ID_IMAGE_ICON_STEPS,
    GRect(2, ROW1_Y, ICON_W, ICON_H), root);

  make_bitmap_layer(&s_bar_steps_layer, &s_bar_steps_bmp,
    RESOURCE_ID_IMAGE_BAR_STEPS,
    GRect(ICON_W + 2, ROW1_Y, BAR_W, BAR_H), root);

  s_steps_label = make_stat_label(
    GRect(ICON_W + 6, ROW1_Y + 2, BAR_W - 10, 16), root);

  /* --- ROW 2: Heart Rate (yellow bar) --- */
  make_bitmap_layer(&s_icon_heart_layer, &s_icon_heart_bmp,
    RESOURCE_ID_IMAGE_ICON_HEART,
    GRect(2, ROW2_Y, ICON_W, ICON_H), root);

  make_bitmap_layer(&s_bar_heart_layer, &s_bar_heart_bmp,
    RESOURCE_ID_IMAGE_BAR_HEART,
    GRect(ICON_W + 2, ROW2_Y, BAR_W, BAR_H), root);

  s_heart_label = make_stat_label(
    GRect(ICON_W + 6, ROW2_Y + 2, BAR_W - 10, 16), root);

  /* --- ROW 3: Battery (grey bar) --- */
  make_bitmap_layer(&s_icon_batt_layer, &s_icon_batt_bmp,
    RESOURCE_ID_IMAGE_ICON_BATT,
    GRect(2, ROW3_Y, ICON_W, ICON_H), root);

  make_bitmap_layer(&s_bar_batt_layer, &s_bar_batt_bmp,
    RESOURCE_ID_IMAGE_BAR_BATT,
    GRect(ICON_W + 2, ROW3_Y, BAR_W, BAR_H), root);

  s_batt_label = make_stat_label(
    GRect(ICON_W + 6, ROW3_Y + 2, BAR_W - 10, 16), root);

  /* --- time: bottom-right corner --- */
  s_date_layer = text_layer_create(GRect(86, 122, 58, 16));
  text_layer_set_background_color(s_date_layer, GColorBlack);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer,
      fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_date_layer));

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
  update_stats();
}

static void window_unload(Window *window) {
  /* destroy text layers */
  text_layer_destroy(s_steps_label);
  text_layer_destroy(s_heart_label);
  text_layer_destroy(s_batt_label);
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);

  /* destroy bitmap layers */
  bitmap_layer_destroy(s_bg_layer);
  bitmap_layer_destroy(s_icon_steps_layer);
  bitmap_layer_destroy(s_icon_heart_layer);
  bitmap_layer_destroy(s_icon_batt_layer);
  bitmap_layer_destroy(s_bar_steps_layer);
  bitmap_layer_destroy(s_bar_heart_layer);
  bitmap_layer_destroy(s_bar_batt_layer);

  /* destroy bitmaps */
  gbitmap_destroy(s_bg_bitmap);
  gbitmap_destroy(s_icon_steps_bmp);
  gbitmap_destroy(s_icon_heart_bmp);
  gbitmap_destroy(s_icon_batt_bmp);
  gbitmap_destroy(s_bar_steps_bmp);
  gbitmap_destroy(s_bar_heart_bmp);
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
  health_service_events_subscribe(NULL, NULL);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  health_service_events_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
