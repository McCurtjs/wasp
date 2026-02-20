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
#include "vertex.h"

typedef enum model_type_t {
  MODEL_NONE = 0,
  MODEL_GRID,
  MODEL_CUBE,
  MODEL_CUBE_COLOR,
  MODEL_FRAME,
  MODEL_SPRITES,
  MODEL_MESH,
  MODEL_TYPES_COUNT
} model_type_t;

// TODO: Add MODEL_EDITABLE for meshes that keep stored data in memory to be
//    updated on the fly (with glBufferSubData). Other model types should be
//    able to be converted into the Editable type based on their parameters.
//    Mostly, this would probably be useful for primitives. Editable should also
//    be convertable back into a Mesh (static draw, no stored data CPU side).

// TODO: Remove binding code from the models and instead add a vertex 
//    descriptor type that describes the format of the verts in a way that can
//    be used to do the actual binding separately. Then models can be completely
//    backend agnostic for binding (rendering maybe not?) and focus on just
//    handling the geometry. The descriptor (vertex_desc_t?) could contain the
//    actual data about the verts, or just be an id that maps to one of the
//    actual vertex type structs. Or could just be statically defined next to
//    the structs? Possibly do the same for instance data layouts?

// A grid is defined by the basis axes provided, along the index primary axes
// if extent is 0, render only a unit axis gizmo
// use a negative extent to scale the gizmo by the absolute value
typedef struct model_grid_t {
  const int extent;
  const vec3 basis[3];
  const byte primary[2];
} model_grid_t;

typedef struct model_grid_param_t {
  int extent;
  vec3 basis[3];
  byte primary[2];
} model_grid_param_t;

typedef struct model_box_t {
  const vec3 dim;
  const vec3i subdivs;
} model_box_t;

typedef struct model_sprites_t {
  vec2i grid;
} model_sprites_t;

typedef struct _opaque_Model_t {
  const model_type_t type;
  const vert_format_t format;
  const index_t vert_count;
  const index_t index_count;
  const bool ready;

  union {
    model_grid_t grid;
    model_sprites_t sprites;
  };
}* Model;

Model model_new_primitive(model_type_t type);
Model model_new_sprites(vec2i dim);
Model model_new_grid(model_grid_param_t grid);
Model model_new_grid_default(int extent);
Model model_new_from_obj(File file);

void  model_delete(Model* model);

void model_render(Model model);
void model_bind(const Model model);
void model_render_instanced(const Model model, index_t count);

void model_sprites_add(
  Model spr, vec2 pos, vec2 scale, index_t frame, bool mirror);

#endif
