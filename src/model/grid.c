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
// Model Grid
////////////////////////////////////////////////////////////////////////////////

typedef struct Model_Internal_Grid {
  model_type_t type;
  index_t vert_count;
  index_t index_count;
  bool ready;
  union {
    model_grid_param_t params;
    struct {
      int extent;
      vec3 basis[3];
      byte primary[2];
    };
  };

  // Secrets
  union {
    GLuint buffers[2];
    struct {
      GLuint vbo; // buffer for verts
      GLuint colors; // buffer for vertex colors
    };
  };
  GLuint vao;

} Model_Internal_Grid;

////////////////////////////////////////////////////////////////////////////////

Model model_new_grid(model_grid_param_t param) {
  Model_Internal_Grid* grid = malloc(sizeof(Model_Internal_Grid));
  assert(grid);

  *grid = (Model_Internal_Grid) {
    .type = MODEL_GRID,
    .ready = false,
    .params = param
  };

  assert(grid->extent == param.extent);

  glGenVertexArrays(1, &grid->vao);
  glBindVertexArray(grid->vao);

  int ext = grid->extent;
  int gext = ext;

  if (ext <= 0) {
    grid->vert_count = 6;
    gext = -ext > 1 ? -ext : 1;
  } else {
    grid->vert_count = 12 + 8 * ext;
  }

  float exf = (float)ext;

  vec3* points = malloc(sizeof(vec3) * grid->vert_count);
  color3b* colors = malloc(sizeof(color3b) * grid->vert_count);

  assert(points != NULL);
  assert(colors != NULL);

  index_t i = 0;
  const vec3* basis = grid->basis;
  const byte  ga = MIN(grid->primary[0], 2);
  const byte  gb = MIN(grid->primary[1], 2);

  for (index_t j = 0; i < 6; i += 2, ++j) {
    colors[i] = b3zero;
    points[i] = points[i+1] = v3zero;
    colors[i].i[j] = 255;
    colors[i+1].i[j] = colors[i].i[j];
    points[i] = v3scale(basis[j], (float)gext);
  }

  if (ext > 0) {

    for (index_t j = 0; i < 12; i += 2, ++j) {
      colors[i] = colors[i+1] = v3b(255, 255, 255);
      points[i] = points[i+1] = v3zero;
      points[i] = v3scale(basis[j], -exf);
    }

    for (index_t j = 1; i < grid->vert_count; i += 8, ++j) {
      // (x * ext) + (y * j)
      float jf = (float)j;
      points[i+0] = v3add(v3scale(basis[ga], exf), v3scale(basis[gb], jf));
      points[i+1] = v3add(v3scale(basis[ga],-exf), v3scale(basis[gb], jf));
      points[i+2] = v3add(v3scale(basis[ga], exf), v3scale(basis[gb],-jf));
      points[i+3] = v3add(v3scale(basis[ga],-exf), v3scale(basis[gb],-jf));
      points[i+4] = v3add(v3scale(basis[ga], jf),  v3scale(basis[gb], exf));
      points[i+5] = v3add(v3scale(basis[ga], jf),  v3scale(basis[gb],-exf));
      points[i+6] = v3add(v3scale(basis[ga],-jf),  v3scale(basis[gb], exf));
      points[i+7] = v3add(v3scale(basis[ga],-jf),  v3scale(basis[gb],-exf));

      byte c = (j % 10 == 0 ? 128 : (j % 5 == 0 ? 0 : 63));
      color3b color = v3b(c, c, c);
      for (index_t k = 0; k < 8; ++k) {
        colors[i + k] = color;
      }
    }
  }

  GLsizeiptr points_size = sizeof(*points) * grid->vert_count;
  glGenBuffers(2, grid->buffers);
  glBindBuffer(GL_ARRAY_BUFFER, grid->vbo);
  glBufferData(GL_ARRAY_BUFFER, points_size, points, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

  GLsizeiptr colors_size = sizeof(*colors) * grid->vert_count;
  glBindBuffer(GL_ARRAY_BUFFER, grid->colors);
  glBufferData(GL_ARRAY_BUFFER, colors_size, colors, GL_STATIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, sizeof(*colors), GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);

  free(points);
  free(colors);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  grid->ready = TRUE;

  return (Model)grid;
}

////////////////////////////////////////////////////////////////////////////////

Model model_new_grid_default(int extent) {
  model_grid_param_t params = {
    .basis = {v3x, v3y, v3z},
    .primary = {0, 2},
    .extent = extent,
  };
  return model_new_grid(params);
}

////////////////////////////////////////////////////////////////////////////////

void _model_render_grid(Model model) {
  Model_Internal_Grid* grid = (Model_Internal_Grid*)model;
  assert(grid->type == MODEL_GRID);
  assert(grid->ready);
  assert(grid->vao);
  glBindVertexArray(grid->vao);
  glDrawArrays(GL_LINES, 0, (GLsizei)grid->vert_count);
  glBindVertexArray(0);
}
