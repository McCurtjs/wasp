/*******************************************************************************
* MIT License
*
* Copyright (c) 2025 Curtis McCoy
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#ifndef WASP_MODEL_H_
#define WASP_MODEL_H_

#include "types.h"
#include "mat.h"
#include "array.h"
#include "file.h"
#include "texture.h"

typedef enum model_type_t {
  MODEL_NONE = 0,
  MODEL_GRID,
  MODEL_CUBE,
  MODEL_CUBE_COLOR,
  MODEL_FRAME,
  MODEL_SPRITES,
  MODEL_OBJ,
  MODEL_TYPES_COUNT
} model_type_t;

// A grid defined by the basis axes provided, along the index primary axes
// if extent is 0, render only a unit axis gizmo
// use a negative extent to scale the gizmo by the absolute value
typedef struct Model_Grid {
  model_type_t type;
  index_t vert_count;
  index_t index_count;
  bool ready;
  int  extent;
  vec3 basis[3];
  byte primary[2];
  uint vao;
  uint buffers[2];
} Model_Grid;

typedef struct Model_Cube {
  model_type_t type;
  index_t vert_count;
  index_t index_count;
  bool ready;
} Model_Cube;

typedef struct Model_CubeColor {
  model_type_t type;
  index_t vert_count;
  index_t index_count;
  bool ready;
} Model_CubeColor;

typedef struct Model_Frame {
  model_type_t type;
  index_t vert_count;
  index_t index_count;
  bool ready;
} Model_Frame;

typedef struct Model_Sprites {
  model_type_t type;
  index_t vert_count;
  index_t index_count;
  bool ready;
  vec2i grid;
  uint vao;
  uint buffer;
  Array verts;
} Model_Sprites;

typedef struct Model_Mesh {
  model_type_t type;
  index_t vert_count;
  index_t index_count;
  bool ready;
  bool use_color;
  Array verts;
  Array indices;
  uint vao;
  union {
    uint buffers[2];
    struct {
      uint vert_buffer;
      uint ebo;
    };
  };
} Model_Mesh;

typedef union Model {
  struct {
    model_type_t type;
    index_t vert_count;
    index_t index_count;
    bool ready;
  };
  Model_Grid grid;
  Model_Cube cube;
  Model_CubeColor cube_color;
  Model_Frame frame;
  Model_Sprites sprites;
  Model_Mesh mesh;
} Model;

typedef struct model_grid_t {
  const int extent;
  const vec3 basis[3];
  const byte primary[2];
} model_grid_t;

typedef struct model_box_t {
  const vec3 dim;
  const vec3i subdivs;
} model_box_t;

typedef struct model_mesh_t {
  const bool has_vertex_color;
} model_mesh_t;

typedef struct _opaque_Model_base {
  const model_type_t type;
  const index_t vert_count;
  const index_t index_count;
  const bool ready;

  union {
    model_grid_t grid;
    model_mesh_t mesh;
  };
}* Model2;

Model2  model_new_primitive(model_type_t type);
Model2  model_new_cube_color(void);
Model2  model_new_frame(void);
Model2  model_new_sprites(texture_t sheet, vec2i dim);
//Model2 model_new_grid(static vec3 basis[3], static byte primary[2]);
Model2  model_new_from_obj(slice_t file);

void    model2_bind(Model2 model);
void    model2_render_instanced(Model2 model, index_t count);

void    model_delete(Model2* model);

int  model_build(Model* model);
void model_bind(const Model* model);
void model_render(const Model* model);
void model_render_instanced(const Model* model, index_t count);

void model_load_obj(Model* model, File file);

void model_grid_set_default(Model* model, int extent);
void model_sprites_draw(
  const Model_Sprites* spr, vec2 pos, vec2 scale, index_t frame, bool mirror);

#endif
