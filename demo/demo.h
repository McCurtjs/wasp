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

#ifndef WASP_DEMO_H_
#define WASP_DEMO_H_

#include "types.h"
#include "vec.h"

#include "shader.h"
#include "model.h"
#include "material.h"
#include "render_target.h"

typedef struct demo_shaders_t {
  Shader loading;
  Shader frame;
  Shader basic;
  Shader light;
  Shader warhol;
} demo_shaders_t;

typedef struct demo_models_t {
  Model color_cube;
  Model gizmo;
  Model grid;
  Model box;
  Model gear;
  Model player;
  Model level_test;
  Model level_1;
} demo_models_t;

typedef struct demo_materials_t {
  Material grass;
  Material sands;
  Material tiles;
  Material crate;
  Material mudds;
  Material renderite;
} demo_materials_t;

typedef struct demo_t {
  vec3              target;
  vec4              light_pos;

  RenderTarget      render_target;

  demo_shaders_t    shaders;
  demo_models_t     models;
  demo_materials_t  materials;
} demo_t;

////////////////////////////////////////////////////////////////////////////////
// Use your game's header over "game.h" in order to replace the game struct's
//    inner game pointer so it's easily accessible in behaviors etc.
////////////////////////////////////////////////////////////////////////////////

#define CUSTOM_GAME_TYPE demo_t
#define CUSTOM_GAME_VAR demo
#include "game.h"

////////////////////////////////////////////////////////////////////////////////
// Entity behavior functions
////////////////////////////////////////////////////////////////////////////////

void behavior_test_camera(Game game, Entity* entity, float dt);
void behavior_grid_toggle(Game game, Entity* entity, float dt);
void behavior_cubespin(Game game, Entity* entity, float dt);
void behavior_gear_rotate_cw(Game game, Entity* entity, float dt);
void behavior_gear_rotate_ccw(Game game, Entity* entity, float dt);
void behavior_stare(Game game, Entity* entity, float dt);
void behavior_attach_to_light(Game game, Entity* entity, float dt);
void behavior_attach_to_camera_target(Game game, Entity* entity, float dt);

////////////////////////////////////////////////////////////////////////////////
// Entity rendering functions
////////////////////////////////////////////////////////////////////////////////

void render_basic(Game game, Entity* entity);
void render_debug(Game game, Entity* entity);
void render_pbr(Game game, Entity* entity);

////////////////////////////////////////////////////////////////////////////////
// Input keymap names
////////////////////////////////////////////////////////////////////////////////

enum demo_key_t {
  IN_CLOSE,

  IN_LEVEL_1,
  LEVEL_COUNT,

  IN_JUMP,
  IN_LEFT,
  IN_DOWN,
  IN_RIGHT,
  IN_KICK,
  IN_REWIND,
  IN_CAMERA_LOCK,

  // editor only
  IN_CLICK,
  IN_ROTATE_LIGHT,
  IN_SHIFT,
  IN_RELOAD,
  IN_TOGGLE_SHADER,
  IN_TOGGLE_GRID
};

////////////////////////////////////////////////////////////////////////////////
// Level loading functions
////////////////////////////////////////////////////////////////////////////////

void level_switch_check(Game game);
void level_switch(Game game, index_t scene);

void level_load_og_test(Game game);

////////////////////////////////////////////////////////////////////////////////
// Specialized event handlers
////////////////////////////////////////////////////////////////////////////////

void demo_callback_window_resize(Game game);

#endif
