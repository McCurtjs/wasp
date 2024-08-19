#ifndef _WASP_DEBUG_DRAW_H_
#define _WASP_DEBUG_DRAW_H_

#include "types.h"
#include "vec.h"

typedef struct DebugDrawState {
  color4 color;
  color4 color_gradient_start;
  bool   use_gradient;
  vec3   vector_offset;
  float  scale;
} DebugDrawState;
extern DebugDrawState draw;

void draw_push();
void draw_pop();
void draw_default_state();

void draw_point(vec3 p);
void draw_circle(vec3 p, float radius);
void draw_line(vec3 a, vec3 b);
void draw_line_solid(vec3 a, vec3 b, color4 color);
void draw_vector(vec3 v);
void draw_dir(vec3 v);
void draw_rect(vec3 pos, vec3 a, vec3 b);

void draw_render();

void draw_cleanup();

#endif
