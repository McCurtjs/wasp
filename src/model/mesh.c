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

#define MCLIB_INTERNAL_IMPL
#include "model.h"

#include "gl.h"

#include <stdlib.h>

extern Array _new_models;

////////////////////////////////////////////////////////////////////////////////
// Models loaded from files
////////////////////////////////////////////////////////////////////////////////

#include "../loaders/obj.h"

typedef struct Model_Internal_Mesh {
  MODEL_PROPS;

  // Secrets

  String name_internal;
  File file;

  union {
    GLuint buffers[2];
    struct {
      GLuint vbo; // buffer for verts
      GLuint ebo; // buffer for indices/"elements"
    };
  };
  GLuint vao;

} Model_Internal_Mesh;

////////////////////////////////////////////////////////////////////////////////

static void _model_build_from_string(Model model, slice_t text) {
  Model_Internal_Mesh* mesh = (Model_Internal_Mesh*)model;
  assert(mesh);
  assert(mesh->type == MODEL_MESH);

  if (mesh->status != S_BUILDING) return;

  model_obj_t obj = file_load_obj(text);

  if (!obj.verts || !obj.verts->size) {
    assert(0);
    mesh->status = S_ERROR;
    goto new_obj_cleanup;
  }

  if (obj.name == NULL) {
    if (mesh->file)
      obj.name = str_copy(str_between_last(mesh->file->name, "/", "."));
    else
      obj.name = str_copy("OBJ_Model");
  }

  mesh->status = S_READY;
  mesh->format = obj.format;
  mesh->vert_count = obj.verts->size;
  mesh->index_count = obj.indices->size;
  mesh->name_internal = obj.name;
  mesh->name = obj.name->slice;

  glGenBuffers(2, mesh->buffers);

  glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
  glBufferData(GL_ARRAY_BUFFER
  , obj.verts->size_bytes
  , obj.verts->begin
  , GL_STATIC_DRAW
  );

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER
  , obj.indices->size_bytes
  , obj.indices->begin
  , GL_STATIC_DRAW
  );

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

new_obj_cleanup:

  arr_delete(&obj.verts);
  arr_delete(&obj.indices);
}

////////////////////////////////////////////////////////////////////////////////

void _model_build_mesh(Model model) {
  Model_Internal_Mesh* mesh = (Model_Internal_Mesh*)model;
  assert(mesh);
  assert(mesh->type == MODEL_MESH);

  if (mesh->status == S_READY) return;
  assert(mesh->file);

  if (mesh->status == S_LOADING && mesh->file->status == S_READY) {
    mesh->status = S_BUILDING;
  }

  str_log("[Model.load] Building model from file: {}", mesh->file->name);

  _model_build_from_string(model, mesh->file->slice);
}

////////////////////////////////////////////////////////////////////////////////
// Load from OBJ
////////////////////////////////////////////////////////////////////////////////

Model model_new_load_obj(slice_t filename) {
  assert(!slice_is_empty(filename));

  Model_Internal_Mesh* model = malloc(sizeof(*model));
  assert(model);

  str_log("[Model.new] Loading from file: {}", filename);

  *model = (Model_Internal_Mesh) {
  .type = MODEL_MESH,
  .status = S_LOADING,
  .file = file_new(filename, FM_READ),
  };

  arr_insert_back(_new_models, &model);

  return (Model)model;
}

////////////////////////////////////////////////////////////////////////////////

Model model_new_from_obj(slice_t obj_text) {
  assert(slice_is_valid(obj_text));

  Model_Internal_Mesh* model = malloc(sizeof(*model));
  assert(model);

  str_write("[Model.new] Loading from text...");

  *model = (Model_Internal_Mesh){
  .type = MODEL_MESH,
  .status = S_BUILDING,
  };

  arr_insert_back(_new_models, &model);

  _model_build_from_string((Model)model, obj_text);

  str_log("[Model.new] Loaded model: {}", model->name);

  return (Model)model;
}

////////////////////////////////////////////////////////////////////////////////
// Binding and rendering
////////////////////////////////////////////////////////////////////////////////

void _model_bind_mesh(const Model model) {
  Model_Internal_Mesh* mesh = (Model_Internal_Mesh*)model;
  assert(mesh->type == MODEL_MESH);
  assert(mesh->status == S_READY);
  assert(mesh->vbo);
  assert(mesh->ebo);

  glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
  vertex_bind(model->format);
}

////////////////////////////////////////////////////////////////////////////////

void _model_render_mesh(Model model) {
  Model_Internal_Mesh* mesh = (Model_Internal_Mesh*)model;
  assert(mesh->type == MODEL_MESH);
  assert(mesh->status == S_READY);
  assert(mesh->index_count);
  assert(mesh->vbo);
  assert(mesh->ebo);

  if (!mesh->vao) {
    glGenVertexArrays(1, &mesh->vao);
    glBindVertexArray(mesh->vao);
    model_bind((Model)mesh);
  }
  else {
    glBindVertexArray(mesh->vao);
  }

  glDrawElements
  ( GL_TRIANGLES
  , (GLsizei)mesh->index_count
  , GL_UNSIGNED_INT
  , 0
  );

  glBindVertexArray(0);
}
