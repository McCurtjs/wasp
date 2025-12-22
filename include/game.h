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

typedef struct game_t game_t;

typedef void (*event_resize_window_t)(game_t* game);

typedef struct app_defaults_t {
  vec2i window;
  String title;
} app_defaults_t;

typedef struct game_t {
  CUSTOM_GAME_TYPE* CUSTOM_GAME_VAR;

  vec2i window;
  String title;
  Camera camera;

  input_t input;

  Array_entity entities;

  uint level;
  bool should_exit;

  event_resize_window_t on_window_resize;

} game_t;

game_t* game_init(int window_width, int window_height);

void game_reset(game_t* game);
void game_quit(game_t* game);

void game_add_entity(game_t* game, Entity* entity);

void game_update(game_t* game, float dt);
void game_render(game_t* game);
void game_cleanup(game_t* game);

#endif
