#pragma once
#include "misc/helpers.h"
#include "color.h"

struct gradient {
  bool enabled;

  uint32_t angle;

  struct color color_start;
  struct color color_end;
};

void gradient_init(struct gradient* gradient);
void gradient_draw(struct gradient* gradient, CGContextRef context, CGRect region, uint32_t corner_radius);

void gradient_serialize(struct gradient* gradient, char* indent, FILE* rsp);
bool gradient_parse_sub_domain(struct gradient* gradient, FILE* rsp, struct token property, char* message);
