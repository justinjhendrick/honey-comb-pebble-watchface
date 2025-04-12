#include <pebble.h>

#define DEBUG_TIME (false)
#define BUFFER_LEN (100)

static Window* s_window;
static Layer* s_layer;
static char s_buffer[BUFFER_LEN];
static GPath* s_hexagon;

static const GPathInfo HEXAGON_POINTS = {
  .num_points = 6,
  // points will be filled by change_arrow_size
  .points = (GPoint []) {
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
    {0, 0},
  }
};

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

static void draw_hexagon(GContext* ctx, GPoint bottom_point, int H) {
  fill_hexagon_points(H);
  graphics_context_set_stroke_width(ctx, 5);
  graphics_context_set_fill_color(ctx, GColorChromeYellow);
  graphics_context_set_stroke_color(ctx, GColorYellow);
  gpath_move_to(s_hexagon, bottom_point);
  gpath_draw_filled(ctx, s_hexagon);
  gpath_draw_outline(ctx, s_hexagon);
}

static void update_layer(Layer* layer, GContext* ctx) {
  time_t temp = time(NULL);
  struct tm* now = localtime(&temp);
  if (DEBUG_TIME) {
    fast_forward_time(now);
  }

  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  int W = bounds.size.w / 2;
  int H = hex_height(W);
  int hsl = hex_half_side_length(H);
  GPoint left = GPoint(center.x - W / 2, H / 2);
  GPoint right = GPoint(center.x + W / 2, H / 2);
  GPoint bot = GPoint(center.x, right.y + H / 2 + hsl);
  draw_hexagon(ctx, left, H);
  draw_hexagon(ctx, right, H);
  draw_hexagon(ctx, bot, H);
}

static void window_load(Window* window) {
  Layer* window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(s_window, GColorBlack);
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

//static void draw_color_swatch_grid(GContext* ctx, GRect bounds) {
//  int w = bounds.size.w / 8;
//  int h = bounds.size.h / 8;
//  GColor color;
//  color.argb = 0b11000000;
//
//  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
//  memset(s_buffer, 0, BUFFER_LEN);
//  graphics_context_set_stroke_width(ctx, 1);
//  for (int y = 0; y < bounds.size.h; y += h) {
//    for (int x = 0; x < bounds.size.w; x += w) {
//      graphics_context_set_fill_color(ctx, color);
//      graphics_context_set_stroke_color(ctx, GColorBlack);
//      GRect bbox = GRect(x, y, w, h);
//      graphics_fill_rect(ctx, bbox, 0, GCornerNone);
//      graphics_draw_rect(ctx, bbox);
//      snprintf(s_buffer, BUFFER_LEN, "%d", color.argb & 0b00111111);
//      graphics_draw_text(ctx, s_buffer, font, bbox, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
//      color.argb += 1;
//    }
//  }
//}
