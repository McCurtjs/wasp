#include "model.h"

#include "gl.h"

#include <stdlib.h>
#include <string.h>

#include "utility.h"
#include "file.h"

#include "wasm.h"

// inlined static data declarations for model primitives
#include "data/inline_primitives.h"

////////////////////////////////////////////////////////////////////////////////
// Model Grid
////////////////////////////////////////////////////////////////////////////////

typedef struct Model_Internal_Grid {
  model_type_t type;
  index_t vert_count;
  index_t index_count;
  bool ready;
  union {
    model_grid_t grid;
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

  *grid = (Model_Internal_Grid){
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

    for (int j = 1; i < grid->vert_count; i += 8, ++j) {
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

static void _model_render_grid(Model_Internal_Grid* grid) {
  glBindVertexArray(grid->vao);
  glDrawArrays(GL_LINES, 0, (GLsizei)grid->vert_count);
  glBindVertexArray(0);
}

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

static void _model_bind_prim_cube(Model _model) {
  Model_Internal_Primitive* model = (Model_Internal_Primitive*)_model;
  assert(model->type == MODEL_CUBE);
  assert(model->ready);
  assert(model->vbo);

  GLsizei stride = 12 * sizeof(float);

  glBindBuffer(GL_ARRAY_BUFFER, model->vbo);

  // position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, v3floats, GL_FLOAT, GL_FALSE, stride, 0);

  // normal
  glEnableVertexAttribArray(1);
  const void* offset = (void*)12;
  glVertexAttribPointer(1, v3floats, GL_FLOAT, GL_FALSE, stride, offset);

  // uv
  glEnableVertexAttribArray(2);
  offset = (void*)24;
  glVertexAttribPointer(2, v2floats, GL_FLOAT, GL_FALSE, stride, offset);

  // tangent
  glEnableVertexAttribArray(3);
  offset = (void*)32;
  glVertexAttribPointer(3, v4floats, GL_FLOAT, GL_FALSE, stride, offset);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

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

static void _model_bind_prim_cube_color(Model _model) {
  Model_Internal_Primitive* model = (Model_Internal_Primitive*)_model;
  assert(model->type == MODEL_CUBE_COLOR);
  assert(model->ready);
  assert(model->vbo);

  glBindBuffer(GL_ARRAY_BUFFER, model->vbo);

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

static void _model_bind_prim_frame(Model _model) {
  Model_Internal_Primitive* model = (Model_Internal_Primitive*)_model;
  assert(model->type == MODEL_FRAME);
  assert(model->ready);
  assert(model->vbo);

  glBindBuffer(GL_ARRAY_BUFFER, model->vbo);

  // position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, v3floats, GL_FLOAT, GL_FALSE, v3bytes, 0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

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

static void _model_render_prim(Model_Internal_Primitive* prim) {
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

static void _model_render_prim_strip(Model_Internal_Primitive* prim) {
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

////////////////////////////////////////////////////////////////////////////////
// Sprites
////////////////////////////////////////////////////////////////////////////////

typedef struct sprite_vertex_t {
  vec2 pos;
  vec2 uv;
  vec3 norm;
  color3b tint;
} sprite_vertex_t;

typedef struct Model_Internal_Sprites {
  model_type_t type;
  index_t vert_count;
  index_t index_count;
  bool ready;

  // Hidden
  vec2i grid;
  Array verts;
  GLuint vbo;
  GLuint vao;
} Model_Internal_Sprites;

Model model_new_sprites(vec2i grid) {
  Model_Internal_Sprites* sprites = malloc(sizeof(Model_Internal_Sprites));
  assert(sprites);

  GLuint vbo;
  glGenBuffers(1, &vbo);

  *sprites = (Model_Internal_Sprites) {
    .type = MODEL_SPRITES,
    .vert_count = 0,
    .index_count = 0,
    .ready = true,
    .grid = grid,
    .verts = arr_new(sprite_vertex_t),
    .vbo = vbo,
    .vao = 0,
  };

  return (Model)sprites;
}

static void _model_bind_sprites(Model_Internal_Sprites* sprites) {
  sprites->verts = arr_new(sprite_vertex_t);
  assert(sprites->type == MODEL_SPRITES);
  assert(sprites->ready);
  assert(sprites->vbo);
  assert(sprites->verts);

  glBindBuffer(GL_ARRAY_BUFFER, sprites->vbo);

  GLsizei stride = (GLsizei)sizeof(sprite_vertex_t);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(
    0, v2floats, GL_FLOAT, GL_FALSE, stride, &((sprite_vertex_t*)0)->pos
  );

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(
    1, v3floats, GL_FLOAT, GL_FALSE, stride, &((sprite_vertex_t*)0)->norm
  );

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(
    2, v2floats, GL_FLOAT, GL_FALSE, stride, &((sprite_vertex_t*)0)->uv
  );

  glEnableVertexAttribArray(3);
  glVertexAttribPointer(
    3, b3bytes, GL_UNSIGNED_BYTE, GL_TRUE, stride, &((sprite_vertex_t*)0)->tint
  );
}

static void _model_render_sprites(Model_Internal_Sprites* sprites) {
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
  sprite_vertex_t BL, TL, TR, BR;

  BL = (sprite_vertex_t) {
    .pos  = v2add(pos, v2neg(scale)),
    .uv   = v2f(corner.x, corner.y + extent.y),
    .norm = v3z,
    .tint = b4white.rgb,
  };

  TR = (sprite_vertex_t) {
    .pos  = v2add(pos, scale),
    .uv   = v2f(corner.x + extent.x, corner.y),
    .norm = v3z,
    .tint = b4white.rgb,
  };

  TL = (sprite_vertex_t) {
    .pos  = v2add(pos, v2f(-scale.x, scale.y)),
    .uv   = corner,
    .norm = v3z,
    .tint = b4white.rgb,
  };

  BR = (sprite_vertex_t) {
    .pos  = v2add(pos, v2f(scale.x, -scale.y)),
    .uv   = v2add(corner, extent),
    .norm = v3z,
    .tint = b4white.rgb,
  };

  arr_write_back(spr->verts, &TR);
  arr_write_back(spr->verts, &TL);
  arr_write_back(spr->verts, &BL);
  arr_write_back(spr->verts, &TR);
  arr_write_back(spr->verts, &BL);
  arr_write_back(spr->verts, &BR);
}

////////////////////////////////////////////////////////////////////////////////
// Model OBJ
////////////////////////////////////////////////////////////////////////////////

#include "loaders/obj.h"

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

Model model_new_from_obj(File file) {
  assert(file);
  if (!file_read(file)) {
    str_log("[Model.new_from_obj] Can't read file: {}", file->name);
    return NULL;
  }

  Model_Internal_Mesh* model = calloc(1, sizeof(Model_Internal_Mesh));
  assert(model);

  model->type = MODEL_OBJ;
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

static void _model_bind_mesh(Model _model) {
  Model_Internal_Mesh* model = (Model_Internal_Mesh*)_model;
  assert(model->type == MODEL_OBJ);
  assert(model->ready);
  assert(model->vbo);
  assert(model->ebo);

  glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ebo);

  GLsizei vert_size = model->use_color
    ? sizeof(obj_vertex_color_t)
    : sizeof(obj_vertex_t);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(
    0, v3floats, GL_FLOAT, GL_FALSE, vert_size, &((obj_vertex_t*)0)->pos
  );

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(
    1, v3floats, GL_FLOAT, GL_FALSE, vert_size, &((obj_vertex_t*)0)->norm
  );

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(
    2, v2floats, GL_FLOAT, GL_FALSE, vert_size, &((obj_vertex_t*)0)->uv
  );

  glEnableVertexAttribArray(3);
  glVertexAttribPointer(
    3, v4floats, GL_FLOAT, GL_FALSE, vert_size, &((obj_vertex_t*)0)->tangent
  );

  if (model->use_color) {
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(3, v3floats, GL_FLOAT, GL_FALSE, vert_size
    , &((obj_vertex_color_t*)0)->color
    );
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ebo);
}

////////////////////////////////////////////////////////////////////////////////

static void _model_render_mesh(Model_Internal_Mesh* mesh) {
  assert(mesh->index_count);

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

////////////////////////////////////////////////////////////////////////////////
// Function routing based on object types
////////////////////////////////////////////////////////////////////////////////

typedef void (*model_render_fn_t)(void* model);
typedef void (*model_bind_fn_t)(const void* model);
typedef void (*model_render_inst_fn_t)(const void* model, index_t count);

typedef struct model_management_fns_t {
  model_bind_fn_t bind;
  model_render_fn_t render_single;
  model_render_inst_fn_t render_inst;
} model_management_fns_t;

static model_management_fns_t model_management_fns[MODEL_TYPES_COUNT] = {
  // MODEL_NONE - No model type selected
  { 0 },

  // MODEL_GRID - Grid rendering
  { .render_single = (model_render_fn_t)_model_render_grid,
  },

  // MODEL_CUBE - Basic cube primitive (with normals and tangents)
  { .bind = (model_bind_fn_t)_model_bind_prim_cube
  , .render_single = (model_render_fn_t)_model_render_prim
  , .render_inst = (model_render_inst_fn_t)_model_render_instanced
  },

  // MODEL_CUBE_COLOR - Debug-cube object with vertex color, no normals
  { .bind = (model_bind_fn_t)_model_bind_prim_cube_color
  , .render_single = (model_render_fn_t)_model_render_prim_strip
  },

  // MODEL_FRAME - Full-screen frame model for deferred rendering
  { .bind = (model_bind_fn_t)_model_bind_prim_frame
  , .render_single = (model_render_fn_t)_model_render_prim
  },

  // MODEL_SPRITES - Accumulated collection of sprites
  { .bind = (model_bind_fn_t)_model_bind_sprites
  , .render_single = (model_render_fn_t)_model_render_sprites
  },

  // MODEL_OBJ - Basic mesh type loaded from .obj file
  { .bind = (model_bind_fn_t)_model_bind_mesh
  , .render_single = (model_render_fn_t)_model_render_mesh
  , .render_inst = (model_render_inst_fn_t)_model_render_instanced
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

  model_bind_fn_t bind_fn = model_management_fns[model_type].bind;

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

  model_render_fn_t render_fn = model_management_fns[model_type].render_single;

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

  model_render_inst_fn_t inst_fn = model_management_fns[model_type].render_inst;

  if (!inst_fn) {
    str_log
    ( "[Model.render_inst] No instanced renderer for type: {}"
    , model_type
    );
    return;
  }

  inst_fn(model, count);
}
