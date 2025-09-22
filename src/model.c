#include "model.h"

#include "gl.h"

#include <stdlib.h>
#include <string.h>

#include "utility.h"
#include "file.h"

#include "wasm.h"

// inlined static data declarations for model primitives
#include "data/inline_primitives.h"

// model data loaders
#include "loaders/obj.h"

// Shared bindings for primitives

static GLuint cube_pos_buffer = 0;
static void prim_bind_cube() {
  if (!cube_pos_buffer) {
    glGenBuffers(1, &cube_pos_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, cube_pos_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_pos), cube_pos, GL_STATIC_DRAW);
  } else {
    glBindBuffer(GL_ARRAY_BUFFER, cube_pos_buffer);
  }
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, sizeof(*cube_pos) / 4, GL_FLOAT, GL_FALSE, 0, 0);
}

static GLuint cube_color_buffer = 0;
static GLuint cube_color_vao = 0;
static void prim_bind_cube_color() {
  if (!cube_color_buffer) {
    GLsizeiptr size = sizeof(cube_color);
    glGenBuffers(1, &cube_color_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, cube_color_buffer);
    glBufferData(GL_ARRAY_BUFFER, size, cube_color, GL_STATIC_DRAW);
  } else {
    glBindBuffer(GL_ARRAY_BUFFER, cube_color_buffer);
  }
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, sizeof(*cube_color) / 4, GL_FLOAT, GL_FALSE, 0, 0);
}

static GLuint cube_vertex_buffer = 0;
static GLuint cube_vao = 0;
static void prim_bind_cube_2() {
  if (!cube_vertex_buffer) {
    GLsizeiptr size = sizeof(cube2_verts);
    glGenBuffers(1, &cube_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, size, cube2_verts, GL_STATIC_DRAW);
  } else {
    glBindBuffer(GL_ARRAY_BUFFER, cube_vertex_buffer);
  }
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);

  glEnableVertexAttribArray(1);
  const void* offset = (void*)12;
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), offset);

  glEnableVertexAttribArray(2);
  offset = (void*)24;
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), offset);
}

// Model_Grid

static int model_build_grid(Model_Grid* grid) {
  if (grid->ready) return 1;

  glGenVertexArrays(1, &grid->vao);
  glBindVertexArray(grid->vao);

  int ext = grid->extent;
  int gext = ext;

  if (ext <= 0) {
    grid->points_count = 6;
    gext = -ext > 1 ? -ext : 1;
  } else {
    grid->points_count = 12 + 8 * ext;
  }

  float exf = (float)ext;

  vec3* points = malloc(sizeof(vec3) * grid->points_count);
  color3b* colors = malloc(sizeof(color3b) * grid->points_count);

  assert(points != NULL);
  assert(colors != NULL);

  uint i = 0;
  const vec3* basis = grid->basis;
  const byte  ga = MIN(grid->primary[0], 2);
  const byte  gb = MIN(grid->primary[1], 2);

  for (int j = 0; i < 6; i += 2, ++j) {
    colors[i] = b3zero;
    points[i] = points[i+1] = v3zero;
    colors[i].i[j] = 255;
    colors[i+1].i[j] = colors[i].i[j];
    points[i] = v3scale(basis[j], (float)gext);
  }

  if (ext > 0) {

    for (uint j = 0; i < 12; i += 2, ++j) {
      colors[i] = colors[i+1] = v3b(255, 255, 255);
      points[i] = points[i+1] = v3zero;
      points[i] = v3scale(basis[j], -exf);
    }

    for (int j = 1; i < grid->points_count; i += 8, ++j) {
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
      for (uint k = 0; k < 8; ++k) {
        colors[i + k] = color;
      }
    }
  }

  GLsizeiptr points_size = sizeof(*points) * grid->points_count;
  glGenBuffers(2, grid->buffers);
  glBindBuffer(GL_ARRAY_BUFFER, grid->buffers[0]);
  glBufferData(GL_ARRAY_BUFFER, points_size, points, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  GLsizeiptr colors_size = sizeof(*colors) * grid->points_count;
  glBindBuffer(GL_ARRAY_BUFFER, grid->buffers[1]);
  glBufferData(GL_ARRAY_BUFFER, colors_size, colors, GL_STATIC_DRAW);
  glVertexAttribPointer(1, sizeof(*colors), GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
  glEnableVertexAttribArray(1);

  free(points);
  free(colors);

  glBindVertexArray(0);
  grid->ready = TRUE;
  return 1;
}

static void model_render_grid(const Model_Grid* grid) {
  glBindVertexArray(grid->vao);
  glDrawArrays(GL_LINES, 0, grid->points_count);
  glBindVertexArray(0);
}

// Model_Cube

static int model_build_cube(Model_Cube* cube) {
  if (cube_vao) return 1;
  glGenVertexArrays(1, &cube_vao);
  glBindVertexArray(cube_vao);
  prim_bind_cube_2();
  glBindVertexArray(0);
  cube->ready = TRUE;
  return 1;
}

static void model_render_cube(const Model_Cube* cube) {\
  PARAM_UNUSED(cube);

  glBindVertexArray(cube_vao);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
}

// Model_CubeColor

static int model_build_cube_color(Model_CubeColor* cube) {
  if (cube_color_vao) return 1;
  glGenVertexArrays(1, &cube_color_vao);
  glBindVertexArray(cube_color_vao);
  prim_bind_cube();
  prim_bind_cube_color();
  glBindVertexArray(0);
  cube->ready = TRUE;
  return 1;
}

static void model_render_cube_color(const Model_CubeColor* cube) {
  PARAM_UNUSED(cube);

  glBindVertexArray(cube_color_vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
  glBindVertexArray(0);
}

// Model_Sprites

typedef struct SpriteVertex {
  vec2 pos;
  vec2 uv;
  vec3 norm;
  color3b tint;
} SpriteVertex;

static int model_build_sprites(Model_Sprites* sprites) {
  sprites->verts = array_new(SpriteVertex);

  glGenVertexArrays(1, &sprites->vao);
  glBindVertexArray(sprites->vao);

  glGenBuffers(1, &sprites->buffer);
  glBindBuffer(GL_ARRAY_BUFFER, sprites->buffer);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, v2floats, GL_FLOAT, GL_FALSE,
    sizeof(SpriteVertex), &((SpriteVertex*)0)->pos);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, v3floats, GL_FLOAT, GL_FALSE,
    sizeof(SpriteVertex), &((SpriteVertex*)0)->norm);

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, v2floats, GL_FLOAT, GL_FALSE,
    sizeof(SpriteVertex), &((SpriteVertex*)0)->uv);

  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, b3bytes, GL_UNSIGNED_BYTE, GL_TRUE,
    sizeof(SpriteVertex), &((SpriteVertex*)0)->tint);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  sprites->ready = TRUE;
  return 1;
}

static void model_render_sprites(Model_Sprites* sprites) {
  glBindVertexArray(sprites->vao);
  glBindBuffer(GL_ARRAY_BUFFER, sprites->buffer);

  index_t size_bytes = sprites->verts->size_bytes;
  void* data_start = array_ref_front(sprites->verts);
  glBufferData(GL_ARRAY_BUFFER, size_bytes, data_start, GL_DYNAMIC_DRAW);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDrawArrays(GL_TRIANGLES, 0, (int)sprites->verts->size);
  glDisable(GL_BLEND);

  glBindVertexArray(0);

  array_clear(sprites->verts);
}

void model_sprites_draw(
  const Model_Sprites* sprites, vec2 pos, vec2 scale, index_t frame, bool mirror
) {
  Model_Sprites* spr = (Model_Sprites*)sprites;

  frame = frame % (spr->grid.w * spr->grid.h);

  vec2 extent = v2f( 1.f / spr->grid.w, -1.f / spr->grid.h );
  vec2 corner = v2f(
    (frame % spr->grid.w) / (float)spr->grid.w,
    1 - (frame / spr->grid.w) / (float)spr->grid.h
  );

  if (mirror) {
    corner.x += 1.f / (float)spr->grid.w;
    extent.x *= -1;
  }

  scale = v2scale(scale, 0.5);
  SpriteVertex BL, TL, TR, BR;

  BL = (SpriteVertex) {
    .pos  = v2add(pos, v2neg(scale)),
    .uv   = v2f(corner.x, corner.y + extent.y),
    .norm = v3z,
    .tint = b4white.rgb,
  };

  TR = (SpriteVertex) {
    .pos  = v2add(pos, scale),
    .uv   = v2f(corner.x + extent.x, corner.y),
    .norm = v3z,
    .tint = b4white.rgb,
  };

  TL = (SpriteVertex) {
    .pos  = v2add(pos, v2f(-scale.x, scale.y)),
    .uv   = corner,
    .norm = v3z,
    .tint = b4white.rgb,
  };

  BR = (SpriteVertex) {
    .pos  = v2add(pos, v2f(scale.x, -scale.y)),
    .uv   = v2add(corner, extent),
    .norm = v3z,
    .tint = b4white.rgb,
  };

  array_write_back(spr->verts, &TR);
  array_write_back(spr->verts, &TL);
  array_write_back(spr->verts, &BL);
  array_write_back(spr->verts, &TR);
  array_write_back(spr->verts, &BL);
  array_write_back(spr->verts, &BR);
}

// Model OBJ

typedef struct ObjVertex {
  vec3 pos;
  vec3 color;
  vec3 norm;
  vec2 uv;
} ObjVertex;

void model_load_obj(Model* model, File file) {
  if (!file_read(file)) {
    model->type = MODEL_NONE;
    return;
  }

  model->type = MODEL_OBJ;
  file_load_obj(&model->mesh, file);
}

static int model_build_mesh(Model_Mesh* mesh) {
  glGenVertexArrays(1, &mesh->vao);
  glBindVertexArray(mesh->vao);

  glGenBuffers(2, mesh->buffers);

  glBindBuffer(GL_ARRAY_BUFFER, mesh->vert_buffer);
  glBufferData(GL_ARRAY_BUFFER, mesh->verts->size_bytes,
               array_ref_front(mesh->verts), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, v3floats, GL_FLOAT, GL_FALSE,
                        sizeof(ObjVertex), &((ObjVertex*)0)->pos);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, v3floats, GL_FLOAT, GL_FALSE,
                        sizeof(ObjVertex), &((ObjVertex*)0)->norm);
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, v2floats, GL_FLOAT, GL_FALSE,
                        sizeof(ObjVertex), &((ObjVertex*)0)->uv);
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, v3floats, GL_FLOAT, GL_FALSE,
                        sizeof(ObjVertex), &((ObjVertex*)0)->color);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices->size_bytes,
               array_ref_front(mesh->indices), GL_STATIC_DRAW);

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  mesh->ready = TRUE;
  return 1;
}

static void model_render_mesh(Model_Mesh* mesh) {
  glBindVertexArray(mesh->vao);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
  glDrawElements(GL_TRIANGLES, (int)mesh->indices->size, GL_UNSIGNED_INT, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

// Exported functions

typedef int  (*model_build_pfn)(void* model);
typedef void (*model_render_pfn)(const void* model);

static model_build_pfn model_build_fns[MODEL_TYPES_COUNT] = {
  (model_build_pfn)model_build_grid,
  (model_build_pfn)model_build_cube,
  (model_build_pfn)model_build_cube_color,
  (model_build_pfn)model_build_sprites,
  (model_build_pfn)model_build_mesh,
};

int model_build(Model* model) {
  if (!model || !model->type || model->ready) return 0;

  return model_build_fns[model->type - 1](model);
}

static model_render_pfn model_render_fns[MODEL_TYPES_COUNT] = {
  (model_render_pfn)model_render_grid,
  (model_render_pfn)model_render_cube,
  (model_render_pfn)model_render_cube_color,
  (model_render_pfn)model_render_sprites,
  (model_render_pfn)model_render_mesh
};

void model_render(const Model* model) {
  if (!model || !model->type || !model->ready) return;

  model_render_fns[model->type - 1](model);
}

void model_grid_set_default(Model* model, int extent) {
  model->grid = (Model_Grid) {
    .type = MODEL_GRID, .ready = FALSE, .extent = extent,
    .basis = {v3x, v3y, v3z}, .primary = {0, 2}
  };
}
