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
// Sprites
////////////////////////////////////////////////////////////////////////////////

typedef struct Model_Internal_Sprites {
  model_type_t type;
  vert_format_t format;
  index_t vert_count;
  index_t index_count;
  bool ready;

  // Hidden
  vec2i grid;
  Array verts;
  GLuint vbo;
  GLuint vao;
} Model_Internal_Sprites;

////////////////////////////////////////////////////////////////////////////////

Model model_new_sprites(vec2i grid) {
  Model_Internal_Sprites* sprites = malloc(sizeof(Model_Internal_Sprites));
  assert(sprites);

  GLuint vbo;
  glGenBuffers(1, &vbo);

  *sprites = (Model_Internal_Sprites) {
    .type = MODEL_SPRITES,
    .format = VF_SPRITES,
    .vert_count = 0,
    .index_count = 0,
    .ready = true,
    .grid = grid,
    .verts = arr_new(vert_sprites_t),
    .vbo = vbo,
    .vao = 0,
  };

  return (Model)sprites;
}

////////////////////////////////////////////////////////////////////////////////

void _model_bind_sprites(const Model model) {
  Model_Internal_Sprites* sprites = (Model_Internal_Sprites*)model;
  assert(sprites->type == MODEL_SPRITES);
  assert(sprites->vbo);

  glBindBuffer(GL_ARRAY_BUFFER, sprites->vbo);
  vert_bind(sprites->format);
}

////////////////////////////////////////////////////////////////////////////////

void _model_render_sprites(Model model) {
  Model_Internal_Sprites* sprites = (Model_Internal_Sprites*)model;
  assert(sprites->type == MODEL_SPRITES);
  assert(sprites->vbo);
  assert(sprites->verts);

  if (!sprites->vao) {
    glGenVertexArrays(1, &sprites->vao);
    glBindVertexArray(sprites->vao);
    model_bind((Model)sprites);
  }
  else {
    glBindVertexArray(sprites->vao);
  }

  index_t size_bytes = sprites->verts->size_bytes;
  void* data_start = arr_ref_front(sprites->verts);
  glBindBuffer(GL_ARRAY_BUFFER, sprites->vbo);
  glBufferData(GL_ARRAY_BUFFER, size_bytes, data_start, GL_DYNAMIC_DRAW);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDrawArrays(GL_TRIANGLES, 0, (GLsizei)sprites->verts->size);
  glDisable(GL_BLEND);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  arr_clear(sprites->verts);
}

////////////////////////////////////////////////////////////////////////////////
// User-facing sprite-specific helper functions
////////////////////////////////////////////////////////////////////////////////

void model_sprites_add(
  Model _sprites, vec2 pos, vec2 scale, index_t frame, bool mirror
) {
  Model_Internal_Sprites* spr = (Model_Internal_Sprites*)_sprites;
  assert(spr);
  assert(spr->ready);
  assert(spr->verts);

  frame = frame % (spr->grid.w * spr->grid.h);

  vec2 extent = v2f( 1.f / spr->grid.w, -1.f / spr->grid.h );
  vec2 corner = v2f
  ( (frame % spr->grid.w) / (float)spr->grid.w
  , 1 - (frame / spr->grid.w) / (float)spr->grid.h
  );

  if (mirror) {
    corner.x += 1.f / (float)spr->grid.w;
    extent.x *= -1;
  }

  scale = v2scale(scale, 0.5);
  vert_sprites_t BL, TL, TR, BR;

  BL = (vert_sprites_t) {
    .pos  = v2add(pos, v2neg(scale)),
    .uv   = v2f(corner.x, corner.y + extent.y),
    .norm = v3z,
    .color = b4white.rgb,
  };

  TR = (vert_sprites_t) {
    .pos  = v2add(pos, scale),
    .uv   = v2f(corner.x + extent.x, corner.y),
    .norm = v3z,
    .color = b4white.rgb,
  };

  TL = (vert_sprites_t) {
    .pos  = v2add(pos, v2f(-scale.x, scale.y)),
    .uv   = corner,
    .norm = v3z,
    .color = b4white.rgb,
  };

  BR = (vert_sprites_t) {
    .pos  = v2add(pos, v2f(scale.x, -scale.y)),
    .uv   = v2add(corner, extent),
    .norm = v3z,
    .color = b4white.rgb,
  };

  arr_write_back(spr->verts, &TR);
  arr_write_back(spr->verts, &TL);
  arr_write_back(spr->verts, &BL);
  arr_write_back(spr->verts, &TR);
  arr_write_back(spr->verts, &BL);
  arr_write_back(spr->verts, &BR);
}
