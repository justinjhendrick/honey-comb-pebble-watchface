#include <pebble.h>
#include "drawing.h"
#include "utils.h"

#define DEBUG_TIME (false)
#define BUFFER_LEN (100)
#define DEG_PER_MIN (360 / 60)
#define COL_BG (PBL_IF_BW_ELSE(GColorWhite, GColorChromeYellow))
#define COL_LT (PBL_IF_BW_ELSE(GColorBlack, GColorYellow))
#define COL_DK (PBL_IF_BW_ELSE(GColorBlack, GColorBulgarianRose))

static Window* s_window;
static Layer* s_layer;
static char s_buffer[BUFFER_LEN];
static GPath* s_hexagon;
static GPath* s_arrow;

static const GPathInfo HEXAGON_POINTS = {
  .num_points = 6,
  // points will be filled by fill_hexagon_points
  .points = (GPoint []) {
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
  }
};

static const GPathInfo ARROW_POINTS = {
  .num_points = 4,
  // points will be filled by change_arrow_size
  .points = (GPoint []) {
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
  }
};

static int hex_height(int W) {
  return W * 1000 / 866;  // 2 / sqrt(3)
}

static int hex_width(int H) {
  return H * 866 / 1000;  // sqrt(3) / 2
}

static int hex_half_side_length(int H) {
  return H / 4;
}

static void fill_hexagon_points(int H) {
  // 0, 0 is center of hex
  int R = H / 2;
  int W = hex_width(H);
  int r = W / 2;
  int hsl = hex_half_side_length(H);

  // bottom left
  HEXAGON_POINTS.points[0].x = -r;
  HEXAGON_POINTS.points[0].y = hsl;

  // top left
  HEXAGON_POINTS.points[1].x = -r;
  HEXAGON_POINTS.points[1].y = -hsl;

  // top up pointy
  HEXAGON_POINTS.points[2].x = 0;
  HEXAGON_POINTS.points[2].y = -R;

  // top right
  HEXAGON_POINTS.points[3].x = r;
  HEXAGON_POINTS.points[3].y = -hsl;

  // bottom right
  HEXAGON_POINTS.points[4].x = r;
  HEXAGON_POINTS.points[4].y = hsl;

  // bottom down pointy
  HEXAGON_POINTS.points[5].x = 0;
  HEXAGON_POINTS.points[5].y = R;
}

static void fast_forward_time(struct tm* now) {
  now->tm_min = now->tm_sec % 60;
}

static void draw_hexagon(GContext* ctx, GPoint center, int H) {
  fill_hexagon_points(H);
  gpath_move_to(s_hexagon, center);
  gpath_draw_outline(ctx, s_hexagon);
}

static void fill_hexagon(GContext* ctx, GPoint center, int H) {
  fill_hexagon_points(H);
  gpath_move_to(s_hexagon, center);
  gpath_draw_filled(ctx, s_hexagon);
}

static void tessellate(GContext* ctx, GPoint origin, int H, bool start_left, int* rows, int num_rows, GPoint* centers) {
  int W = hex_width(H);
  int hsl = hex_half_side_length(H);
  int y_stride = H / 2 + hsl;
  int x_stride = W;
  int row_stagger = start_left;
  int y_begin = origin.y + 2 * hsl + 1;
  int i = 0;
  for (int r = 0; r < num_rows; r++) {
    int y = y_begin + y_stride * r;
    int x_begin = origin.x + x_stride / 8 - row_stagger * x_stride / 2;
    for (int c = 0; c < rows[r]; c++) {
      int x = x_begin + x_stride * c;
      GPoint center = GPoint(x, y);
      centers[i++] = center;
    }
    row_stagger = !row_stagger;
  }
}

static void change_arrow_size(int w, int h) {
  ARROW_POINTS.points[0].x = 0;
  ARROW_POINTS.points[0].y = 0;

  ARROW_POINTS.points[1].x = w / 2;
  ARROW_POINTS.points[1].y = -h / 3;

  ARROW_POINTS.points[2].x = 0;
  ARROW_POINTS.points[2].y = -h;

  ARROW_POINTS.points[3].x = -w / 2;
  ARROW_POINTS.points[3].y = -h / 3;
}

static void draw_hand(GContext* ctx, GPoint center, int angle_deg, int hand_width, int hand_length) {
  // location and shape
  change_arrow_size(hand_width, hand_length);
  gpath_rotate_to(s_arrow, DEG_TO_TRIGANGLE(angle_deg));
  gpath_move_to(s_arrow, center);

  // hand fill
  graphics_context_set_fill_color(ctx, COL_BG);
  gpath_draw_filled(ctx, s_arrow);

  // hand outline
  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_stroke_color(ctx, COL_DK);
  gpath_draw_outline(ctx, s_arrow);

  // Circle at the base to smooth out the rotation
  graphics_context_set_fill_color(ctx, COL_DK);
  graphics_fill_circle(ctx, center, 2);
}

static void draw_ticks(GContext* ctx, GPoint center, int radius, bool hour_text) {
  int text_bbox_height = max(10, radius / 4);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_stroke_color(ctx, COL_DK);
  for (int16_t tick_minute = 0; tick_minute < 60; tick_minute += 5) {
    int tick_deg = tick_minute * DEG_PER_MIN;
    int tick_length = radius / 2;
    GPoint minute_tick_outer = cartesian_from_polar(center, radius - 1, tick_deg);
    GPoint minute_tick_inner = cartesian_from_polar(center, radius - tick_length, tick_deg);
    if (tick_minute % 10 == 0) {
      // put text in the hex corners
      GPoint text_mp = cartesian_from_polar(center, radius - text_bbox_height / 4, tick_deg);
      GRect text_bbox = rect_from_midpoint(text_mp, GSize(text_bbox_height, text_bbox_height));
      if (hour_text) {
        int tick_hour = tick_minute * 12 / 60;
        if (tick_hour == 0) {
          tick_hour = 12;
        }
        draw_digits(ctx, tick_hour, text_bbox);
      } else {
        draw_digits(ctx, tick_minute, text_bbox);
      }
      // shorten tick when we have text
      minute_tick_outer = cartesian_from_polar(center, radius - text_bbox_height, tick_deg);
    }
    graphics_draw_line(ctx, minute_tick_inner, minute_tick_outer);
  }
}

static char wday_letter(int wday_idx) {
  if (wday_idx == 0) {
    return 'S';
  } else if (wday_idx == 1) {
    return 'M';
  } else if (wday_idx == 2) {
    return 'T';
  } else if (wday_idx == 3) {
    return 'W';
  } else if (wday_idx == 4) {
    return 'T';
  } else if (wday_idx == 5) {
    return 'F';
  } else if (wday_idx == 6) {
    return 'S';
  } else {
    return '?';
  }
}

static void draw_wday(GContext* ctx, GPoint center, int big_hex_height, int wday) {
  GPoint centers[7];
  int rows[3] = {2, 3, 2};
  int big_hex_hsl = hex_half_side_length(big_hex_height);
  int big_hex_width = hex_width(big_hex_height);
  int lil_hex_height = 4 * big_hex_hsl / 5;
  int lil_hex_width = hex_width(lil_hex_height);
  tessellate(ctx, GPoint(center.x + lil_hex_width / 2 - 4, center.y - big_hex_hsl), lil_hex_height, false, rows, 3, centers);
  graphics_context_set_stroke_width(ctx, 1);
  for (int center_idx = 0; center_idx < 7; center_idx++) {
    GPoint center = centers[center_idx];

    graphics_context_set_stroke_color(ctx, COL_DK);
    draw_hexagon(ctx, center, lil_hex_height);

    if (wday == center_idx) {
      graphics_context_set_stroke_color(ctx, COL_BG);
      graphics_context_set_fill_color(ctx, COL_DK);
      fill_hexagon(ctx, center, lil_hex_height);
    } else {
      graphics_context_set_stroke_color(ctx, COL_DK);
    }
    GRect text_bbox = rect_from_midpoint(center, GSize(lil_hex_height / 2, lil_hex_width / 2));
    draw_one_letter(ctx, wday_letter(center_idx), text_bbox);
  }
}

static void update_layer(Layer* layer, GContext* ctx) {
  time_t temp = time(NULL);
  struct tm* now = localtime(&temp);
  if (DEBUG_TIME) {
    fast_forward_time(now);
  }


  GRect bounds = layer_get_bounds(layer);
  int H = (bounds.size.h - 2) * 4 / 7;
  int W = hex_width(H);
  int hex_boundary_stroke_width = max(3, H / 30);

  int lil_rows[10] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
  GPoint lil_hex_centers[100];
  tessellate(ctx, bounds.origin, H / 10, true, lil_rows, 10, lil_hex_centers);
  for (int i = 0; i < 100; i++) {
    draw_hexagon(ctx, lil_hex_centers[i], H);
  }

  int rows[2] = {3, 3};
  GPoint hex_centers[6];
  tessellate(ctx, bounds.origin, H, true, rows, 2, hex_centers);

  graphics_context_set_stroke_width(ctx, hex_boundary_stroke_width);
  graphics_context_set_stroke_color(ctx, COL_LT);
  for (int i = 0; i < 6; i++) {
    draw_hexagon(ctx, hex_centers[i], H);
  }
  int radius = W / 2 - hex_boundary_stroke_width / 2;
  GPoint hours_center = hex_centers[1];

  // draw hours watchface in a hexagon
  draw_ticks(ctx, hours_center, radius, true);
  int minutes_per_hour_hand_rev = 60 * 12;
  int minutes_elapsed = now->tm_hour * 60 + now->tm_min;
  int hour_angle_degrees = 360 * minutes_elapsed / minutes_per_hour_hand_rev;
  int hour_hand_width = 9;
  int hour_hand_length = radius * 7 / 10;
  draw_hand(ctx, hours_center, hour_angle_degrees % 360, hour_hand_width, hour_hand_length);

  // draw minutes watchface in a hexagon
  GPoint minutes_center = hex_centers[4];
  draw_ticks(ctx, minutes_center, radius, false);
  int minute_hand_width = 8;
  int minute_hand_length = radius * 9 / 10;
  draw_hand(ctx, minutes_center, now->tm_min * DEG_PER_MIN, minute_hand_width, minute_hand_length);

  // draw day of week in a hexagon
  draw_wday(ctx, hex_centers[3], H, now->tm_wday);
}

static void window_load(Window* window) {
  Layer* window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(s_window, COL_BG);
  s_layer = layer_create(bounds);
  layer_set_update_proc(s_layer, update_layer);
  layer_add_child(window_layer, s_layer);
}

static void window_unload(Window* window) {
  layer_destroy(s_layer);
}

static void tick_handler(struct tm* now, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_window));
}

static void init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  s_arrow = gpath_create(&ARROW_POINTS);
  s_hexagon = gpath_create(&HEXAGON_POINTS);
  tick_timer_service_subscribe(DEBUG_TIME ? SECOND_UNIT : MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
