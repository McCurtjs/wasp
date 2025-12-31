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

#include "graphics.h"
#include "game.h"

#include "light.h"

#define con_type light_t
#define con_prefix light
#define con_view_type view_light_t
#include "packedmap.h"
#undef con_view_type
#undef con_prefix
#undef con_type

#include <stdlib.h> // malloc, free
#include <string.h> // memcpy

typedef struct Graphics_Internal {
  PackedMap_light lights;
} Graphics_Internal;

#define GRAPHICS_INTERNAL                                                     \
  Game game = game_get_active();                                              \
  Graphics_Internal* graphics = (Graphics_Internal*)game->graphics;           \
  assert(graphics)                                                            //

Graphics gfx_new(void) {
  Graphics_Internal* ret = malloc(sizeof(Graphics_Internal));
  assert(ret);
  *ret = (Graphics_Internal){
    .lights = pmap_light_new(),
  };
  return ret;
}

void gfx_delete(Graphics* gfx) {
  if (!gfx || !*gfx) return;
  Graphics_Internal* graphics = (Graphics_Internal*)*gfx;
  pmap_light_delete(&graphics->lights);
  *gfx = NULL;
}

slotkey_t light_add(light_t light) {
  GRAPHICS_INTERNAL;
  return pmap_light_insert(graphics->lights, &light);
}

light_t* light_ref(slotkey_t light_id) {
  GRAPHICS_INTERNAL;
  return pmap_light_ref(graphics->lights, light_id);
}

void light_remove(slotkey_t light_id) {
  GRAPHICS_INTERNAL;
  pmap_light_remove(graphics->lights, light_id);
}

void light_clear(void) {
  GRAPHICS_INTERNAL;
  pmap_light_clear(graphics->lights);
}

view_light_t light_buffer(void) {
  GRAPHICS_INTERNAL;
  return graphics->lights->view;
}

texture_t tex_from_lights(void) {
  GRAPHICS_INTERNAL;
  light_t* buffer = malloc(graphics->lights->size * sizeof(light_t));
  assert(buffer);
  memcpy(buffer, graphics->lights->begin, graphics->lights->size_bytes);

  // transform all light positions from world to view space
  for (index_t i = 0; i < graphics->lights->size; ++i) {
    buffer[i].pos = mv4mul(game->camera.view, p34(buffer[i].pos)).xyz;
  }

  vec2i tex_size = v2i((int)graphics->lights->size * 3, 1);
  texture_t texture = tex_from_data(TF_RGB_32, tex_size, buffer);
  free(buffer);
  return texture;
}
