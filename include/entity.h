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

typedef struct _opaque_Game_t* Game;
typedef struct entity_t entity_t;

typedef void (*entity_update_fn_t)(Game game, entity_t* e, float dt);
typedef void (*entity_render_fn_t)(Game game, entity_t* e);
typedef void (*entity_create_fn_t)(Game game, entity_t* e);
typedef void (*entity_delete_fn_t)(Game game, entity_t* e);

typedef struct entity_id_t {
  uint index;
  uint unique;
} entity_id_t;

// todo: if "Entity" is not partially opaque, should it be "entity" instead?
//          should it just be opaque? Should entities be created via a prefab
//          or entity_builder type object (want a way to define them inline in
//          levels and whatnot of course).
typedef struct entity_t {
  entity_id_t id;

  vec3 pos;

  union {
    quat rotation;
    float angle;
    index_t tmp; // replace with custom data fields/pointer?
  };

  mat4 transform;
  const Model* model;
  Material material;
  vec3 tint;

  bool hidden;

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
