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

#ifndef WASP_GAME_H_
#define WASP_GAME_H_

#ifndef CUSTOM_GAME_TYPE
#define CUSTOM_GAME_TYPE void
#endif

#ifndef CUSTOM_GAME_VAR
#define CUSTOM_GAME_VAR game
#endif

#include "vec.h"
#include "camera.h"
#include "entity.h"
#include "input.h"

typedef struct _opaque_Game_t* Game;

typedef void (*event_resize_window_fn_t)(Game game);

typedef void (*scene_load_fn_t)(Game game);

#define con_type scene_load_fn_t
#define con_prefix scene
#include "span.h"
#undef con_type
#undef con_name

typedef struct app_defaults_t {
  vec2i window;
  String title;
} app_defaults_t;

typedef struct _opaque_Game_t {
  CUSTOM_GAME_TYPE* CUSTOM_GAME_VAR;

  // const properties
  const vec2i         window;
  const String        title;
  const Array_entity  entities;
  const index_t       scene;

  // game setup
  input_t       input;
  span_scene_t  scenes;

  // reactive properties
  camera_t  camera;
  index_t next_scene;
  bool    should_exit;

  // settable events
  event_resize_window_fn_t on_window_resize;
}* Game;

Game game_init(int window_width, int window_height);
Game game_new(String title, vec2i window_size);

void game_reset(Game game);

void game_add_entity(Game game, Entity* entity);

void game_update(Game game, float dt);
void game_render(Game game);
void game_cleanup(Game game);

#endif
