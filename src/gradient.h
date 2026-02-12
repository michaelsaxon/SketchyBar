#pragma once
#include "misc/helpers.h"
#include "color.h"

struct gradient_stop {
  float position;
  struct color color;
};

struct gradient {
  bool enabled;

  uint32_t angle;
  uint32_t type;
  float radius_h;
  float radius_v;

  // Legacy 2-color API
  struct color color_start;
  struct color color_end;

  // Power user arbitrary stops
  struct gradient_stop* stops;
  uint32_t stops_count;
  uint32_t stops_capacity;
};

void gradient_init(struct gradient* gradient);
void gradient_destroy(struct gradient* gradient);
void gradient_draw(struct gradient* gradient, CGContextRef context, CGRect region, uint32_t corner_radius);

void gradient_serialize(struct gradient* gradient, char* indent, FILE* rsp);
bool gradient_parse_sub_domain(struct gradient* gradient, FILE* rsp, struct token property, char* message);
