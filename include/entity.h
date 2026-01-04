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

#ifndef WASP_ENTITY_H_
#define WASP_ENTITY_H_

#include "mat.h"
#include "model.h"
#include "texture.h"
#include "shader.h"
#include "array.h"
#include "material.h"
#include "slotkey.h"

typedef struct _opaque_Game_t* Game;
typedef struct entity_t entity_t;

typedef void (*entity_update_fn_t)(Game game, entity_t* e, float dt);
typedef void (*entity_render_fn_t)(Game game, entity_t* e);
typedef void (*entity_create_fn_t)(Game game, entity_t* e);
typedef void (*entity_delete_fn_t)(Game game, entity_t* e);

typedef enum renderer_type_t {
  R_BASIC,
  R_PBR,
  R_BASIC_DEFERRED,
  R_PBR_DEFERRED,
  R_CUSTOM_START,
} renderer_type_t;

typedef struct renderer_t renderer_t;

// Called once for each entity that's flagged for update
typedef void (*renderer_entity_update_fn_t)(renderer_t*, entity_t*);

typedef void (*renderer_entity_register_fn_t)(entity_t*, Game);
typedef void (*renderer_entity_unregister_fn_t)(entity_t*);

// Called once for each entity before the final render function
typedef void (*renderer_entity_render_fn_t)(renderer_t*, Game, entity_t*);

// Called once per frame after the entities are processed 
typedef void (*renderer_render_fn_t)(renderer_t*, Game);

// Called when the renderer is destroyed
typedef void (*renderer_delete_fn_t)(renderer_t**);

typedef struct renderer_basic_t {
  renderer_type_t type;
} renderer_basic_t;

typedef struct renderer_t {
  renderer_entity_register_fn_t register_entity;
  renderer_entity_unregister_fn_t unregister_entity;
  renderer_entity_update_fn_t update_entity;
  renderer_entity_render_fn_t render_entity;
  renderer_render_fn_t render;
} renderer_t;

#define con_type renderer_t*
#define con_prefix renderer
#include "span.h"
#undef con_prefix
#undef con_type

typedef struct entity_t {
  slotkey_t id;
  slice_t name;
  float create_time;

  vec3 pos;

  union {
    quat rotation;
    float angle;
    slotkey_t tmp; // replace with custom data fields/pointer?
  };

  mat4 transform;
  Model* model;
  Material material;
  vec3 tint;

  bool is_static;
  bool is_hidden;

  renderer_t* renderer;
  slotkey_t render_id;

  entity_update_fn_t behavior;
  entity_render_fn_t render;
  entity_create_fn_t oncreate;
  entity_delete_fn_t ondelete;

} entity_t;

/*
typedef struct Entity2 {
  vec3 pos;
  quat dir;

  mat4 transform;

  Model model;
  Material material;
  vec3 tint;

  // Renderer renderer;
  // {
  //    type: fn | managed;
  //    union {
  //      RenderFn render;
  //      RenderManager manager;
  //    };
  // }
  //
  // Controller controller;
  // {
  //    type: fn | controller;
  //    union {
  //      UpdateFn behavior;
  //      Controller controller;
  //    }
  // }
  UpdateFn behavior;
  DeleteFn delete;

  bool hidden;
  bool frozen;
};//*/

#endif
