#ifndef LOADER_OBJ
#define LOADER_OBJ

#include "file.h"
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

void file_load_obj(Model_Mesh* model, File file);

#endif
