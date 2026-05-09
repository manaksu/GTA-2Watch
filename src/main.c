/*
 * GTA2Watch -- Pebble Time Steel (basalt) only
 * Font: Orbitron weight 900 (static instance)
 *   FONT_ORB_20 size 20 — time (drawn twice for extra boldness)
 *   FONT_ORB_7  size 7  — all body text
 *
 * Layout (144 x 168)
 * ─────────────────────────────────────────
 *  [■ FRI, 08 MAY 26 ═══════════════════► ]  y=3
 *         ( 2  3 : 5  0 )                     y=16
 *        PLAYER ONE  12,500                   y=44
 *             EMPTY                           y=53
 *             EMPTY                           y=61
 *
 *       HEART RATE: 72 BPM                   y=74
 *          CALORIES: 540 KCAL                y=83
 *       ACTIVE MINS: 45                      y=92
 *           BATTERY: 87%                     y=101
 *          RECOVERY: STABLE                  y=110
 *
 *        PLAY NEXT LEVEL                     y=124
 *       SAVE CURRENT GAME                    y=133
 *       BACK TO MAIN MENU                    y=142
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
#define BAR_Y          3
#define BAR_H         10
#define BAR_X         10

#define CAP_Y         16
#define CAP_H         22
#define CAP_R         11
#define CAP_PAD_X      5

#define SCORE_Y       44
#define SCORE_LH       9

#define STATS_Y       74
#define STATS_LH       9
#define STATS_N        5

#define MENU_Y       124
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
static GFont   s_font_time;   /* Orbitron 900 size 20 */
static GFont   s_font_body;   /* Orbitron 900 size 7  */

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
  else                  snprintf(buf, sz, "%d,%03d,%03d",
                          n/1000000,(n/1000)%1000,n%1000);
}

/* Centre-aligned text helper */
static void draw_center(GContext *ctx, const char *txt, GFont font,
                        int y, int h, GColor col) {
  graphics_context_set_text_color(ctx, col);
  graphics_draw_text(ctx, txt, font,
    GRect(0, y, SCR_W, h+2),
    GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

/* Draw time digit bold — rendered twice, offset by 1px */
static void draw_time_digit(GContext *ctx, const char *ch, GFont font,
                             GRect r, GColor col) {
  graphics_context_set_text_color(ctx, col);
  graphics_draw_text(ctx, ch, font, r,
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  /* Second pass offset 1px right for extra stroke weight */
  GRect r2 = GRect(r.origin.x+1, r.origin.y, r.size.w, r.size.h);
  graphics_draw_text(ctx, ch, font, r2,
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}

static void canvas_update(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, COL_BLACK);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);

  GColor accent = acc();

  /* ── DATE BAR ── */
  if (s.show_date) {
    graphics_context_set_fill_color(ctx, COL_BLUE);
    graphics_fill_circle(ctx,
      GPoint(BAR_X + BAR_H/2, BAR_Y + BAR_H/2), BAR_H/2);
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

  int ty  = CAP_Y + 3;
  int mid = SCR_W / 2;

  /* HH — draw twice for boldness */
  draw_time_digit(ctx, s_hh, s_font_time,
    GRect(CAP_PAD_X, ty, mid - CAP_PAD_X - 4, CAP_H),
    COL_WHITE);
  /* Force right-align manually: not needed — just use right align */
  graphics_context_set_text_color(ctx, COL_WHITE);
  graphics_draw_text(ctx, s_hh, s_font_time,
    GRect(CAP_PAD_X, ty, mid - CAP_PAD_X - 4, CAP_H),
    GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);

  /* Colon */
  graphics_context_set_text_color(ctx, COL_WHITE);
  graphics_draw_text(ctx, ":", s_font_time,
    GRect(mid - 5, ty, 10, CAP_H),
    GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, ":", s_font_time,
    GRect(mid - 4, ty, 10, CAP_H),
    GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  /* MM tens white — twice */
  graphics_draw_text(ctx, s_mm_tens, s_font_time,
    GRect(mid + 4, ty, 18, CAP_H),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, s_mm_tens, s_font_time,
    GRect(mid + 5, ty, 18, CAP_H),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  /* MM unit accent — twice */
  graphics_context_set_text_color(ctx, accent);
  graphics_draw_text(ctx, s_mm_unit, s_font_time,
    GRect(mid + 18, ty, 18, CAP_H),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, s_mm_unit, s_font_time,
    GRect(mid + 19, ty, 18, CAP_H),
    GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  /* ── SCORES ── no divider lines */
  draw_center(ctx, s_score, s_font_body, SCORE_Y,            SCORE_LH, COL_WHITE);
  draw_center(ctx, "EMPTY", s_font_body, SCORE_Y + SCORE_LH, SCORE_LH, COL_DIM);
  draw_center(ctx, "EMPTY", s_font_body, SCORE_Y + SCORE_LH*2, SCORE_LH, COL_DIM);

  /* ── STATS ── no divider lines */
  for (int i = 0; i < STATS_N; i++) {
    draw_center(ctx, s_stats[i], s_font_body,
      STATS_Y + i * STATS_LH, STATS_LH, COL_WHITE);
  }

  /* ── MENU ── */
  if (s.show_menu) {
    for (int i = 0; i < MENU_N; i++) {
      GColor mc = (i == 0) ? accent : COL_GOLD;
      draw_center(ctx, s_menu[i], s_font_body,
        MENU_Y + i * MENU_LH, MENU_LH, mc);
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
  /*
   * CloudPebble resources — same Orbitron-900.ttf twice:
   *   FONT_ORB_20  size 20  — time digits
   *   FONT_ORB_7   size 7   — body text
   */
  s_font_time = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORB_20));
  s_font_body = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ORB_7));
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
