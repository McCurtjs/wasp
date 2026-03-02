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

#ifndef WASP_RENDERER_H_
#define WASP_RENDERER_H_

#include "types.h"

#include "model.h"
#include "material.h"
#include "entity.h"
#include "instance_attributes.h"

typedef struct _opaque_Game_t* Game;
typedef struct renderer_t renderer_t;
typedef struct render_group_t render_group_t;

////////////////////////////////////////////////////////////////////////////////
// Callback types to define renderer functionality
////////////////////////////////////////////////////////////////////////////////

// Called once for each entity that's flagged for update
typedef slotkey_t (*renderer_entity_update_fn_t)(Entity);

// Called when entity is created/registered with the renderer
typedef slotkey_t (*renderer_entity_register_fn_t)(Entity, Game);

// Called when entity is deleted/unregistered from the renderer
typedef void      (*renderer_entity_unregister_fn_t)(Entity);

// Called when directly accessing per-entity instance attributes
typedef void*     (*renderer_entity_attributes_fn_t)(Entity, bool modify);

// Called when a render group has instances updated (non-negative update range)
typedef void      (*renderer_instance_update_fn_t)(render_group_t*);

// Called once per frame after the entities are processed 
typedef void      (*renderer_render_fn_t)(renderer_t*, Game);

// Called when the renderer is destroyed
typedef void      (*renderer_delete_fn_t)(renderer_t**);

////////////////////////////////////////////////////////////////////////////////
// Key type for distinguishing each render by model/material set
////////////////////////////////////////////////////////////////////////////////

typedef struct render_group_key_t {
  Model model;
  Material material;
  size_t is_static;
  // Note: "is_static" can't just be a bool or the padding will be uninitialized
  //    which causes errors because this key is used for hashing. None of the
  //    methods I tried to prevent optimizing out memset worked in clang/wasm.
} render_group_key_t;

////////////////////////////////////////////////////////////////////////////////
// Render group type representing batched draws and instance data layout
////////////////////////////////////////////////////////////////////////////////

#include "packedmap.h"

typedef struct render_group_t {
  union {
    render_group_key_t key;
    struct {
      Model model;
      Material material;
      size_t is_static;
    };
  };
  PackedMap instances;
  uint vao;
  uint instance_buffer;
  int32_t update_range_low;
  int32_t update_range_high;
  bool update_full;
} render_group_t;

////////////////////////////////////////////////////////////////////////////////
// Renderer type for representing shader passes and their related instance data
////////////////////////////////////////////////////////////////////////////////

#define con_type render_group_t
#define key_type render_group_key_t
#define con_prefix rg
#include "map.h"
#undef con_prefix
#undef key_type
#undef con_type

typedef struct renderer_t {
  const char* const               name;
  renderer_entity_register_fn_t   entity_register;
  renderer_entity_unregister_fn_t entity_unregister;
  renderer_entity_update_fn_t     entity_update;
  renderer_entity_attributes_fn_t entity_attributes;
  renderer_instance_update_fn_t   instance_update;
  renderer_render_fn_t            render;
  HMap_rg                         groups;
  Shader                          shader;
} renderer_t;

void      renderer_clear_instances(renderer_t*);

void      renderer_entity_register(renderer_t*, Entity);
void      renderer_entity_update(Entity);
void      renderer_entity_unregister(Entity);

slotkey_t renderer_callback_entity_register(Entity, Game);
slotkey_t renderer_callback_entity_update(Entity);
void      renderer_callback_entity_unregister(Entity);
void*     renderer_callback_entity_attributes(Entity, bool modify);
void      renderer_callback_instance_update(render_group_t*);
void      renderer_callback_render(renderer_t*, Game);

////////////////////////////////////////////////////////////////////////////////
// Span type for defining collection of renderers for a pipeline
////////////////////////////////////////////////////////////////////////////////

#define con_type renderer_t*
#define con_prefix renderer
#include "span.h"
#undef con_prefix
#undef con_type

#endif
