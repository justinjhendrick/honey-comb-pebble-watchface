#include <pebble.h>

static void draw_one_digit(GContext* ctx, int digit, GRect bbox) {
  int x = bbox.origin.x + 1;
  int y = bbox.origin.y;
  int w = bbox.size.w - 2;
  int h = bbox.size.h;

  GPoint tl = GPoint(x,         y);
  GPoint tc = GPoint(x + w / 2, y);
  GPoint tr = GPoint(x + w,     y);

  GPoint ml = GPoint(x,         y + h / 2);
  GPoint mc = GPoint(x + w / 2, y + h / 2);
  GPoint mr = GPoint(x + w,     y + h / 2);

  GPoint bl = GPoint(x,         y + h);
  GPoint bc = GPoint(x + w / 2, y + h);
  GPoint br = GPoint(x + w,     y + h);
  if (digit == 0) {
    graphics_draw_line(ctx, tl, tr);
    graphics_draw_line(ctx, tr, br);
    graphics_draw_line(ctx, br, bl);
    graphics_draw_line(ctx, bl, tl);
  } else if (digit == 1) {
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

static void draw_one_letter(GContext* ctx, char letter, GRect bbox) {
  int x = bbox.origin.x + 1;
  int y = bbox.origin.y;
  int w = bbox.size.w - 2;
  int h = bbox.size.h;

  GPoint tl = GPoint(x,         y);
  GPoint tc = GPoint(x + w / 2, y);
  GPoint tr = GPoint(x + w,     y);

  GPoint ml = GPoint(x,         y + h / 2);
  GPoint mc = GPoint(x + w / 2, y + h / 2);
  GPoint mr = GPoint(x + w,     y + h / 2);

  GPoint bl = GPoint(x,         y + h);
  GPoint bc = GPoint(x + w / 2, y + h);
  GPoint br = GPoint(x + w,     y + h);
  if (letter == 'S') {
    draw_one_digit(ctx, 5, bbox);
  } else if (letter == 'M') {
    graphics_draw_line(ctx, bl, tl);
    graphics_draw_line(ctx, tl, mc);
    graphics_draw_line(ctx, mc, tr);
    graphics_draw_line(ctx, tr, br);
  } else if (letter == 'T') {
    graphics_draw_line(ctx, tl, tr);
    graphics_draw_line(ctx, tc, bc);
  } else if (letter == 'W') {
    graphics_draw_line(ctx, tl, bl);
    graphics_draw_line(ctx, bl, mc);
    graphics_draw_line(ctx, mc, br);
    graphics_draw_line(ctx, br, tr);
  } else if (letter == 'F') {
    graphics_draw_line(ctx, tl, bl);
    graphics_draw_line(ctx, tl, tr);
    graphics_draw_line(ctx, ml, mr);
  }
}