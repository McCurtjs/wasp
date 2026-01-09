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
  Shader light_inst;
  Shader warhol;
} demo_shaders_t;

typedef struct demo_models_t {
  Model color_cube;
  Model gizmo;
  Model grid;
  Model box;
  Model box_inst;
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

void behavior_camera_test(Game game, entity_t* entity, float dt);
void behavior_camera_monument(Game game, entity_t* entity, float dt);
void behavior_grid_toggle(Game game, entity_t* entity, float dt);
void behavior_cubespin(Game game, entity_t* entity, float dt);
void behavior_gear_rotate_cw(Game game, entity_t* entity, float dt);
void behavior_gear_rotate_ccw(Game game, entity_t* entity, float dt);
void behavior_stare(Game game, entity_t* entity, float dt);
void behavior_attach_to_light(Game game, entity_t* entity, float dt);
void behavior_wizard_level(Game game, entity_t* entity, float dt);
void behavior_attach_to_camera_target(Game game, entity_t* entity, float dt);
void behavior_wizard(Game game, entity_t* entity, float dt);

////////////////////////////////////////////////////////////////////////////////
// Entity rendering functions
////////////////////////////////////////////////////////////////////////////////

void render_basic(Game game, entity_t* entity);
void render_debug(Game game, entity_t* entity);
void render_pbr(Game game, entity_t* entity);

void render_basic2(renderer_t* renderer, Game game, entity_t* entity);
void render_debug2(renderer_t* renderer, Game game, entity_t* entity);

slotkey_t render_pbr_register(entity_t* e, Game game);
void render_pbr_unregister(entity_t* e);
void render_pbr2(renderer_t* renderer, Game game, entity_t* entity);
void render_pbr3(renderer_t* renderer, Game game);

void render_debug3(renderer_t* renderer, Game game);

extern renderer_t* renderer_pbr;
extern renderer_t* renderer_basic;
extern renderer_t* renderer_debug;

////////////////////////////////////////////////////////////////////////////////
// Input keymap names
////////////////////////////////////////////////////////////////////////////////

enum demo_key_t {
  IN_CLOSE,

  IN_LEVEL_1,
  IN_LEVEL_2,
  IN_LEVEL_3_1,
  IN_LEVEL_3_2,
  IN_LEVEL_3_3,
  IN_LEVEL_3_4,
  IN_LEVEL_3_5,
  LEVEL_COUNT,

  IN_JUMP,
  IN_LEFT,
  IN_DOWN,
  IN_RIGHT,
  IN_KICK,
  IN_SNAP_LIGHT,
  IN_CAMERA_LOCK,

  IN_CLICK,
  IN_CLICK_MOVE,
  IN_CREATE_OBJECT,
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

scene_unload_fn_t scene_load_gears(Game game);
scene_unload_fn_t scene_load_wizard(Game game);
scene_unload_fn_t scene_load_monument(Game game);
scene_unload_fn_t scene_load_monument2(Game game);
scene_unload_fn_t scene_load_monument3(Game game);
scene_unload_fn_t scene_load_monument4(Game game);
scene_unload_fn_t scene_load_monument5(Game game);

////////////////////////////////////////////////////////////////////////////////
// Specialized event handlers
////////////////////////////////////////////////////////////////////////////////

void demo_callback_window_resize(Game game);

#endif
