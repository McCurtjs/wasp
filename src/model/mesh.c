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

#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////
// Models loaded from files
////////////////////////////////////////////////////////////////////////////////

#include "../loaders/obj.h"

typedef struct Model_Internal_Mesh {
  model_type_t type;
  index_t vert_count;
  index_t index_count;
  bool ready;

  union {
    model_mesh_t mesh;
    struct {
      bool use_color;
    };
  };

  // Secrets

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
// Load from OBJ
////////////////////////////////////////////////////////////////////////////////

Model model_new_from_obj(File file) {
  assert(file);
  if (!file_read(file)) {
    str_log("[Model.new_from_obj] Can't read file: {}", file->name);
    return NULL;
  }

  Model_Internal_Mesh* model = calloc(1, sizeof(Model_Internal_Mesh));
  assert(model);

  model->type = MODEL_MESH;
  model_obj_t obj = file_load_obj(file);
  model->use_color = obj.has_vertex_color;

  if (!obj.verts || !obj.verts->size) {
    assert(0);
    goto new_obj_cleanup;
  }

  model->vert_count = obj.verts->size;
  model->index_count = obj.indices->size;

  glGenBuffers(2, model->buffers);

  glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
  glBufferData(GL_ARRAY_BUFFER
  , obj.verts->size_bytes
  , obj.verts->begin
  , GL_STATIC_DRAW
  );

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER
  , obj.indices->size_bytes
  , obj.indices->begin
  , GL_STATIC_DRAW
  );

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  model->ready = true;

new_obj_cleanup:

  arr_delete(&obj.verts);
  arr_delete(&obj.indices);

  return (Model)model;
}

////////////////////////////////////////////////////////////////////////////////
// Binding and rendering
////////////////////////////////////////////////////////////////////////////////

void _model_bind_mesh(const Model model) {
  Model_Internal_Mesh* mesh = (Model_Internal_Mesh*)model;
  assert(mesh->type == MODEL_MESH);
  assert(mesh->ready);
  assert(mesh->vbo);
  assert(mesh->ebo);

  glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);

  GLsizei vert_size = mesh->use_color
    ? sizeof(obj_vertex_color_t)
    : sizeof(obj_vertex_t);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(
    0, v3floats, GL_FLOAT, GL_FALSE, vert_size, &((obj_vertex_t*)0)->pos
  );

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(
    1, v2floats, GL_FLOAT, GL_FALSE, vert_size, &((obj_vertex_t*)0)->uv
  );

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(
    2, v3floats, GL_FLOAT, GL_FALSE, vert_size, &((obj_vertex_t*)0)->norm
  );

  glEnableVertexAttribArray(3);
  glVertexAttribPointer(
    3, v4floats, GL_FLOAT, GL_FALSE, vert_size, &((obj_vertex_t*)0)->tangent
  );

  if (mesh->use_color) {
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, v3floats, GL_FLOAT, GL_FALSE, vert_size
    , &((obj_vertex_color_t*)0)->color
    );
  }
}

////////////////////////////////////////////////////////////////////////////////

void _model_render_mesh(Model model) {
  Model_Internal_Mesh* mesh = (Model_Internal_Mesh*)model;
  assert(mesh->type == MODEL_MESH);
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
