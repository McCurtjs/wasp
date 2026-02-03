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
#include <string.h> // memcpy

// inlined static data declarations for model primitives
#include "../data/inline_primitives.h"

////////////////////////////////////////////////////////////////////////////////
// Primitives
////////////////////////////////////////////////////////////////////////////////

typedef struct Model_Internal_Primitive {
  model_type_t type;
  index_t vert_count;
  index_t index_count;
  bool ready;

  // Hidden
  GLuint vbo;
  GLuint vao;
} Model_Internal_Primitive;

////////////////////////////////////////////////////////////////////////////////
// Basic 1x1x1 cube with UV, normals, and tangents
////////////////////////////////////////////////////////////////////////////////

static Model _model_new_cube(void) {
  Model_Internal_Primitive* model = malloc(sizeof(Model_Internal_Primitive));
  assert(model);

  GLsizeiptr size = sizeof(cube2_verts);
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, size, cube2_verts, GL_STATIC_DRAW);

  *model = (Model_Internal_Primitive) {
    .type = MODEL_CUBE,
    .vert_count = 36,
    .index_count = 0,
    .ready = true,
    .vbo = vbo,
    .vao = 0,
  };

  return (Model)model;
}

////////////////////////////////////////////////////////////////////////////////

void _model_bind_prim_cube(Model model) {
  Model_Internal_Primitive* cube = (Model_Internal_Primitive*)model;
  assert(cube->type == MODEL_CUBE);
  assert(cube->ready);
  assert(cube->vbo);

  GLsizei stride = 12 * sizeof(float);

  glBindBuffer(GL_ARRAY_BUFFER, cube->vbo);

  // position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, v3floats, GL_FLOAT, GL_FALSE, stride, 0);

  // uv
  glEnableVertexAttribArray(1);
  const void* offset = (void*)24;
  glVertexAttribPointer(1, v2floats, GL_FLOAT, GL_FALSE, stride, offset);

  // normal
  glEnableVertexAttribArray(2);
  offset = (void*)12;
  glVertexAttribPointer(2, v3floats, GL_FLOAT, GL_FALSE, stride, offset);

  // tangent
  glEnableVertexAttribArray(3);
  offset = (void*)32;
  glVertexAttribPointer(3, v4floats, GL_FLOAT, GL_FALSE, stride, offset);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

////////////////////////////////////////////////////////////////////////////////
// Vertex-color sample cube
////////////////////////////////////////////////////////////////////////////////

static Model _model_new_cube_color(void) {
  Model_Internal_Primitive* model = malloc(sizeof(Model_Internal_Primitive));
  assert(model);

  size_t size = sizeof(cube_pos) + sizeof(cube_color);
  byte* temp = malloc(size);
  assert(temp);

  memcpy(temp, cube_pos, sizeof(cube_pos));
  memcpy(temp + sizeof(cube_pos), cube_color, sizeof(cube_color));

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, size, temp, GL_STATIC_DRAW);

  free(temp);

  *model = (Model_Internal_Primitive) {
    .type = MODEL_CUBE_COLOR,
    .vert_count = 14,
    .index_count = 0,
    .ready = true,
    .vbo = vbo,
    .vao = 0,
  };

  return (Model)model;
}

////////////////////////////////////////////////////////////////////////////////

void _model_bind_prim_cube_color(Model model) {
  Model_Internal_Primitive* prim = (Model_Internal_Primitive*)model;
  assert(prim->type == MODEL_CUBE_COLOR);
  assert(prim->ready);
  assert(prim->vbo);

  glBindBuffer(GL_ARRAY_BUFFER, prim->vbo);

  // position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, v3floats, GL_FLOAT, GL_FALSE, v3bytes, 0);

  // vertex colors
  void* offset = (void*)sizeof(cube_pos);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, v4floats, GL_FLOAT, GL_FALSE, v4bytes, offset);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

////////////////////////////////////////////////////////////////////////////////
// Single-triangle for full-screen rendering/deferred
////////////////////////////////////////////////////////////////////////////////

static Model _model_new_frame(void) {
  Model_Internal_Primitive* model = malloc(sizeof(Model_Internal_Primitive));
  assert(model);

  GLsizeiptr size = sizeof(frame_verts);
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, size, frame_verts, GL_STATIC_DRAW);

  *model = (Model_Internal_Primitive) {
    .type = MODEL_FRAME,
    .vert_count = 3,
    .index_count = 0,
    .ready = true,
    .vbo = vbo,
    .vao = 0,
  };

  return (Model)model;
}

////////////////////////////////////////////////////////////////////////////////

void _model_bind_prim_frame(Model model) {
  Model_Internal_Primitive* frame = (Model_Internal_Primitive*)model;
  assert(frame->type == MODEL_FRAME);
  assert(frame->ready);
  assert(frame->vbo);

  glBindBuffer(GL_ARRAY_BUFFER, frame->vbo);

  // position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, v3floats, GL_FLOAT, GL_FALSE, v3bytes, 0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

////////////////////////////////////////////////////////////////////////////////
// Joint new basic primitive function
////////////////////////////////////////////////////////////////////////////////

Model model_new_primitive(model_type_t type) {
  switch (type) {

    case MODEL_CUBE:
      return _model_new_cube();

    case MODEL_CUBE_COLOR:
      return _model_new_cube_color();

    case MODEL_FRAME:
      return _model_new_frame();

    default:
      str_log("[Model.new_primitive] Not a primitive type: {}", (int)type);
      assert(0);
      break;
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Primitive and basic render functions
////////////////////////////////////////////////////////////////////////////////

void _model_render_prim(Model model) {
  Model_Internal_Primitive* prim = (Model_Internal_Primitive*)model;

  if (!prim->vao) {
    glGenVertexArrays(1, &prim->vao);
    glBindVertexArray(prim->vao);
    model_bind((Model)prim);
  }
  else {
    glBindVertexArray(prim->vao);
  }

  if (prim->index_count) {
    glDrawElements
    ( GL_TRIANGLES
    , (GLsizei)prim->index_count
    , GL_UNSIGNED_INT
    , 0
    );
  }
  else {
    glDrawArrays
    ( GL_TRIANGLES
    , 0
    , (GLsizei)prim->vert_count
    );
  }
  glBindVertexArray(0);
}

////////////////////////////////////////////////////////////////////////////////

void _model_render_prim_strip(Model model) {
  Model_Internal_Primitive* prim = (Model_Internal_Primitive*)model;
  assert(prim->type == MODEL_CUBE_COLOR);

  if (!prim->vao) {
    glGenVertexArrays(1, &prim->vao);
    glBindVertexArray(prim->vao);
    model_bind((Model)prim);
  }
  else {
    glBindVertexArray(prim->vao);
  }

  glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei)prim->vert_count);
  glBindVertexArray(0);
}
