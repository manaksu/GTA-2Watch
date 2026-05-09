/*
 * GTA2Watch — Pebble Time Steel (basalt) only
 * Fonts: Orbitron ExtraBold (time), Orbitron Bold (scores/stats/date),
 *        Orbitron ExtraBold (menu)
 *
 * Layout (144 × 168)
 * ─────────────────────────────────────────────
 *  [■ MON, 04 MAY 26 ══════════════════════►  ]  blue pill bar, bleeds right
 *  [         ( 1  2  :  1  2 )               ]  tight stadium border
 *
 *   PLAYER ONE        2,130,775               score rows, centre-split
 *   EMPTY             1,000,000
 *   EMPTY               500,000
 *  ─────────────────────────────────────────
 *       HEART RATE: 72 BPM                   stats, centre-aligned
 *          CALORIES: 540 KCAL                label white, value red
 *       ACTIVE MINS: 45
 *           BATTERY: 87%
 *          RECOVERY: STABLE
 *  ─────────────────────────────────────────
 *         PLAY NEXT LEVEL                    red  (matches last digit)
 *        SAVE CURRENT GAME                   gold
 *        BACK TO MAIN MENU                   gold
 * ─────────────────────────────────────────────
 *
 * Settings (appKeys alphabetical → index):
 *   Key 0: A_SHOW_DATE    0=hide  1=show
 *   Key 1: B_SHOW_MENU    0=hide  1=show
 *   Key 2: C_SHOW_STEPS   0=battery  1=steps on line 4
 *   Key 3: D_TIME_LAST    0=red  1=gold  (last digit + accent colour)
 */

#include <pebble.h>

#define SCR_W        144
#define SCR_H        168
#define SETTINGS_KEY   1

/* ── Accent colour (last digit, stat values, PLAY NEXT LEVEL) ── */
/* toggled by D_TIME_LAST setting */
#define COL_RED      GColorRed
#define COL_GOLD     GColorChromeYellow
#define COL_WHITE    GColorWhite
#define COL_DIM      GColorDarkGray
#define COL_DIMV     GColorVeryLightGray   /* used inverted — actually very dark */
#define COL_BLUE     GColorCobaltBlue
#define COL_BLACK    GColorBlack

/* ── Layout ── */
#define BAR_Y          6
#define BAR_H         13
#define BAR_X_START   10   /* rounded left cap starts here            */

/* Capsule: drawn dynamically around measured digit size             */
#define CAP_Y         22   /* top of capsule                         */
#define CAP_PAD_X      6   /* horizontal padding inside capsule (px)  */
#define CAP_PAD_Y      4   /* vertical padding inside capsule (px)    */

/* Score rows below capsule — y set dynamically after capsule draw  */
#define SCORE_LH       9
#define SCORE_N        3

/* Stats */
#define STATS_LH       9
#define STATS_N        5

/* Menu */
#define MENU_LH        9
#define MENU_N         3

/* ── Settings ── */
typedef struct {
  uint8_t show_date;   /* 0=hide  1=show  */
  uint8_t show_menu;   /* 0=hide  1=show  */
  uint8_t show_steps;  /* 0=battery  1=steps */
  uint8_t time_last;   /* 0=red  1=gold   */
} Settings;

static Settings s = {
  .show_date  = 1,
  .show_menu  = 1,
  .show_steps = 0,
  .time_last  = 0,
};

static void settings_load(void) { persist_read_data(SETTINGS_KEY, &s, sizeof(s)); }
static void settings_save(void) { persist_write_data(SETTINGS_KEY, &s, sizeof(s)); }

static GColor accent(void) { return s.time_last ? COL_GOLD : COL_RED; }

/* ── Globals ── */
static Window *s_win;
static Layer  *s_canvas;

static GFont s_font_eb_24;  /* Orbitron ExtraBold 24 — time digits  */
static GFont s_font_bd_8;   /* Orbitron Bold 8      — date/scores/stats */
static GFont s_font_eb_8;   /* Orbitron ExtraBold 8 — menu          */

static char s_hh[3];
static char s_mm_h[2];   /* tens digit of minutes */
static char s_mm_l[2];   /* units digit of minutes — accent colour */
static char s_date[18];
static char s_score[14];
static char s_stats[STATS_N][24];
static char s_menu[MENU_N][20];
static int  s_batt = 100;

/* ── Comma formatter ── */
static void fmt_comma(int n, char *buf, int sz) {
  if      (n < 1000)    snprintf(buf, sz, "%d", n);
  else if (n < 1000000) snprintf(buf, sz, "%d,%03d", n/1000, n%1000);
  else                  snprintf(buf, sz, "%d,%03d,%03d", n/1000000, (n/1000)%1000, n%1000);
}

/* ── Canvas ── */
static void canvas_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GColor acc   = accent();

  /* Black background */
  graphics_context_set_fill_color(ctx, COL_BLACK);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  /* ── DATE BAR ── blue pill, rounded left, bleeds right */
  if (s.show_date) {
    graphics_context_set_fill_color(ctx, COL_BLUE);
    /* Rounded rect from BAR_X_START to SCR_W+8 (bleed), radius = BAR_H/2 */
    graphics_fill_rect(ctx,
      GRect(BAR_X_START + BAR_H/2, BAR_Y, SCR_W + 8 - BAR_X_START - BAR_H/2, BAR_H),
      0, GCornerNone);
    graphics_fill_circle(ctx,
      GPoint(BAR_X_START + BAR_H/2, BAR_Y + BAR_H/2), BAR_H/2);

    graphics_context_set_text_color(ctx, COL_WHITE);
    graphics_draw_text(ctx, s_date, s_font_bd_8,
      GRect(BAR_X_START + 8, BAR_Y, SCR_W - BAR_X_START - 8, BAR_H),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  }

  /* ── TIME CAPSULE ── */
  /* Measure digit heights to size capsule */
  GSize sz_hh = graphics_text_layout_get_content_size(
    "00", s_font_eb_24, GRect(0,0,SCR_W,40), GTextOverflowModeWordWrap, GTextAlignmentLeft);
  int dig_h = sz_hh.h;
  int cap_h = dig_h + CAP_PAD_Y * 2;
  int cap_y1 = CAP_Y;
  int cap_y2 = cap_y1 + cap_h;
  int cap_r  = cap_h / 2;

  /* Capsule border — white */
  graphics_context_set_stroke_color(ctx, COL_WHITE);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_round_rect(ctx,
    GRect(CAP_PAD_X, cap_y1, SCR_W - CAP_PAD_X*2, cap_h), cap_r);

  /* Time digits — split rendering: HH + ":" white, MM tens white, MM units accent */
  int text_y = cap_y1 + CAP_PAD_Y - 2;

  /* HH left of centre */
  graphics_context_set_text_color(ctx, COL_WHITE);
  graphics_draw_text(ctx, s_hh, s_font_eb_24,
    GRect(CAP_PAD_X + 4, text_y, SCR_W/2 - CAP_PAD_X - 4, dig_h + 4),
    GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);

  /* Colon */
  graphics_draw_text(ctx, ":", s_font_eb_24,
    GRect(SCR_W/2 - 6, text_y, 12, dig_h + 4),
    GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  /* MM tens — white */
  graphics_draw_text(ctx, s_mm_h, s_font_eb_24,
    GRect(SCR_W/2 + 4, text_y, 24, dig_h + 4),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  /* MM units — accent colour */
  graphics_context_set_text_color(ctx, acc);
  GSize sz_dig = graphics_text_layout_get_content_size(
    s_mm_h, s_font_eb_24, GRect(0,0,SCR_W,40), GTextOverflowModeWordWrap, GTextAlignmentLeft);
  graphics_draw_text(ctx, s_mm_l, s_font_eb_24,
    GRect(SCR_W/2 + 4 + sz_dig.w, text_y, 24, dig_h + 4),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  int score_y = cap_y2 + 3;

  /* ── SCORE ROWS ── centre-split at SCR_W/2 */
  static const char * const score_labels[SCORE_N] = {"PLAYER ONE","EMPTY","EMPTY"};

  for (int i = 0; i < SCORE_N; i++) {
    GColor lc = (i == 0) ? COL_WHITE : COL_DIM;
    GColor vc = (i == 0) ? COL_WHITE : GColorBlack;

    /* Label right-aligned to centre-2 */
    graphics_context_set_text_color(ctx, lc);
    graphics_draw_text(ctx, score_labels[i], s_font_bd_8,
      GRect(4, score_y, SCR_W/2 - 6, SCORE_LH + 2),
      GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);

    /* Value left-aligned from centre+2 */
    if (i == 0) {
      graphics_context_set_text_color(ctx, COL_WHITE);
      graphics_draw_text(ctx, s_score, s_font_bd_8,
        GRect(SCR_W/2 + 2, score_y, SCR_W/2 - 6, SCORE_LH + 2),
        GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    }
    score_y += SCORE_LH;
  }

  /* ── DIVIDER 1 ── */
  int div1_y = score_y + 2;
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(8, div1_y), GPoint(SCR_W - 8, div1_y));

  /* ── STATS ── centred: label white, value accent */
  int stat_y = div1_y + 3;
  for (int i = 0; i < STATS_N; i++) {
    graphics_context_set_text_color(ctx, COL_WHITE);
    graphics_draw_text(ctx, s_stats[i], s_font_bd_8,
      GRect(0, stat_y, SCR_W, STATS_LH + 2),
      GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    stat_y += STATS_LH;
  }

  /* ── DIVIDER 2 ── */
  if (s.show_menu) {
    int div2_y = stat_y + 2;
    graphics_context_set_stroke_color(ctx, GColorDarkGray);
    graphics_draw_line(ctx, GPoint(8, div2_y), GPoint(SCR_W - 8, div2_y));

    /* ── MENU ── */
    int menu_y = div2_y + 3;
    for (int i = 0; i < MENU_N; i++) {
      GColor mc = (i == 0) ? acc : COL_GOLD;
      graphics_context_set_text_color(ctx, mc);
      graphics_draw_text(ctx, s_menu[i], s_font_eb_8,
        GRect(0, menu_y, SCR_W, MENU_LH + 2),
        GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
      menu_y += MENU_LH;
    }
  }
}

/* ── Data update ── */
static void update_display(struct tm *t) {
  /* Time — split into HH, MM tens, MM units */
  snprintf(s_hh,   sizeof(s_hh),   "%02d", t->tm_hour);
  snprintf(s_mm_h, sizeof(s_mm_h), "%d",   t->tm_min / 10);
  snprintf(s_mm_l, sizeof(s_mm_l), "%d",   t->tm_min % 10);

  /* Date */
  static const char * const MON[12] = {
    "JAN","FEB","MAR","APR","MAY","JUN",
    "JUL","AUG","SEP","OCT","NOV","DEC"
  };
  static const char * const DOW[7] = {
    "SUN","MON","TUE","WED","THU","FRI","SAT"
  };
  snprintf(s_date, sizeof(s_date), "%s, %02d %s %02d",
    DOW[t->tm_wday], t->tm_mday, MON[t->tm_mon], (t->tm_year+1900)%100);

  /* Score — steps as high score */
  HealthValue steps = health_service_sum_today(HealthMetricStepCount);
  fmt_comma((int)steps, s_score, sizeof(s_score));

  /* Stats — label and value in one string (centred together) */
  HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
  snprintf(s_stats[0], 24, "HEART RATE: %d BPM", (int)(hr > 0 ? hr : 0));

  HealthValue cal = health_service_sum_today(HealthMetricActiveKCalories);
  snprintf(s_stats[1], 24, "CALORIES: %d KCAL", (int)cal);

  HealthValue secs = health_service_sum_today(HealthMetricActiveSeconds);
  snprintf(s_stats[2], 24, "ACTIVE MINS: %d", (int)(secs/60));

  if (s.show_steps) {
    char sc[10]; fmt_comma((int)steps, sc, sizeof(sc));
    snprintf(s_stats[3], 24, "STEPS: %s", sc);
  } else {
    snprintf(s_stats[3], 24, "BATTERY: %d%%", s_batt);
  }

  HealthValue dist = health_service_sum_today(HealthMetricWalkedDistanceMeters);
  int rec = (int)(dist/50); if (rec > 99) rec = 99;
  const char *rec_str = rec > 75 ? "OPTIMAL" : rec > 50 ? "STABLE" : rec > 25 ? "LOW" : "CRITICAL";
  snprintf(s_stats[4], 24, "RECOVERY: %s", rec_str);

  /* Menu */
  snprintf(s_menu[0], 20, "PLAY NEXT LEVEL");
  snprintf(s_menu[1], 20, "SAVE CURRENT GAME");
  snprintf(s_menu[2], 20, "BACK TO MAIN MENU");

  layer_mark_dirty(s_canvas);
}

/* ── AppMessage ── */
static void inbox_received(DictionaryIterator *iter, void *ctx) {
  Tuple *t;
  t = dict_find(iter, 0); if (t) s.show_date  = (uint8_t)t->value->int32;
  t = dict_find(iter, 1); if (t) s.show_menu  = (uint8_t)t->value->int32;
  t = dict_find(iter, 2); if (t) s.show_steps = (uint8_t)t->value->int32;
  t = dict_find(iter, 3); if (t) s.time_last  = (uint8_t)t->value->int32;
  settings_save();
  layer_mark_dirty(s_canvas);
}

/* ── Callbacks ── */
static void tick_cb(struct tm *t, TimeUnits u)        { update_display(t); }
static void batt_cb(BatteryChargeState bs)             { s_batt = bs.charge_percent; layer_mark_dirty(s_canvas); }
static void health_cb(HealthEventType type, void *ctx) {
  time_t now = time(NULL); update_display(localtime(&now));
}

/* ── Window ── */
static void window_load(Window *w) {
  Layer *root = window_get_root_layer(w);

  /*
   * CloudPebble resources:
   *   Orbitron-ExtraBold.ttf → FONT_ORB_EB_24  size 24
   *   Orbitron-Bold.ttf      → FONT_ORB_BD_8   size 8
   *   Orbitron-ExtraBold.ttf → FONT_ORB_EB_8   size 8
   */
  s_font_eb_24 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORB_EB_24));
  s_font_bd_8  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORB_BD_8));
  s_font_eb_8  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORB_EB_8));

  window_set_background_color(w, GColorBlack);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);

  s_batt = battery_state_service_peek().charge_percent;
  time_t now = time(NULL); update_display(localtime(&now));
}

static void window_unload(Window *w) {
  layer_destroy(s_canvas);
  fonts_unload_custom_font(s_font_eb_24);
  fonts_unload_custom_font(s_font_bd_8);
  fonts_unload_custom_font(s_font_eb_8);
}

/* ── Init ── */
static void init(void) {
  settings_load();
  app_message_open(64, 64);
  app_message_register_inbox_received(inbox_received);
  s_win = window_create();
  window_set_window_handlers(s_win, (WindowHandlers){
    .load = window_load, .unload = window_unload
  });
  window_stack_push(s_win, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_cb);
  battery_state_service_subscribe(batt_cb);
  health_service_events_subscribe(health_cb, NULL);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  health_service_events_unsubscribe();
  window_destroy(s_win);
}

int main(void) { init(); app_event_loop(); deinit(); }
