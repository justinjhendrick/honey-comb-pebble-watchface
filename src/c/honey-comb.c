#include <pebble.h>

#define DEBUG_TIME (false)
#define BUFFER_LEN (100)
#define DEG_PER_MIN (360 / 60)

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

static GRect rect_from_midpoint(GPoint midpoint, GSize size) {
  GRect ret;
  ret.origin.x = midpoint.x - size.w / 2;
  ret.origin.y = midpoint.y - size.h / 2;
  ret.size = size;
  return ret;
}

static int min(int a, int b) {
  if (a < b) {
    return a;
  }
  return b;
}

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
  now->tm_min = now->tm_sec;           /* Minutes. [0-59] */
  now->tm_hour = now->tm_sec % 24;     /* Hours.  [0-23] */
  now->tm_mday = now->tm_sec % 31 + 1; /* Day. [1-31] */
  now->tm_mon = now->tm_sec % 12;      /* Month. [0-11] */
  now->tm_wday = now->tm_sec % 7;      /* Day of week. [0-6] */
}

static void draw_hexagon(GContext* ctx, GPoint center, int stroke_w, int H) {
  fill_hexagon_points(H);
  graphics_context_set_stroke_width(ctx, stroke_w);
  graphics_context_set_stroke_color(ctx, GColorYellow);
  gpath_move_to(s_hexagon, center);
  gpath_draw_outline(ctx, s_hexagon);
}

static void tessellate(GContext* ctx, GRect bounds, int stroke_w, int H, GPoint* visible_centers) {
  int W = hex_width(H);
  int hsl = hex_half_side_length(H);
  int y_stride = H / 2 + hsl;
  int x_stride = W;
  int row_stagger = false;
  int y_begin = bounds.origin.y - hsl;
  int visible_center_count = 0;
  for (int y = y_begin; y <= bounds.size.h + y_stride; y += y_stride) {
    int x_begin = -x_stride - row_stagger * W / 2;
    for (int x = x_begin; x <= bounds.size.w + x_stride; x += x_stride) {
      GPoint center = GPoint(x, y);
      draw_hexagon(ctx, center, stroke_w, H);
      if (center.x - W / 2 >= 0
       && center.x + W / 2 <= bounds.size.w
       && center.y - H / 2 >= 0
       && center.y + H / 2 <= bounds.size.h) {
        visible_centers[visible_center_count] = center;
        visible_center_count++;
      }
    }
    row_stagger = !row_stagger;
  }
}

static GPoint cartesian_from_polar(GPoint center, int radius, int angle_deg) {
  GPoint ret = {
    .x = (int16_t)(sin_lookup(DEG_TO_TRIGANGLE(angle_deg)) * radius / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(DEG_TO_TRIGANGLE(angle_deg)) * radius / TRIG_MAX_RATIO) + center.y,
  };
  return ret;
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
  graphics_context_set_stroke_color(ctx, GColorBulgarianRose);
  graphics_context_set_fill_color(ctx, GColorBulgarianRose);
  graphics_context_set_stroke_width(ctx, 3);
  change_arrow_size(hand_width, hand_length);
  gpath_rotate_to(s_arrow, DEG_TO_TRIGANGLE(angle_deg));
  gpath_move_to(s_arrow, center);
  //gpath_draw_filled(ctx, s_arrow);
  gpath_draw_outline(ctx, s_arrow);
  // Circle at the base to smooth out the rotation
  graphics_fill_circle(ctx, center, 3);
}

static void draw_one_digit(GContext* ctx, int digit, GRect bbox) {
  int x = bbox.origin.x + 1;
  int y = bbox.origin.y + 1;
  int w = bbox.size.w - 1;
  int h = bbox.size.h - 1;
  GPoint tl = GPoint(x,     y);
  GPoint tr = GPoint(x + w, y);
  GPoint ml = GPoint(x,     y + h / 2);
  GPoint mr = GPoint(x + w, y + h / 2);
  GPoint bl = GPoint(x,     y + h);
  GPoint br = GPoint(x + w, y + h);
  if (digit == 0) {
    graphics_draw_line(ctx, tl, tr);
    graphics_draw_line(ctx, tr, br);
    graphics_draw_line(ctx, br, bl);
    graphics_draw_line(ctx, bl, tl);
  } else if (digit == 1) {
    GPoint tc = GPoint(x + w / 2, y);
    GPoint bc = GPoint(x + w / 2, y + h);
    graphics_draw_line(ctx, tc, bc);
  } else if (digit == 2) {
    graphics_draw_line(ctx, tl, tr);
    graphics_draw_line(ctx, tr, mr);
    graphics_draw_line(ctx, mr, ml);
    graphics_draw_line(ctx, ml, bl);
    graphics_draw_line(ctx, bl, br);
  } else if (digit == 3) {
    graphics_draw_line(ctx, tl, tr);
    graphics_draw_line(ctx, tr, br);
    graphics_draw_line(ctx, mr, ml);
    graphics_draw_line(ctx, br, bl);
  } else if (digit == 4) {
    graphics_draw_line(ctx, tl, ml);
    graphics_draw_line(ctx, ml, mr);
    graphics_draw_line(ctx, tr, br);
  } else if (digit == 5) {
    graphics_draw_line(ctx, tl, tr);
    graphics_draw_line(ctx, tl, ml);
    graphics_draw_line(ctx, ml, mr);
    graphics_draw_line(ctx, mr, br);
    graphics_draw_line(ctx, br, bl);
  } else if (digit == 6) {
    graphics_draw_line(ctx, tr, tl);
    graphics_draw_line(ctx, tl, bl);
    graphics_draw_line(ctx, bl, br);
    graphics_draw_line(ctx, br, mr);
    graphics_draw_line(ctx, mr, ml);
  } else if (digit == 7) {
    graphics_draw_line(ctx, tl, tr);
    graphics_draw_line(ctx, tr, br);
  } else if (digit == 8) {
    graphics_draw_line(ctx, tl, tr);
    graphics_draw_line(ctx, ml, mr);
    graphics_draw_line(ctx, bl, br);
    graphics_draw_line(ctx, tl, bl);
    graphics_draw_line(ctx, tr, br);
  } else if (digit == 9) {
    graphics_draw_line(ctx, mr, ml);
    graphics_draw_line(ctx, ml, tl);
    graphics_draw_line(ctx, tl, tr);
    graphics_draw_line(ctx, tr, br);
    graphics_draw_line(ctx, br, bl);
  }
}

static void draw_digits(GContext* ctx, int digits, GRect bbox) {
  int tens = digits / 10;
  GRect ones_bbox;
  graphics_context_set_stroke_color(ctx, GColorBulgarianRose);
  graphics_context_set_stroke_width(ctx, 1);
  if (tens != 0) {
    GRect tens_bbox = GRect(bbox.origin.x,                   bbox.origin.y, bbox.size.w / 2, bbox.size.h);
    ones_bbox       = GRect(bbox.origin.x + bbox.size.w / 2, bbox.origin.y, bbox.size.w / 2, bbox.size.h);
    draw_one_digit(ctx, tens, tens_bbox);
  } else {
    ones_bbox       = GRect(bbox.origin.x + bbox.size.w / 4, bbox.origin.y, bbox.size.w / 2, bbox.size.h);
  }
  int ones = digits % 10;
  draw_one_digit(ctx, ones, ones_bbox);
}

static void draw_ticks(GContext* ctx, GPoint center, int radius, int text_bbox_height, bool hour_text) {
  graphics_context_set_stroke_color(ctx, GColorBulgarianRose);
  for (int16_t tick_minute = 0; tick_minute < 60; tick_minute += 1) {
    int tick_deg = tick_minute * DEG_PER_MIN;
    GPoint minute_tick_outer = cartesian_from_polar(center, radius, tick_deg);
    int tick_length = 1;
    if (tick_minute % 10 == 0) {
      GPoint text_mp = cartesian_from_polar(center, radius + 3 * text_bbox_height / 4, tick_deg);
      GRect text_bbox = rect_from_midpoint(text_mp, GSize(text_bbox_height, text_bbox_height));
      if (hour_text) {
        int tick_hour = tick_minute * 12 / 60;
        draw_digits(ctx, tick_hour, text_bbox);
      } else {
        draw_digits(ctx, tick_minute, text_bbox);
      }
    }

    if (tick_minute % 15 == 0) {
      graphics_context_set_stroke_width(ctx, 3);
      tick_length = radius / 5;
    } else if (tick_minute % 5 == 0) {
      graphics_context_set_stroke_width(ctx, 1);
      tick_length = radius / 5;
    } else {
      //graphics_draw_pixel(ctx, minute_tick_outer);
      continue;
    }
    GPoint minute_tick_inner = cartesian_from_polar(center, radius - tick_length, tick_deg);
    graphics_draw_line(ctx, minute_tick_inner, minute_tick_outer);
  }
}

static void update_layer(Layer* layer, GContext* ctx) {
  time_t temp = time(NULL);
  struct tm* now = localtime(&temp);
  if (DEBUG_TIME) {
    fast_forward_time(now);
  }

  int hex_boundary_stroke_width = 5;
  GRect bounds = layer_get_bounds(layer);
  int H = bounds.size.h * 4 / 7;
  int W = hex_width(H);
  GPoint visible_centers[10];
  tessellate(ctx, bounds, hex_boundary_stroke_width, H, visible_centers);
  int text_bbox_height = 10;
  int radius = W / 2 - text_bbox_height / 2;
  GPoint hours_center = visible_centers[0];
  draw_ticks(ctx, hours_center, radius, text_bbox_height, true);
  int minutes_per_hour_hand_rev = 60 * 12;
  int minutes_elapsed = now->tm_hour * 60 + now->tm_min;
  int hour_angle_degrees = 360 * minutes_elapsed / minutes_per_hour_hand_rev;
  draw_hand(ctx, hours_center, hour_angle_degrees % 360, 8, radius);

  GPoint minutes_center = visible_centers[1];
  draw_ticks(ctx, minutes_center, radius, text_bbox_height, false);
  int seconds_per_minute_hand_rev = 60 * 60;
  int seconds_elapsed = now->tm_min * 60 + now->tm_sec;
  int min_angle_degrees = 360 * seconds_elapsed / seconds_per_minute_hand_rev;
  draw_hand(ctx, minutes_center, min_angle_degrees % 360, 8, radius);

}

static void window_load(Window* window) {
  Layer* window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(s_window, GColorChromeYellow);
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
