#include "gradient.h"
#include "bar_manager.h"
#include "animation.h"

void gradient_init(struct gradient* gradient) {
  gradient->enabled = false;
  gradient->angle = 0;
  color_init(&gradient->color_start, 0x00000000);
  color_init(&gradient->color_end, 0x00000000);
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

static bool gradient_set_color_start(struct gradient* gradient, uint32_t color) {
  bool changed = gradient_set_enabled(gradient, true);
  return color_set_hex(&gradient->color_start, color) || changed;
}

static bool gradient_set_color_end(struct gradient* gradient, uint32_t color) {
  bool changed = gradient_set_enabled(gradient, true);
  return color_set_hex(&gradient->color_end, color) || changed;
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

void gradient_draw(struct gradient* gradient, CGContextRef context, CGRect region, uint32_t corner_radius) {
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

  CGFloat components[8] = {
    gradient->color_start.r, gradient->color_start.g,
    gradient->color_start.b, gradient->color_start.a,
    gradient->color_end.r,   gradient->color_end.g,
    gradient->color_end.b,   gradient->color_end.a
  };

  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
  CGGradientRef cg_gradient = CGGradientCreateWithColorComponents(
      color_space, components, NULL, 2);
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

void gradient_serialize(struct gradient* gradient, char* indent, FILE* rsp) {
  fprintf(rsp, "%s\"drawing\": \"%s\",\n"
               "%s\"angle\": %u,\n"
               "%s\"color_start\": \"0x%x\",\n"
               "%s\"color_end\": \"0x%x\"",
               indent, format_bool(gradient->enabled),
               indent, gradient->angle,
               indent, gradient->color_start.hex,
               indent, gradient->color_end.hex);
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
      else {
        respond(rsp, "[!] Gradient: Invalid subdomain '%s'\n", subdom.text);
      }
    } else {
      respond(rsp, "[!] Gradient: Invalid property '%s'\n", property.text);
    }
  }

  return needs_refresh;
}
