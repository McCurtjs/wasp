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

#include "graphics.h"
#include "game.h"

#include "light.h"

#define con_type light_t
#define con_prefix light
#include "slotmap.h"
#undef con_prefix
#undef con_type

#include <stdlib.h> // malloc, free
#include <string.h> // memcpy

typedef struct Graphics_Internal {
  span_renderer_t renderers;
  SlotMap_light lights;
} Graphics_Internal;

#define GRAPHICS_INTERNAL                                                     \
  Graphics_Internal* gfx = (Graphics_Internal*)_gfx;                          \
  assert(gfx)                                                                 //

////////////////////////////////////////////////////////////////////////////////
// Initialize rendering containers
////////////////////////////////////////////////////////////////////////////////

Graphics gfx_new(void) {
  Graphics_Internal* ret = malloc(sizeof(Graphics_Internal));
  assert(ret);
  *ret = (Graphics_Internal){
    .lights = smap_light_new(),
  };
  return (Graphics)ret;
}

////////////////////////////////////////////////////////////////////////////////
// Deletes and cleans up graphics components
////////////////////////////////////////////////////////////////////////////////

void gfx_delete(Graphics* gfx) {
  if (!gfx || !*gfx) return;
  Graphics_Internal* graphics = (Graphics_Internal*)*gfx;
  smap_light_delete(&graphics->lights);
  *gfx = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Iterates the list of renderers in order and executes their render functions
////////////////////////////////////////////////////////////////////////////////

void gfx_render(Graphics _gfx, Game game) {
  GRAPHICS_INTERNAL;
  renderer_t** span_foreach(renderer_ptr, gfx->renderers) {
    renderer_t* renderer = *renderer_ptr;
    if (renderer->render) {
      renderer->render(renderer, game);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Clears out instances data from all renderers
////////////////////////////////////////////////////////////////////////////////

void gfx_clear_instances(Graphics gfx) {
  assert(gfx);
  renderer_t** span_foreach(prenderer, gfx->renderers) {
    renderer_t* renderer = *prenderer;
    assert(renderer);
    if (renderer->groups) {
      render_group_t* map_foreach(group, renderer->groups) {
        pmap_clear(group->instances);
        group->needs_update = true;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Handling for lights
////////////////////////////////////////////////////////////////////////////////

#define LIGHT_INTERNAL                                                        \
  Game game = game_get_active();                                              \
  Graphics_Internal* graphics = (Graphics_Internal*)game->graphics;           \
  assert(graphics)                                                            //

////////////////////////////////////////////////////////////////////////////////

slotkey_t light_add(light_t light) {
  LIGHT_INTERNAL;
  return smap_light_insert(graphics->lights, &light);
}

////////////////////////////////////////////////////////////////////////////////

index_t light_count(void) {
  LIGHT_INTERNAL;
  return graphics->lights->size;
}

////////////////////////////////////////////////////////////////////////////////

light_t* light_ref(slotkey_t light_id) {
  LIGHT_INTERNAL;
  return smap_light_ref(graphics->lights, light_id);
}

////////////////////////////////////////////////////////////////////////////////

void light_remove(slotkey_t light_id) {
  LIGHT_INTERNAL;
  smap_light_remove(graphics->lights, light_id);
}

////////////////////////////////////////////////////////////////////////////////

void light_clear(void) {
  LIGHT_INTERNAL;
  smap_light_clear(graphics->lights);
}

////////////////////////////////////////////////////////////////////////////////
// Build a data texture representing light data for the scene
////////////////////////////////////////////////////////////////////////////////

#pragma pack(1)
typedef struct light_img_t {
  vec3 pos;
  float radius;

  vec2 dir_oct;
  float spot_outer;
  float spot_inner;

  vec3 color;
  float _unused;
} light_img_t;
#pragma pack()

////////////////////////////////////////////////////////////////////////////////

texture_t tex_from_lights(void) {
  LIGHT_INTERNAL;

  light_img_t* buffer = malloc(graphics->lights->size * sizeof(light_img_t));
  assert(buffer);

  const light_t* smap_foreach_index(light, i, graphics->lights) {
    vec2 dir = v2zero;
    if (!v3eq(light->dir, v3zero)) {
      dir = v3oct(mv4mul(game->camera.view, v34(light->dir)).xyz);
    }

    buffer[i] = (light_img_t) {
      .pos = mv4mul(game->camera.view, p34(light->pos)).xyz,
      .radius = light->radius,
      .dir_oct = dir,
      .spot_outer = light->spot_outer,
      .spot_inner = light->spot_inner,
      .color = v3scale(light->color, light->intensity),
      ._unused = 0.0f,
    };
  }

  vec2i tex_size = v2i(3, (int)graphics->lights->size);
  texture_t texture = tex_from_data(TF_RGBA_32, tex_size, buffer);
  free(buffer);
  return texture;
}
