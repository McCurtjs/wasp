#ifndef WASP_LOADER_OBJ_H_
#define WASP_LOADER_OBJ_H_

#include "file.h"
#include "array.h"
#include "model.h"

typedef struct {
  vec3 pos;
  vec3 norm;
  vec2 uv;
  vec4 tangent;
} obj_vertex_t;

typedef struct {
  vec3 pos;
  vec3 norm;
  vec2 uv;
  vec4 tangent;
  vec3 color;
} obj_vertex_color_t;

typedef struct model_obj_t {
  Array verts;
  Array indices;
  bool has_vertex_color;
} model_obj_t;

model_obj_t file_load_obj(File file);

#endif
