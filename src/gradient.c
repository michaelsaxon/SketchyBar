#include "gradient.h"
#include "bar_manager.h"
#include "animation.h"

void gradient_init(struct gradient* gradient) {
  gradient->enabled = false;
  gradient->angle = 0;
  gradient->type = 0;  // 0 = linear (default), 1 = radial
  gradient->radius_h = 0.0f;  // 0 = auto (diagonal)
  gradient->radius_v = 0.0f;  // 0 = auto (diagonal)
  color_init(&gradient->color_start, 0x00000000);
  color_init(&gradient->color_end, 0x00000000);
  gradient->stops = NULL;
  gradient->stops_count = 0;
  gradient->stops_capacity = 0;
}

void gradient_destroy(struct gradient* gradient) {
  if (gradient->stops) {
    free(gradient->stops);
    gradient->stops = NULL;
    gradient->stops_count = 0;
    gradient->stops_capacity = 0;
  }
}

static bool gradient_set_enabled(struct gradient* gradient, bool enabled) {
  if (gradient->enabled == enabled) return false;
  gradient->enabled = enabled;
  return true;
}

static bool gradient_set_angle(struct gradient* gradient, uint32_t angle) {
  if (gradient->angle == angle) return false;
  gradient->angle = angle;
  return true;
}

static bool gradient_set_type(struct gradient* gradient, uint32_t type) {
  if (gradient->type == type) return false;
  gradient->type = type;
  return true;
}

static bool gradient_set_radius_h(struct gradient* gradient, float radius) {
  if (gradient->radius_h == radius) return false;
  gradient->radius_h = radius;
  return true;
}

static bool gradient_set_radius_v(struct gradient* gradient, float radius) {
  if (gradient->radius_v == radius) return false;
  gradient->radius_v = radius;
  return true;
}

static bool gradient_set_color_start(struct gradient* gradient, uint32_t color) {
  bool changed = gradient_set_enabled(gradient, true);
  return color_set_hex(&gradient->color_start, color) || changed;
}

static bool gradient_set_color_end(struct gradient* gradient, uint32_t color) {
  bool changed = gradient_set_enabled(gradient, true);
  return color_set_hex(&gradient->color_end, color) || changed;
}

static void gradient_ensure_stop_capacity(struct gradient* gradient, uint32_t index) {
  // Ensure we have capacity for index+1 stops
  uint32_t required = index + 1;

  if (required > gradient->stops_capacity) {
    uint32_t new_capacity = gradient->stops_capacity == 0 ? 4 : gradient->stops_capacity * 2;
    while (new_capacity < required) new_capacity *= 2;

    gradient->stops = realloc(gradient->stops, new_capacity * sizeof(struct gradient_stop));
    gradient->stops_capacity = new_capacity;
  }

  // Initialize new stops if we're expanding count
  if (required > gradient->stops_count) {
    for (uint32_t i = gradient->stops_count; i < required; i++) {
      gradient->stops[i].position = 0.0f;
      color_init(&gradient->stops[i].color, 0x00000000);
    }
    gradient->stops_count = required;
  }
}

static bool gradient_set_stop_position(struct gradient* gradient, uint32_t index, float position) {
  gradient_ensure_stop_capacity(gradient, index);
  float clamped = position < 0.0f ? 0.0f : (position > 1.0f ? 1.0f : position);
  if (gradient->stops[index].position == clamped) return false;
  gradient->stops[index].position = clamped;
  gradient_set_enabled(gradient, true);
  return true;
}

static bool gradient_set_stop_color(struct gradient* gradient, uint32_t index, uint32_t color) {
  gradient_ensure_stop_capacity(gradient, index);
  bool changed = color_set_hex(&gradient->stops[index].color, color);
  gradient_set_enabled(gradient, true);
  return changed;
}

static void gradient_get_points(uint32_t angle, CGRect region, CGPoint* start, CGPoint* end) {
  double rad = (double)angle * deg_to_rad;
  double dx = cos(rad);
  double dy = -sin(rad);

  double cx = region.origin.x + region.size.width / 2.0;
  double cy = region.origin.y + region.size.height / 2.0;

  double hw = region.size.width / 2.0;
  double hh = region.size.height / 2.0;

  double extent = fabs(dx) * hw + fabs(dy) * hh;

  start->x = cx - dx * extent;
  start->y = cy - dy * extent;
  end->x   = cx + dx * extent;
  end->y   = cy + dy * extent;
}

static void gradient_draw_linear(struct gradient* gradient, CGContextRef context, CGRect region, uint32_t corner_radius) {
  CGContextSaveGState(context);

  CGMutablePathRef path = CGPathCreateMutable();
  if (corner_radius > region.size.height / 2.f || corner_radius > region.size.width / 2.f)
    corner_radius = region.size.height > region.size.width
                    ? region.size.width / 2.f
                    : region.size.height / 2.f;
  CGPathAddRoundedRect(path, NULL, region, corner_radius, corner_radius);
  CGContextAddPath(context, path);
  CGContextClip(context);
  CFRelease(path);

  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
  CGGradientRef cg_gradient;

  if (gradient->stops_count > 0) {
    // Power user: arbitrary stops
    CGFloat* components = malloc(gradient->stops_count * 4 * sizeof(CGFloat));
    CGFloat* locations = malloc(gradient->stops_count * sizeof(CGFloat));

    for (uint32_t i = 0; i < gradient->stops_count; i++) {
      components[i * 4 + 0] = gradient->stops[i].color.r;
      components[i * 4 + 1] = gradient->stops[i].color.g;
      components[i * 4 + 2] = gradient->stops[i].color.b;
      components[i * 4 + 3] = gradient->stops[i].color.a;
      locations[i] = gradient->stops[i].position;
    }

    cg_gradient = CGGradientCreateWithColorComponents(
        color_space, components, locations, gradient->stops_count);

    free(components);
    free(locations);
  } else {
    // Legacy: 2-color gradient
    CGFloat components[8] = {
      gradient->color_start.r, gradient->color_start.g,
      gradient->color_start.b, gradient->color_start.a,
      gradient->color_end.r,   gradient->color_end.g,
      gradient->color_end.b,   gradient->color_end.a
    };
    cg_gradient = CGGradientCreateWithColorComponents(
        color_space, components, NULL, 2);
  }
  CFRelease(color_space);

  CGPoint start_point, end_point;
  gradient_get_points(gradient->angle, region, &start_point, &end_point);

  CGContextDrawLinearGradient(context, cg_gradient,
                              start_point, end_point,
                              kCGGradientDrawsBeforeStartLocation
                              | kCGGradientDrawsAfterEndLocation);
  CFRelease(cg_gradient);

  CGContextRestoreGState(context);
}

static void gradient_draw_radial(struct gradient* gradient, CGContextRef context, CGRect region, uint32_t corner_radius) {
  CGContextSaveGState(context);

  CGMutablePathRef path = CGPathCreateMutable();
  if (corner_radius > region.size.height / 2.f || corner_radius > region.size.width / 2.f)
    corner_radius = region.size.height > region.size.width
                    ? region.size.width / 2.f
                    : region.size.height / 2.f;
  CGPathAddRoundedRect(path, NULL, region, corner_radius, corner_radius);
  CGContextAddPath(context, path);
  CGContextClip(context);
  CFRelease(path);

  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
  CGGradientRef cg_gradient;

  if (gradient->stops_count > 0) {
    // Power user: arbitrary stops
    CGFloat* components = malloc(gradient->stops_count * 4 * sizeof(CGFloat));
    CGFloat* locations = malloc(gradient->stops_count * sizeof(CGFloat));

    for (uint32_t i = 0; i < gradient->stops_count; i++) {
      components[i * 4 + 0] = gradient->stops[i].color.r;
      components[i * 4 + 1] = gradient->stops[i].color.g;
      components[i * 4 + 2] = gradient->stops[i].color.b;
      components[i * 4 + 3] = gradient->stops[i].color.a;
      locations[i] = gradient->stops[i].position;
    }

    cg_gradient = CGGradientCreateWithColorComponents(
        color_space, components, locations, gradient->stops_count);

    free(components);
    free(locations);
  } else {
    // Legacy: 2-color gradient
    CGFloat components[8] = {
      gradient->color_start.r, gradient->color_start.g,
      gradient->color_start.b, gradient->color_start.a,
      gradient->color_end.r,   gradient->color_end.g,
      gradient->color_end.b,   gradient->color_end.a
    };
    cg_gradient = CGGradientCreateWithColorComponents(
        color_space, components, NULL, 2);
  }
  CFRelease(color_space);

  CGPoint center = {
    region.origin.x + region.size.width / 2.0,
    region.origin.y + region.size.height / 2.0
  };

  // Calculate radii (0 = auto = diagonal)
  double radius_h = gradient->radius_h > 0
                    ? gradient->radius_h
                    : region.size.width / 2.0;
  double radius_v = gradient->radius_v > 0
                    ? gradient->radius_v
                    : region.size.height / 2.0;

  // For elliptical gradient, we need to scale the context
  bool is_elliptical = (radius_h != radius_v);
  if (is_elliptical) {
    // Use the larger radius as the base, scale the other dimension
    double max_radius = radius_h > radius_v ? radius_h : radius_v;
    double scale_x = radius_h / max_radius;
    double scale_y = radius_v / max_radius;

    CGContextTranslateCTM(context, center.x, center.y);
    CGContextScaleCTM(context, scale_x, scale_y);
    CGContextTranslateCTM(context, -center.x, -center.y);

    CGContextDrawRadialGradient(context, cg_gradient,
                                center, 0.0,
                                center, max_radius,
                                kCGGradientDrawsBeforeStartLocation
                                | kCGGradientDrawsAfterEndLocation);
  } else {
    // Circular gradient - use diagonal for auto
    double radius = radius_h > 0
                    ? radius_h
                    : sqrt(pow(region.size.width / 2.0, 2) +
                          pow(region.size.height / 2.0, 2));

    CGContextDrawRadialGradient(context, cg_gradient,
                                center, 0.0,
                                center, radius,
                                kCGGradientDrawsBeforeStartLocation
                                | kCGGradientDrawsAfterEndLocation);
  }

  CFRelease(cg_gradient);

  CGContextRestoreGState(context);
}

void gradient_draw(struct gradient* gradient, CGContextRef context, CGRect region, uint32_t corner_radius) {
  if (gradient->type == 1) {
    gradient_draw_radial(gradient, context, region, corner_radius);
  } else {
    gradient_draw_linear(gradient, context, region, corner_radius);
  }
}

void gradient_serialize(struct gradient* gradient, char* indent, FILE* rsp) {
  fprintf(rsp, "%s\"drawing\": \"%s\",\n"
               "%s\"type\": \"%s\",\n"
               "%s\"angle\": %u,\n"
               "%s\"radius_h\": %.2f,\n"
               "%s\"radius_v\": %.2f,\n"
               "%s\"color_start\": \"0x%x\",\n"
               "%s\"color_end\": \"0x%x\",\n"
               "%s\"stops_count\": %u",
               indent, format_bool(gradient->enabled),
               indent, gradient->type == 1 ? "radial" : "linear",
               indent, gradient->angle,
               indent, gradient->radius_h,
               indent, gradient->radius_v,
               indent, gradient->color_start.hex,
               indent, gradient->color_end.hex,
               indent, gradient->stops_count);

  if (gradient->stops_count > 0) {
    fprintf(rsp, ",\n%s\"stops\": [\n", indent);
    for (uint32_t i = 0; i < gradient->stops_count; i++) {
      fprintf(rsp, "%s  {\"position\": %.2f, \"color\": \"0x%x\"}",
              indent,
              gradient->stops[i].position,
              gradient->stops[i].color.hex);
      if (i < gradient->stops_count - 1) fprintf(rsp, ",\n");
    }
    fprintf(rsp, "\n%s]", indent);
  }
}

bool gradient_parse_sub_domain(struct gradient* gradient, FILE* rsp, struct token property, char* message) {
  bool needs_refresh = false;

  if (token_equals(property, PROPERTY_DRAWING)) {
    needs_refresh = gradient_set_enabled(gradient,
                      evaluate_boolean_state(get_token(&message),
                                             gradient->enabled));
  }
  else if (token_equals(property, PROPERTY_ANGLE)) {
    struct token token = get_token(&message);
    ANIMATE(gradient_set_angle,
            gradient,
            gradient->angle,
            token_to_int(token));
  }
  else if (token_equals(property, PROPERTY_GRADIENT_TYPE)) {
    struct token token = get_token(&message);
    uint32_t type = 0;
    if (token_equals(token, "radial")) {
      type = 1;
    } else if (token_equals(token, "linear")) {
      type = 0;
    } else {
      type = token_to_int(token);
    }
    ANIMATE(gradient_set_type,
            gradient,
            gradient->type,
            type);
  }
  else if (token_equals(property, PROPERTY_GRADIENT_RADIUS_H)) {
    struct token token = get_token(&message);
    ANIMATE_FLOAT(gradient_set_radius_h,
                  gradient,
                  gradient->radius_h,
                  token_to_float(token));
  }
  else if (token_equals(property, PROPERTY_GRADIENT_RADIUS_V)) {
    struct token token = get_token(&message);
    ANIMATE_FLOAT(gradient_set_radius_v,
                  gradient,
                  gradient->radius_v,
                  token_to_float(token));
  }
  else if (token_equals(property, PROPERTY_COLOR_START)) {
    struct token token = get_token(&message);
    ANIMATE_BYTES(gradient_set_color_start,
                  gradient,
                  gradient->color_start.hex,
                  token_to_int(token));
  }
  else if (token_equals(property, PROPERTY_COLOR_END)) {
    struct token token = get_token(&message);
    ANIMATE_BYTES(gradient_set_color_end,
                  gradient,
                  gradient->color_end.hex,
                  token_to_int(token));
  }
  else {
    struct key_value_pair key_value_pair = get_key_value_pair(property.text, '.');
    if (key_value_pair.key && key_value_pair.value) {
      struct token subdom = {key_value_pair.key, strlen(key_value_pair.key)};
      struct token entry = {key_value_pair.value, strlen(key_value_pair.value)};
      if (token_equals(subdom, SUB_DOMAIN_COLOR_START)) {
        return color_parse_sub_domain(&gradient->color_start, rsp, entry, message);
      }
      else if (token_equals(subdom, SUB_DOMAIN_COLOR_END)) {
        return color_parse_sub_domain(&gradient->color_end, rsp, entry, message);
      }
      else if (token_equals(subdom, "stops")) {
        // Parse stops.INDEX.PROPERTY or stops.INDEX.color.PROPERTY
        struct key_value_pair stop_pair = get_key_value_pair(entry.text, '.');
        if (stop_pair.key && stop_pair.value) {
          uint32_t index = atoi(stop_pair.key);
          struct token stop_property = {stop_pair.value, strlen(stop_pair.value)};

          // Check if it's a direct property or color sub-property
          struct key_value_pair color_pair = get_key_value_pair(stop_property.text, '.');
          if (color_pair.key && color_pair.value) {
            // stops.INDEX.color.PROPERTY
            struct token color_subdom = {color_pair.key, strlen(color_pair.key)};
            struct token color_entry = {color_pair.value, strlen(color_pair.value)};
            if (token_equals(color_subdom, "color")) {
              gradient_ensure_stop_capacity(gradient, index);
              return color_parse_sub_domain(&gradient->stops[index].color, rsp, color_entry, message);
            }
          } else {
            // stops.INDEX.PROPERTY (direct property)
            if (token_equals(stop_property, "position")) {
              struct token token = get_token(&message);
              needs_refresh = gradient_set_stop_position(gradient, index, token_to_float(token));
            }
            else if (token_equals(stop_property, "color")) {
              struct token token = get_token(&message);
              needs_refresh = gradient_set_stop_color(gradient, index, token_to_int(token));
            }
            else {
              respond(rsp, "[!] Gradient: Invalid stop property '%s'\n", stop_property.text);
            }
          }
        } else {
          respond(rsp, "[!] Gradient: Invalid stops syntax '%s'\n", entry.text);
        }
      }
      else {
        respond(rsp, "[!] Gradient: Invalid subdomain '%s'\n", subdom.text);
      }
    } else {
      respond(rsp, "[!] Gradient: Invalid property '%s'\n", property.text);
    }
  }

  return needs_refresh;
}
