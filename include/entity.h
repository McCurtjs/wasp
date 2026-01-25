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

#ifndef WASP_ENTITY_H_
#define WASP_ENTITY_H_

#include "types.h"

typedef struct _opaque_Game_t* Game;
typedef struct entity_t entity_t;
typedef struct renderer_t renderer_t;

#include "mat.h"
#include "quat.h"
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

typedef struct transform_t {
  quat  rot;
  vec3  pos;
  float scale;
} transform_t;

typedef struct entity_desc_t {
  slotkey_t           user_id;
  slotkey_t           parent_id;
  slice_t             name;

  Model               model;
  Material            material;

  union {
    transform_t       transform;
    struct {
      quat            rot;
      vec3            pos;
      float           scale;
    };
  };

  color3              tint;
  bool                is_static;
  bool                is_hidden;

  renderer_t*         renderer;

  entity_update_fn_t  behavior;
  entity_render_fn_t  render;
  entity_create_fn_t  oncreate;
  entity_delete_fn_t  ondelete;
} entity_desc_t;

#ifdef WASP_ENTITY_INTERNAL
#undef CONST
#define CONST
#endif

typedef struct entity_t {
  // Entity manager/identifier
  slotkey_t           CONST id;
  slotkey_t           CONST parent_id;
  String                    name;

  // Transform
  union {
    transform_t       CONST transform;
    struct {
      quat            CONST rot;
      vec3            CONST pos;
      float           CONST scale;
    };
  };

  // Visual
  renderer_t*         CONST renderer;
  slotkey_t           CONST render_id;
  Model               CONST model;
  Material            CONST material;

  // Flags
  bool                CONST is_hidden;
  bool                CONST is_static;
  bool                CONST is_dirty_renderer;
  bool                CONST is_dirty_static;

  // Entity attributes
  float                     create_time;
  slotkey_t                 user_id;

  // Actions and event callbacks
  entity_update_fn_t        behavior;
  entity_render_fn_t        render; // onrender, happens before renderer but
  entity_create_fn_t        oncreate;         // after transform sync
  entity_delete_fn_t        ondelete;
}* Entity;

#ifdef WASP_ENTITY_INTERNAL
#undef CONST
#define CONST const
#endif

slotkey_t entity_add(const entity_desc_t* entity);
void      entity_remove(slotkey_t entity_id);
index_t   entity_count(void);
Entity    entity_ref(slotkey_t entity_id);
Entity    entity_next(slotkey_t* entity_id);

void      entity_set_parent(Entity, const Entity new_parent);
void      entity_set_behavior(Entity, entity_update_fn_t bh);

mat4      entity_transform(Entity);

void      entity_set_renderer(Entity, renderer_t*);
void      entity_set_hidden(Entity, bool is_hidden);
void      entity_set_static(Entity, bool is_static);

void      entity_set_position(Entity, vec3 new_pos);
void      entity_set_rotation(Entity, quat new_rot);
void      entity_set_rotation_a(Entity, vec3 axis, float angle);
void      entity_set_scale(Entity, float uniform_scale);
void      entity_teleport(Entity, vec3 pos, quat rotation);
void      entity_teleport_a(Entity, vec3 pos, vec3 axis, float angle);
void      entity_translate(Entity, vec3 dir);
void      entity_rotate(Entity, quat rotation);
void      entity_rotate_a(Entity, vec3 axis, float angle);

#endif
