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
  vert_format_t format;
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

  GLsizeiptr size = sizeof(primitive_cube_uv_norm);
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, size, primitive_cube_uv_norm, GL_STATIC_DRAW);

  *model = (Model_Internal_Primitive) {
    .type = MODEL_CUBE,
    .format = VF_UV_NORM,
    .vert_count = 36,
    .index_count = 0,
    .ready = true,
    .vbo = vbo,
    .vao = 0,
  };

  return (Model)model;
}

////////////////////////////////////////////////////////////////////////////////
// Vertex-color sample cube
////////////////////////////////////////////////////////////////////////////////

static Model _model_new_cube_color(void) {
  Model_Internal_Primitive* model = malloc(sizeof(Model_Internal_Primitive));
  assert(model);

  GLsizeiptr size = sizeof(primitive_cube_color);
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, size, primitive_cube_color, GL_STATIC_DRAW);

  *model = (Model_Internal_Primitive) {
    .type = MODEL_CUBE_COLOR,
    .format = VF_COLOR,
    .vert_count = 14,
    .index_count = 0,
    .ready = true,
    .vbo = vbo,
    .vao = 0,
  };

  return (Model)model;
}

////////////////////////////////////////////////////////////////////////////////
// Single-triangle for full-screen rendering/deferred
////////////////////////////////////////////////////////////////////////////////

static Model _model_new_frame(void) {
  Model_Internal_Primitive* model = malloc(sizeof(Model_Internal_Primitive));
  assert(model);

  GLsizeiptr size = sizeof(primitive_frame);
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, size, primitive_frame, GL_STATIC_DRAW);

  *model = (Model_Internal_Primitive) {
    .type = MODEL_FRAME,
    .format = VF_POS_ONLY,
    .vert_count = 3,
    .index_count = 0,
    .ready = true,
    .vbo = vbo,
    .vao = 0,
  };

  return (Model)model;
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
// Generic binding function for primitive types
////////////////////////////////////////////////////////////////////////////////

void _model_bind_primitive(Model model) {
  Model_Internal_Primitive* prim = (Model_Internal_Primitive*)model;
  assert(prim->type == MODEL_CUBE
      || prim->type == MODEL_CUBE_COLOR
      || prim->type == MODEL_FRAME
  );
  assert(prim->ready);
  assert(prim->vbo);

  glBindBuffer(GL_ARRAY_BUFFER, prim->vbo);
  vert_bind(prim->format);
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
    glDrawElements(GL_TRIANGLES
    , (GLsizei)prim->index_count
    , GL_UNSIGNED_INT
    , 0
    );
  }
  else {
    glDrawArrays(GL_TRIANGLES
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
