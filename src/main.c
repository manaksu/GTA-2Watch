#include <pebble.h>

/*
 * GTA2 HUD Watchface
 *   Key 0: KEY_THEME  0=Normal  1=ePaper  2=Monochrome
 */

#define KEY_THEME      0
#define THEME_NORMAL   0
#define THEME_EPAPER   1
#define THEME_MONO     2

/* ── HUD layout ── */
#define ICON_W    7
#define BAR_W    22
#define ROW_H     8
#define ROW_GAP   2
#define LEFT      5
#define BAR_X    (LEFT + ICON_W + 1)
#define STAT_X   (BAR_X + BAR_W + 2)
#define STAT_W   (144 - STAT_X)
#define ROW1_Y    6
#define ROW2_Y   (ROW1_Y + ROW_H + ROW_GAP)
#define ROW3_Y   (ROW2_Y + ROW_H + ROW_GAP)

static int s_theme = THEME_NORMAL;

static Window      *s_window;
static BitmapLayer *s_bg_layer;
static GBitmap     *s_bg_bitmap;

static BitmapLayer *s_icon_steps_layer, *s_icon_heart_layer, *s_icon_batt_layer;
static GBitmap     *s_icon_steps_bmp,   *s_icon_heart_bmp,   *s_icon_batt_bmp;
static BitmapLayer *s_bar_steps_layer,  *s_bar_heart_layer,  *s_bar_batt_layer;
static GBitmap     *s_bar_steps_bmp,    *s_bar_heart_bmp,    *s_bar_batt_bmp;

static TextLayer   *s_steps_label, *s_heart_label, *s_batt_label;
static TextLayer   *s_time_layer, *s_date_layer;

static char s_time_buf[6], s_date_buf[10];
static char s_steps_buf[8], s_heart_buf[6], s_batt_buf[6];

/* ── resource selectors ── */
static uint32_t bg_res(void) {
  if (s_theme == THEME_EPAPER) return RESOURCE_ID_IMAGE_GTA2MAP_EPAPER;
  if (s_theme == THEME_MONO)   return RESOURCE_ID_IMAGE_GTA2MAP_MONO;
  return RESOURCE_ID_IMAGE_GTA2MAP;
}
static uint32_t icon_res(uint32_t normal, uint32_t mono) {
  return (s_theme == THEME_MONO) ? mono : normal;
}
static uint32_t bar_res(uint32_t normal, uint32_t mono) {
  return (s_theme == THEME_MONO) ? mono : normal;
}

/* ── apply theme ── */
static void apply_theme(void) {
  /* swap background */
  if (s_bg_bitmap) gbitmap_destroy(s_bg_bitmap);
  s_bg_bitmap = gbitmap_create_with_resource(bg_res());
  bitmap_layer_set_bitmap(s_bg_layer, s_bg_bitmap);

  /* swap HUD icon bitmaps */
  gbitmap_destroy(s_icon_steps_bmp);
  s_icon_steps_bmp = gbitmap_create_with_resource(
    icon_res(RESOURCE_ID_IMAGE_HUD_ICON_STEPS, RESOURCE_ID_IMAGE_HUD_ICON_STEPS_MONO));
  bitmap_layer_set_bitmap(s_icon_steps_layer, s_icon_steps_bmp);

  gbitmap_destroy(s_icon_heart_bmp);
  s_icon_heart_bmp = gbitmap_create_with_resource(
    icon_res(RESOURCE_ID_IMAGE_HUD_ICON_HEART, RESOURCE_ID_IMAGE_HUD_ICON_HEART_MONO));
  bitmap_layer_set_bitmap(s_icon_heart_layer, s_icon_heart_bmp);

  gbitmap_destroy(s_icon_batt_bmp);
  s_icon_batt_bmp = gbitmap_create_with_resource(
    icon_res(RESOURCE_ID_IMAGE_HUD_ICON_BATT, RESOURCE_ID_IMAGE_HUD_ICON_BATT_MONO));
  bitmap_layer_set_bitmap(s_icon_batt_layer, s_icon_batt_bmp);

  /* swap HUD bar bitmaps */
  gbitmap_destroy(s_bar_steps_bmp);
  s_bar_steps_bmp = gbitmap_create_with_resource(
    bar_res(RESOURCE_ID_IMAGE_HUD_BAR_STEPS, RESOURCE_ID_IMAGE_HUD_BAR_STEPS_MONO));
  bitmap_layer_set_bitmap(s_bar_steps_layer, s_bar_steps_bmp);

  gbitmap_destroy(s_bar_heart_bmp);
  s_bar_heart_bmp = gbitmap_create_with_resource(
    bar_res(RESOURCE_ID_IMAGE_HUD_BAR_HEART, RESOURCE_ID_IMAGE_HUD_BAR_HEART_MONO));
  bitmap_layer_set_bitmap(s_bar_heart_layer, s_bar_heart_bmp);

  gbitmap_destroy(s_bar_batt_bmp);
  s_bar_batt_bmp = gbitmap_create_with_resource(
    bar_res(RESOURCE_ID_IMAGE_HUD_BAR_BATT, RESOURCE_ID_IMAGE_HUD_BAR_BATT_MONO));
  bitmap_layer_set_bitmap(s_bar_batt_layer, s_bar_batt_bmp);

  /* time colour:
   *   Normal   = yellow
   *   ePaper   = white (on black box)
   *   Mono     = black (inverted — black text on white bg)
   */
  GColor time_col, time_bg, stat_col;
  if (s_theme == THEME_MONO) {
    time_col = GColorWhite;
    time_bg  = GColorBlack;
    stat_col = GColorWhite;
  } else if (s_theme == THEME_EPAPER) {
    time_col = GColorWhite;
    time_bg  = GColorBlack;
    stat_col = GColorWhite;
  } else {
    time_col = GColorYellow;
    time_bg  = GColorBlack;
    stat_col = GColorWhite;
  }
  text_layer_set_text_color(s_time_layer, time_col);
  text_layer_set_background_color(s_time_layer, time_bg);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, time_bg);
  text_layer_set_text_color(s_steps_label, stat_col);
  text_layer_set_text_color(s_heart_label, stat_col);
  text_layer_set_text_color(s_batt_label,  stat_col);

  layer_mark_dirty(window_get_root_layer(s_window));
}

/* ── stats ── */
static void update_stats(void) {
  time_t t_end   = time(NULL);
  time_t t_start = time_start_of_today();

  if (t_start < t_end &&
      health_service_metric_accessible(HealthMetricStepCount, t_start, t_end)) {
    snprintf(s_steps_buf, sizeof(s_steps_buf), "%d",
             (int)health_service_sum_today(HealthMetricStepCount));
  } else {
    snprintf(s_steps_buf, sizeof(s_steps_buf), "--");
  }

  HealthServiceAccessibilityMask hr =
    health_service_metric_accessible(HealthMetricHeartRateBPM, t_end-60, t_end);
  if (hr & HealthServiceAccessibilityMaskAvailable) {
    HealthValue v = health_service_peek_current_value(HealthMetricHeartRateBPM);
    snprintf(s_heart_buf, sizeof(s_heart_buf), v > 0 ? "%d" : "--", (int)v);
  } else {
    snprintf(s_heart_buf, sizeof(s_heart_buf), "--");
  }

  BatteryChargeState bat = battery_state_service_peek();
  snprintf(s_batt_buf, sizeof(s_batt_buf), "%d%%", (int)bat.charge_percent);

  text_layer_set_text(s_steps_label, s_steps_buf);
  text_layer_set_text(s_heart_label, s_heart_buf);
  text_layer_set_text(s_batt_label,  s_batt_buf);
}

/* ── time ── */
static void update_time(struct tm *t) {
  strftime(s_time_buf, sizeof(s_time_buf),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  strftime(s_date_buf, sizeof(s_date_buf), "%a %d", t);
  text_layer_set_text(s_time_layer, s_time_buf);
  text_layer_set_text(s_date_layer, s_date_buf);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(tick_time);
  if (tick_time->tm_min % 5 == 0) update_stats();
}

/* ── AppMessage ── */
static void inbox_received(DictionaryIterator *iter, void *ctx) {
  Tuple *t = dict_find(iter, KEY_THEME);
  if (t) {
    s_theme = (int)t->value->int32;
    persist_write_int(KEY_THEME, s_theme);
    apply_theme();
  }
}

/* ── helpers ── */
static void make_bmp(BitmapLayer **bl, GBitmap **bmp,
                     uint32_t res, GRect frame, Layer *root) {
  *bmp = gbitmap_create_with_resource(res);
  *bl  = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(*bl, *bmp);
  bitmap_layer_set_compositing_mode(*bl, GCompOpSet);
  layer_add_child(root, bitmap_layer_get_layer(*bl));
}

static TextLayer *make_stat(GRect frame, Layer *root) {
  TextLayer *tl = text_layer_create(frame);
  text_layer_set_background_color(tl, GColorClear);
  text_layer_set_text_color(tl, GColorWhite);
  text_layer_set_font(tl, fonts_get_system_font(FONT_KEY_GOTHIC_09));
  text_layer_set_text_alignment(tl, GTextAlignmentLeft);
  layer_add_child(root, text_layer_get_layer(tl));
  return tl;
}

/* ── window load ── */
static void window_load(Window *window) {
  Layer *root   = window_get_root_layer(window);
  GRect  bounds = layer_get_bounds(root);

  /* background */
  s_bg_bitmap = gbitmap_create_with_resource(bg_res());
  s_bg_layer  = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bg_layer, s_bg_bitmap);
  bitmap_layer_set_compositing_mode(s_bg_layer, GCompOpAssign);
  layer_add_child(root, bitmap_layer_get_layer(s_bg_layer));

  /* row 1 — steps */
  make_bmp(&s_icon_steps_layer, &s_icon_steps_bmp,
           icon_res(RESOURCE_ID_IMAGE_HUD_ICON_STEPS, RESOURCE_ID_IMAGE_HUD_ICON_STEPS_MONO),
           GRect(LEFT, ROW1_Y, ICON_W, ROW_H), root);
  make_bmp(&s_bar_steps_layer, &s_bar_steps_bmp,
           bar_res(RESOURCE_ID_IMAGE_HUD_BAR_STEPS, RESOURCE_ID_IMAGE_HUD_BAR_STEPS_MONO),
           GRect(BAR_X, ROW1_Y, BAR_W, ROW_H), root);
  s_steps_label = make_stat(GRect(STAT_X, ROW1_Y-1, STAT_W, ROW_H+2), root);

  /* row 2 — heart rate */
  make_bmp(&s_icon_heart_layer, &s_icon_heart_bmp,
           icon_res(RESOURCE_ID_IMAGE_HUD_ICON_HEART, RESOURCE_ID_IMAGE_HUD_ICON_HEART_MONO),
           GRect(LEFT, ROW2_Y, ICON_W, ROW_H), root);
  make_bmp(&s_bar_heart_layer, &s_bar_heart_bmp,
           bar_res(RESOURCE_ID_IMAGE_HUD_BAR_HEART, RESOURCE_ID_IMAGE_HUD_BAR_HEART_MONO),
           GRect(BAR_X, ROW2_Y, BAR_W, ROW_H), root);
  s_heart_label = make_stat(GRect(STAT_X, ROW2_Y-1, STAT_W, ROW_H+2), root);

  /* row 3 — battery */
  make_bmp(&s_icon_batt_layer, &s_icon_batt_bmp,
           icon_res(RESOURCE_ID_IMAGE_HUD_ICON_BATT, RESOURCE_ID_IMAGE_HUD_ICON_BATT_MONO),
           GRect(LEFT, ROW3_Y, ICON_W, ROW_H), root);
  make_bmp(&s_bar_batt_layer, &s_bar_batt_bmp,
           bar_res(RESOURCE_ID_IMAGE_HUD_BAR_BATT, RESOURCE_ID_IMAGE_HUD_BAR_BATT_MONO),
           GRect(BAR_X, ROW3_Y, BAR_W, ROW_H), root);
  s_batt_label = make_stat(GRect(STAT_X, ROW3_Y-1, STAT_W, ROW_H+2), root);

  /* date */
  s_date_layer = text_layer_create(GRect(86, 122, 58, 16));
  text_layer_set_background_color(s_date_layer, GColorBlack);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_date_layer));

  /* time */
  s_time_layer = text_layer_create(GRect(74, 138, 70, 30));
  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer,
      (s_theme == THEME_MONO) ? GColorWhite : (s_theme == THEME_EPAPER) ? GColorWhite : GColorYellow);
  text_layer_set_font(s_time_layer,
      fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_time_layer));

  time_t now = time(NULL);
  update_time(localtime(&now));
  update_stats();
}

static void window_unload(Window *window) {
  text_layer_destroy(s_steps_label);
  text_layer_destroy(s_heart_label);
  text_layer_destroy(s_batt_label);
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
  s_theme = persist_exists(KEY_THEME)
            ? persist_read_int(KEY_THEME)
            : THEME_NORMAL;

  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  app_message_open(64, 64);
  app_message_register_inbox_received(inbox_received);
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
