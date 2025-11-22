#include "draw.h"

#include "gl.h"

typedef struct Vert {
  vec3    pos;
  color4  color;
} Vert;

#define con_type Vert
#define con_prefix vert
#include "span.h"
#include "array.h"
#undef con_type
#undef con_prefix

DebugDrawState draw = {
  .color = sc4black,
  .color_gradient_start = sc4black,
  .use_gradient = FALSE,
  .vector_offset = svNzero,
  .scale = 0.1f
};

#define con_type DebugDrawState
#define con_prefix dstate
#include "span.h"
#include "array.h"
#undef con_type
#undef con_prefix


static Array_vert geometry = NULL;
static Array_dstate draw_state_stack = NULL;
static uint gl_vao = 0;
static uint gl_buffer = 0;

static void draw_init() {
  if (geometry) return;
  draw_default_state();
  geometry = arr_vert_new_reserve(128);
}

static void draw_add_point(vec3 pos, color4 col) {
  draw_init();

  arr_vert_push_back(geometry, (Vert){pos, col});
}

static bool draw_init_gl() {
  if (!geometry) return FALSE;
  if (gl_buffer) return TRUE;

  const void* color_offset = (void*)sizeof(vec3);

  glGenVertexArrays(1, &gl_vao);
  glBindVertexArray(gl_vao);
  glGenBuffers(1, &gl_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, gl_buffer);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vert), 0);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vert), color_offset);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  return TRUE;
}

// State functions

void draw_push() {
  if (!draw_state_stack) {
    draw_state_stack = arr_dstate_new();
  }
  arr_dstate_push_back(draw_state_stack, draw);
}

void draw_pop() {
  if (!draw_state_stack || draw_state_stack->size == 0) return;
  draw = arr_dstate_get_back(draw_state_stack);
  arr_dstate_pop_back(draw_state_stack);
}

void draw_default_state() {
  draw = (DebugDrawState){c4black, c4black, FALSE, v3zero, 0.1f};
}

// Draw functions

void draw_point(vec3 p) {
  vec3 a, b;
  float s = draw.scale;
  vec3 x = v3scale(v3x, s), y = v3scale(v3y, s), z = v3scale(v3z, s);
  a = v3sub(p, x); b = v3add(p, x);
  draw_line_solid(a, b, draw.color);
  a = v3sub(p, y); b = v3add(p, y);
  draw_line_solid(a, b, draw.color);
  a = v3sub(p, z); b = v3add(p, z);
  draw_line_solid(a, b, draw.color);
}

void draw_circle(vec3 center, float radius) {
  vec3 prev = v3scale(v3x, radius);

  for (uint i = 0; i < 32; ++i) {
    vec3 next = v23f(v2rot(prev.xy, TAU / 32), 0);
    draw_line_solid(v3add(center, prev), v3add(center, next), draw.color);
    prev = next;
  }
}

void draw_line_solid(vec3 a, vec3 b, color4 color_override) {
  draw_add_point(a, color_override);
  draw_add_point(b, color_override);
}

void draw_line(vec3 a, vec3 b) {
  if (draw.use_gradient) {
    draw_add_point(a, draw.color_gradient_start);
  } else {
    draw_add_point(a, draw.color);
  }

  draw_add_point(b, draw.color);
}

void draw_vector(vec3 v) {
  vec3 target = v3add(draw.vector_offset, v);
  draw_line(draw.vector_offset, target);
  vec3 base = v3add(draw.vector_offset, v3scale(v, 1.f - draw.scale));
  float base_scale = v3mag(v) * draw.scale * 0.75f;
  vec3 perp = v3scale(v3norm(v3perp(v)), base_scale);
  vec3 pcrs = v3scale(v3norm(v3cross(v, perp)), base_scale);
  draw_line_solid(target, v3add(base, perp), draw.color);
  draw_line_solid(base,   v3add(base, perp), draw.color);
  draw_line_solid(target, v3sub(base, perp), draw.color);
  draw_line_solid(base,   v3sub(base, perp), draw.color);
  draw_line_solid(target, v3add(base, pcrs), draw.color);
  draw_line_solid(base,   v3add(base, pcrs), draw.color);
  draw_line_solid(target, v3sub(base, pcrs), draw.color);
  draw_line_solid(base,   v3sub(base, pcrs), draw.color);

  draw_line_solid(v3add(base, pcrs), v3add(base, perp), draw.color);
  draw_line_solid(v3sub(base, pcrs), v3sub(base, perp), draw.color);
  draw_line_solid(v3sub(base, pcrs), v3add(base, perp), draw.color);
  draw_line_solid(v3add(base, pcrs), v3sub(base, perp), draw.color);
}

void draw_dir(vec3 v) {
  draw_vector(v3norm(v));
}

void draw_rect(vec3 pos, vec3 a, vec3 b) {
  vec3 end = v3add(v3add(pos, a), b);
  draw_line(pos, v3add(pos, a));
  draw_line_solid(pos, v3add(pos, b),
    draw.use_gradient ? draw.color_gradient_start : draw.color);
  draw_line(v3add(pos, b), end);
  draw_line_solid(v3add(pos, a), end, draw.color);
}

// Rendering and cleanup

void draw_render() {
  if (!draw_init_gl()) return;

  int size_bytes = (int)geometry->size_bytes;
  int size_lines = (int)geometry->size;
  Vert* draw_buffer = geometry->begin;

  glBindVertexArray(gl_vao);
  glBindBuffer(GL_ARRAY_BUFFER, gl_buffer);
  glBufferData(GL_ARRAY_BUFFER, size_bytes, draw_buffer, GL_STATIC_DRAW);

  glDrawArrays(GL_LINES, 0, size_lines);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  draw_default_state();
  arr_vert_clear(geometry);
}

void draw_cleanup() {
  arr_vert_delete(&geometry);
  glDeleteBuffers(1, &gl_buffer);
  glDeleteVertexArrays(1, &gl_vao);
  gl_buffer = 0; gl_vao = 0;
}
