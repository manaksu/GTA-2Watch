/*
 * GTA2Watch -- Pebble Time Steel (basalt) only
 * Font: Orbitron weight 900 (instantiated from variable font)
 *       FONT_ORB_14 size 14 — time digits
 *       FONT_ORB_6  size 6  — all body text
 *
 * Layout (144 x 168)
 * ─────────────────────────────────────────
 *  [■ FRI, 08 MAY 26 ═══════════════════► ]  y=4   blue pill, bleeds right
 *         ( 2  3 : 4  2 )                     y=16  capsule
 *        PLAYER ONE  12,500                   y=ac  scores
 *             EMPTY
 *             EMPTY
 *  ──────────────────────────────────────      div
 *       HEART RATE: 72 BPM                    stats
 *          CALORIES: 540 KCAL
 *       ACTIVE MINS: 45
 *           BATTERY: 87%
 *          RECOVERY: STABLE
 *  ──────────────────────────────────────      div
 *        PLAY NEXT LEVEL                      menu
 *       SAVE CURRENT GAME
 *       BACK TO MAIN MENU
 *
 * Settings (appKeys alphabetical -> index):
 *   Key 0: A_SHOW_DATE   0=hide  1=show
 *   Key 1: B_SHOW_MENU   0=hide  1=show
 *   Key 2: C_SHOW_STEPS  0=battery  1=steps
 *   Key 3: D_ACCENT      0=red  1=gold
 */

#include <pebble.h>

#define SCR_W        144
#define SCR_H        168
#define SETTINGS_KEY   1

#define COL_WHITE   GColorWhite
#define COL_DIM     GColorDarkGray
#define COL_BLUE    GColorCobaltBlue
#define COL_BLACK   GColorBlack
#define COL_RED     GColorRed
#define COL_GOLD    GColorChromeYellow

/* ── Layout ── */
#define BAR_Y          4
#define BAR_H          9
#define BAR_X         10
#define CAP_Y         16
#define CAP_H         14
#define CAP_R          7
#define CAP_PAD_X      5
#define SCORE_Y       38   /* cap_y + cap_h + 8 */
#define SCORE_LH       9
#define DIV1_Y        67   /* score_y + 3*lh + 4 */
#define STATS_Y       72
#define STATS_LH       9
#define STATS_N        5
#define DIV2_Y       121
#define MENU_Y       126
#define MENU_LH        9
#define MENU_N         3

/* ── Settings ── */
typedef struct {
  uint8_t show_date;
  uint8_t show_menu;
  uint8_t show_steps;
  uint8_t accent;
} Settings;

static Settings s = { .show_date=1, .show_menu=1, .show_steps=0, .accent=0 };
static void settings_load(void) { persist_read_data(SETTINGS_KEY, &s, sizeof(s)); }
static void settings_save(void) { persist_write_data(SETTINGS_KEY, &s, sizeof(s)); }
static GColor acc(void) { return s.accent ? COL_GOLD : COL_RED; }

/* ── Globals ── */
static Window *s_win;
static Layer  *s_canvas;
static GFont   s_font_time;   /* Orbitron 900 size 14 */
static GFont   s_font_body;   /* Orbitron 900 size 6  */

static char s_hh[3];
static char s_mm_tens[2];
static char s_mm_unit[2];
static char s_date[18];
static char s_score[22];
static char s_stats[STATS_N][28];
static char s_menu[MENU_N][20];
static int  s_batt = 100;

static void fmt_comma(int n, char *buf, int sz) {
  if      (n < 1000)    snprintf(buf, sz, "%d", n);
  else if (n < 1000000) snprintf(buf, sz, "%d,%03d", n/1000, n%1000);
  else                  snprintf(buf, sz, "%d,%03d,%03d", n/1000000,(n/1000)%1000,n%1000);
}

static void draw_center(GContext *ctx, const char *txt, GFont font, int y, int h, GColor col) {
  graphics_context_set_text_color(ctx, col);
  graphics_draw_text(ctx, txt, font,
    GRect(0, y, SCR_W, h+2),
    GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void canvas_update(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, COL_BLACK);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);

  GColor accent = acc();

  /* ── DATE BAR ── */
  if (s.show_date) {
    graphics_context_set_fill_color(ctx, COL_BLUE);
    graphics_fill_circle(ctx, GPoint(BAR_X + BAR_H/2, BAR_Y + BAR_H/2), BAR_H/2);
    graphics_fill_rect(ctx,
      GRect(BAR_X + BAR_H/2, BAR_Y, SCR_W - BAR_X, BAR_H), 0, GCornerNone);
    graphics_context_set_text_color(ctx, COL_WHITE);
    graphics_draw_text(ctx, s_date, s_font_body,
      GRect(BAR_X + 8, BAR_Y + 1, SCR_W - BAR_X - 8, BAR_H),
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  }

  /* ── CAPSULE ── */
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_round_rect(ctx,
    GRect(CAP_PAD_X, CAP_Y, SCR_W - CAP_PAD_X*2, CAP_H), CAP_R);

  int ty  = CAP_Y + 1;
  int mid = SCR_W / 2;

  /* HH white */
  graphics_context_set_text_color(ctx, COL_WHITE);
  graphics_draw_text(ctx, s_hh, s_font_time,
    GRect(CAP_PAD_X, ty, mid - CAP_PAD_X - 4, CAP_H),
    GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);

  /* Colon white */
  graphics_draw_text(ctx, ":", s_font_time,
    GRect(mid - 4, ty, 8, CAP_H),
    GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  /* MM tens white */
  graphics_draw_text(ctx, s_mm_tens, s_font_time,
    GRect(mid + 3, ty, 14, CAP_H),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  /* MM unit accent */
  graphics_context_set_text_color(ctx, accent);
  graphics_draw_text(ctx, s_mm_unit, s_font_time,
    GRect(mid + 15, ty, 14, CAP_H),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  /* ── SCORES ── */
  draw_center(ctx, s_score, s_font_body, SCORE_Y,          SCORE_LH, COL_WHITE);
  draw_center(ctx, "EMPTY", s_font_body, SCORE_Y+SCORE_LH, SCORE_LH, COL_DIM);
  draw_center(ctx, "EMPTY", s_font_body, SCORE_Y+SCORE_LH*2, SCORE_LH, COL_DIM);

  /* ── DIVIDER 1 ── */
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(6, DIV1_Y), GPoint(SCR_W-6, DIV1_Y));

  /* ── STATS ── */
  for (int i = 0; i < STATS_N; i++) {
    draw_center(ctx, s_stats[i], s_font_body,
      STATS_Y + i*STATS_LH, STATS_LH, COL_WHITE);
  }

  /* ── DIVIDER 2 + MENU ── */
  if (s.show_menu) {
    graphics_draw_line(ctx, GPoint(6, DIV2_Y), GPoint(SCR_W-6, DIV2_Y));
    for (int i = 0; i < MENU_N; i++) {
      GColor mc = (i == 0) ? accent : COL_GOLD;
      draw_center(ctx, s_menu[i], s_font_body, MENU_Y + i*MENU_LH, MENU_LH, mc);
    }
  }
}

static void update_display(struct tm *t) {
  snprintf(s_hh,      sizeof(s_hh),      "%02d", t->tm_hour);
  snprintf(s_mm_tens, sizeof(s_mm_tens), "%d",   t->tm_min/10);
  snprintf(s_mm_unit, sizeof(s_mm_unit), "%d",   t->tm_min%10);

  static const char * const MON[12] = {
    "JAN","FEB","MAR","APR","MAY","JUN",
    "JUL","AUG","SEP","OCT","NOV","DEC"
  };
  static const char * const DOW[7] = {
    "SUN","MON","TUE","WED","THU","FRI","SAT"
  };
  snprintf(s_date, sizeof(s_date), "%s, %02d %s %02d",
    DOW[t->tm_wday], t->tm_mday, MON[t->tm_mon], (t->tm_year+1900)%100);

  HealthValue steps = health_service_sum_today(HealthMetricStepCount);
  char sc[12]; fmt_comma((int)steps, sc, sizeof(sc));
  snprintf(s_score, sizeof(s_score), "PLAYER ONE  %s", sc);

  HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
  snprintf(s_stats[0], 28, "HEART RATE: %d BPM", (int)(hr>0?hr:0));

  HealthValue cal = health_service_sum_today(HealthMetricActiveKCalories);
  snprintf(s_stats[1], 28, "CALORIES: %d KCAL", (int)cal);

  HealthValue secs = health_service_sum_today(HealthMetricActiveSeconds);
  snprintf(s_stats[2], 28, "ACTIVE MINS: %d", (int)(secs/60));

  if (s.show_steps) {
    char sv[12]; fmt_comma((int)steps, sv, sizeof(sv));
    snprintf(s_stats[3], 28, "STEPS: %s", sv);
  } else {
    snprintf(s_stats[3], 28, "BATTERY: %d%%", s_batt);
  }

  HealthValue dist = health_service_sum_today(HealthMetricWalkedDistanceMeters);
  int rec = (int)(dist/50); if(rec>99) rec=99;
  const char *rs = rec>75?"OPTIMAL":rec>50?"STABLE":rec>25?"LOW":"CRITICAL";
  snprintf(s_stats[4], 28, "RECOVERY: %s", rs);

  snprintf(s_menu[0], 20, "PLAY NEXT LEVEL");
  snprintf(s_menu[1], 20, "SAVE CURRENT GAME");
  snprintf(s_menu[2], 20, "BACK TO MAIN MENU");

  layer_mark_dirty(s_canvas);
}

static void inbox_received(DictionaryIterator *iter, void *ctx) {
  Tuple *t;
  t=dict_find(iter,0); if(t) s.show_date =(uint8_t)t->value->int32;
  t=dict_find(iter,1); if(t) s.show_menu =(uint8_t)t->value->int32;
  t=dict_find(iter,2); if(t) s.show_steps=(uint8_t)t->value->int32;
  t=dict_find(iter,3); if(t) s.accent    =(uint8_t)t->value->int32;
  settings_save();
  layer_mark_dirty(s_canvas);
}

static void tick_cb(struct tm *t, TimeUnits u)        { update_display(t); }
static void batt_cb(BatteryChargeState bs)             { s_batt=bs.charge_percent; layer_mark_dirty(s_canvas); }
static void health_cb(HealthEventType type, void *ctx) {
  time_t now=time(NULL); update_display(localtime(&now));
}

static void window_load(Window *w) {
  Layer *root = window_get_root_layer(w);
  s_font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORB_14));
  s_font_body = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORB_6));
  window_set_background_color(w, COL_BLACK);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);
  s_batt = battery_state_service_peek().charge_percent;
  time_t now = time(NULL); update_display(localtime(&now));
}

static void window_unload(Window *w) {
  layer_destroy(s_canvas);
  fonts_unload_custom_font(s_font_time);
  fonts_unload_custom_font(s_font_body);
}

static void init(void) {
  settings_load();
  app_message_register_inbox_received(inbox_received);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  s_win = window_create();
  window_set_window_handlers(s_win, (WindowHandlers){
    .load=window_load, .unload=window_unload
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
