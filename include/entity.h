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

#include "types.h"

typedef struct _opaque_Game_t* Game;
typedef struct entity_t entity_t;
typedef struct renderer_t renderer_t;

#include "mat.h"
#include "model.h"
#include "texture.h"
#include "shader.h"
#include "array.h"
#include "material.h"
#include "slotkey.h"

typedef void (*entity_update_fn_t)(Game game, entity_t* e, float dt);
typedef void (*entity_render_fn_t)(Game game, entity_t* e);
typedef void (*entity_create_fn_t)(Game game, entity_t* e);
typedef void (*entity_delete_fn_t)(Game game, entity_t* e);

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

slotkey_t entity_add(Game game, const entity_t* entity);
void      entity_remove(Game game, slotkey_t entity_id);
entity_t* entity_ref(Game game, slotkey_t entity_id);

void      entity_set_behavior(entity_t*, entity_update_fn_t bh);
void      entity_set_renderer(entity_t*, renderer_t*);

void      entity_apply_transform(slotkey_t entity_id);

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
