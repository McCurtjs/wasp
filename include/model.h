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

int  model_build(Model* model);
void model_bind(const Model* model);
void model_render(const Model* model);
void model_render_instanced(const Model* model, index_t count);

void model_load_obj(Model* model, File file);

void model_grid_set_default(Model* model, int extent);
void model_sprites_draw(
  const Model_Sprites* spr, vec2 pos, vec2 scale, index_t frame, bool mirror);

#endif
