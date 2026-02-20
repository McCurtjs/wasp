/*******************************************************************************
* MIT License
*
* Copyright (c) 2026 Curtis McCoy
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

#include "model.h"

#include "gl.h"

////////////////////////////////////////////////////////////////////////////////

static void _model_render_instanced(const Model model, index_t count) {
  UNUSED(model);
  UNUSED(count);

  if (model->index_count) {
    // indexed draw
    glDrawElementsInstanced
    ( GL_TRIANGLES
    , (GLsizei)model->vert_count
    , GL_UNSIGNED_INT
    , 0
    , (GLsizei)count
    );
  }
  else {
    // non-indexed draw
    glDrawArraysInstanced
    ( GL_TRIANGLES
    , 0
    , (GLsizei)model->vert_count
    , (GLsizei)count
    );
  }
}

////////////////////////////////////////////////////////////////////////////////
// Function routing based on object types
////////////////////////////////////////////////////////////////////////////////

typedef void (model_bind_fn_t)(const Model model);
typedef void (model_render_fn_t)(Model model);
typedef void (model_render_inst_fn_t)(const Model model, index_t count);

// Internal model binding and render functions defined in ./models directory
extern model_bind_fn_t    _model_bind_primitive;
extern model_bind_fn_t    _model_bind_sprites;
extern model_bind_fn_t    _model_bind_mesh;
extern model_render_fn_t  _model_render_grid;
extern model_render_fn_t  _model_render_prim;
extern model_render_fn_t  _model_render_prim_strip;
extern model_render_fn_t  _model_render_sprites;
extern model_render_fn_t  _model_render_mesh;

typedef struct model_management_fns_t {
  model_bind_fn_t*        bind;
  model_render_fn_t*      render_single;
  model_render_inst_fn_t* render_inst;
} model_management_fns_t;

static model_management_fns_t model_management_fns[MODEL_TYPES_COUNT] = {
  // MODEL_NONE - No model type selected
  { 0 },

  // MODEL_GRID - Grid rendering
  { .render_single  = _model_render_grid,
  },

  // MODEL_CUBE - Basic cube primitive (with normals and tangents)
  { .bind           = _model_bind_primitive
  , .render_single  = _model_render_prim
  , .render_inst    = _model_render_instanced
  },

  // MODEL_CUBE_COLOR - Debug-cube object with vertex color, no normals
  { .bind           = _model_bind_primitive
  , .render_single  = _model_render_prim_strip
  },

  // MODEL_FRAME - Full-screen frame model for deferred rendering
  { .bind           = _model_bind_primitive
  , .render_single  = _model_render_prim
  },

  // MODEL_SPRITES - Accumulated collection of sprites
  { .bind           = _model_bind_sprites
  , .render_single  = _model_render_sprites
  },

  // MODEL_MESH - Basic mesh type loaded from .obj file
  { .bind           = _model_bind_mesh
  , .render_single  = _model_render_mesh
  , .render_inst    = _model_render_instanced
  }
};

////////////////////////////////////////////////////////////////////////////////

void model_bind(const Model model) {
  if (!model) {
    str_write("[Model.bind] Model is null");
    return;
  }

  int model_type = model->type;
  if (model_type < 0 || model_type >= MODEL_TYPES_COUNT) {
    str_log("[Model.bind] Invalid model type: {}", model_type);
    return;
  }

  if (!model->ready) {
    str_write("[Model.bind] Model not flagged as ready");
    return;
  }

  model_bind_fn_t* bind_fn = model_management_fns[model_type].bind;

  if (!bind_fn) {
    str_log("[Model.bind] No binding function for model type: ", model_type);
    return;
  }

  bind_fn(model);
}

////////////////////////////////////////////////////////////////////////////////

void model_render(Model model) {
  if (!model) {
    str_write("[Model.render] Model is null");
    return;
  }

  int model_type = model->type;
  if (model_type <= 0 || model_type >= MODEL_TYPES_COUNT) {
    str_log("[Model.render] Invalid model type: {}", model_type);
    return;
  }

  if (!model->ready) {
    str_write("[Model.render] Model not flagged as ready");
    return;
  }

  model_render_fn_t* render_fn = model_management_fns[model_type].render_single;

  if (!render_fn) {
    str_log("[Model.render] No basic renderer for type: {}", model_type);
    return;
  }

  render_fn(model);
}

////////////////////////////////////////////////////////////////////////////////

void model_render_instanced(const Model model, index_t count) {
  if (!model) {
    str_write("[Model.render_inst] Model is null");
    return;
  }

  if (count <= 0) {
    str_log("[Model.render_inst] Non-positive instance count: {}", count);
    return;
  }

  int model_type = model->type;
  if (model_type <= 0 || model_type >= MODEL_TYPES_COUNT) {
    str_log("[Model.render_inst] Invalid model type: {}", model_type);
    return;
  }

  if (!model->ready) {
    str_write("[Model.render_inst] Model not flagged as ready");
    return;
  }

  model_render_inst_fn_t* ins_fn = model_management_fns[model_type].render_inst;

  if (!ins_fn) {
    str_log
    ( "[Model.render_inst] No instanced renderer for type: {}"
    , model_type
    );
    return;
  }

  ins_fn(model, count);
}
