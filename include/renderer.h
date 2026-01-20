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

typedef struct _opaque_Game_t* Game;
typedef struct renderer_t renderer_t;
typedef struct render_group_t render_group_t;

////////////////////////////////////////////////////////////////////////////////
// Callback types to define renderer functionality
////////////////////////////////////////////////////////////////////////////////

// Called once for each entity that's flagged for update
typedef void (*renderer_entity_update_fn_t)(renderer_t*, entity_t*);

// Called when entity is created/registered with the renderer
typedef slotkey_t (*renderer_entity_register_fn_t)(entity_t*, Game);

// Called when entity is deleted/unregistered from the renderer
typedef void (*renderer_entity_unregister_fn_t)(entity_t*);

// Called once for each entity before the final render function
typedef void (*renderer_entity_render_fn_t)(renderer_t*, Game, entity_t*);

// Used in the default instanced render callback to bind shader attributes
typedef void (*renderer_bind_attributes_fn_t)(Shader, render_group_t*);

// Called once per frame after the entities are processed 
typedef void (*renderer_render_fn_t)(renderer_t*, Game);

// Called when the renderer is destroyed
typedef void (*renderer_delete_fn_t)(renderer_t**);

////////////////////////////////////////////////////////////////////////////////
// Key type for distinguishing each render by model/material set
////////////////////////////////////////////////////////////////////////////////

typedef struct render_group_key_t {
  Model model;
  Material material;
  bool is_static;
} render_group_key_t;

////////////////////////////////////////////////////////////////////////////////
// Render group type representing batched draws and instance data layout
////////////////////////////////////////////////////////////////////////////////

#include "packedmap.h"

typedef struct render_group_t {
  union {
    render_group_key_t key;
    struct {
      union {
        Model model;
      };
      Material material;
      bool is_static;
    };
  };
  PackedMap instances;
  uint vao;
  uint instance_buffer;
  index_t update_range_low;
  index_t update_range_high;
  bool needs_update;
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
  renderer_entity_register_fn_t register_entity;
  renderer_entity_unregister_fn_t unregister_entity;
  renderer_entity_update_fn_t update_entity;
  renderer_entity_render_fn_t render_entity;
  renderer_bind_attributes_fn_t bind_attributes;
  renderer_render_fn_t render;
  HMap_rg groups;
  Shader shader;
  index_t instance_size;
} renderer_t;

void      renderer_entity_register(entity_t*, renderer_t*);
void      renderer_entity_unregister(entity_t*);

slotkey_t renderer_callback_register(entity_t*, Game);
void      renderer_callback_unregister(entity_t*);
void      renderer_callback_render(renderer_t*, Game);
void      renderer_callback_update(renderer_t*, entity_t*);

////////////////////////////////////////////////////////////////////////////////
// Default layout used by provided registration callbacks
////////////////////////////////////////////////////////////////////////////////

typedef struct instance_attribute_default_t {
  mat4 transform; // world-space
} instance_attribute_default_t;

////////////////////////////////////////////////////////////////////////////////
// Span type for defining collection of renderers for a pipeline
////////////////////////////////////////////////////////////////////////////////

#define con_type renderer_t*
#define con_prefix renderer
#include "span.h"
#undef con_prefix
#undef con_type

#endif
