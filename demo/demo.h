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
#include "texture.h"

typedef struct demo_shaders_t {
  Shader loading;
  Shader frame;
  Shader pass;
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
  Model frame;
  Model gear;
  Model player;
  Model level_test;
  Model level_1;
} demo_models_t;

typedef struct demo_materials_t {
  Material atlas;
  Material grass;
  Material sands;
  Material tiles;
  Material crate;
  Material mudds;
  Material renderite;
} demo_materials_t;

typedef struct demo_t {
  vec3              target;
  vec3              light_pos;
  int               monument_extent;
  int               monument_size;

  RenderTarget      render_target;
  RenderTarget      render_target_min;

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
// Shared entity behavior functions
////////////////////////////////////////////////////////////////////////////////

void behavior_grid_toggle(Game game, entity_t* entity, float dt);

////////////////////////////////////////////////////////////////////////////////
// Entity rendering functions
////////////////////////////////////////////////////////////////////////////////

void render_basic(Game game, entity_t* entity);
void render_debug(Game game, entity_t* entity);
void render_pbr(Game game, entity_t* entity);
 
extern renderer_t* renderer_pbr;
extern renderer_t* renderer_basic;

////////////////////////////////////////////////////////////////////////////////
// Input keymap names
////////////////////////////////////////////////////////////////////////////////

enum demo_key_t {
  IN_CLOSE,

  IN_LEVEL_1,
  IN_LEVEL_2,
  IN_LEVEL_3,
  LEVEL_COUNT,

  IN_JUMP,
  IN_LEFT,
  IN_DOWN,
  IN_RIGHT,
  IN_KICK,
  IN_SNAP_LIGHT,

  IN_CLICK,
  IN_CLICK_MOVE,
  IN_CREATE_OBJECT,
  IN_DELETE_OBJECT,
  IN_ROTATE_LIGHT,
  IN_CAM_DRAG,
  IN_CAM_ROTATE,
  IN_CAM_CENTER,
  IN_SHIFT,
  IN_RELOAD,
  IN_TOGGLE_SHADER,
  IN_TOGGLE_GRID,
  IN_TOGGLE_LOCK,
  IN_TOGGLE_UI,
  IN_INCREASE,
  IN_DECREASE,
  IN_INCREASE_FAST,
  IN_DECREASE_FAST
};

////////////////////////////////////////////////////////////////////////////////
// Level loading functions
////////////////////////////////////////////////////////////////////////////////

scene_unload_fn_t scene_load_gears(Game game);
scene_unload_fn_t scene_load_wizard(Game game);
scene_unload_fn_t scene_load_monument(Game game);

////////////////////////////////////////////////////////////////////////////////
// Specialized event handlers
////////////////////////////////////////////////////////////////////////////////

void demo_callback_window_resize(Game game);

#endif
