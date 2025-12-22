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

#include "game.h"

#include "wasm.h"

typedef struct Game_Internal {
  void* game;

  // const properties
  vec2i window;
  String title;
  Array_entity entities;
  index_t scene;

  // setup properties
  input_t input;
  span_scene_t scenes;


  // reactive properties
  Camera camera;
  index_t next_scene;
  bool should_exit;

  // settable events
  event_resize_window_fn_t on_window_resize;

} Game_Internal;

#define GAME_INTERNAL \
  Game_Internal* game = (Game_Internal*)(_game); \
  assert(game)

static Game_Internal _game_singleton;
Game game_singleton = (Game)&_game_singleton;

////////////////////////////////////////////////////////////////////////////////
// Game initialization function
////////////////////////////////////////////////////////////////////////////////

Game export(game_init) (int x, int y) {
  return game_new(str_copy("Game"), v2i(x, y));
}

Game game_new(String title, vec2i window_size) {
  _game_singleton = (Game_Internal) {
    .window = window_size,
    .title = title,
    .camera = {
      .pos = v4f(0, 0, 60, 1),
      .front = v4front,
      .up = v4y,
      .persp = {d2r(60), i2aspect(window_size), 0.1f, 500}
      //.ortho = {-6 * i2aspect(windim), 6 * i2aspect(windim), 6, -6, 0.1, 500}
    },
    .entities = arr_entity_new(),
    .scene = 0,
    .next_scene = 0,
  };

  game_singleton = (Game)&_game_singleton;

  game_reset(game_singleton);

  return game_singleton;
}

////////////////////////////////////////////////////////////////////////////////
// Changes the game level
////////////////////////////////////////////////////////////////////////////////

static void _game_scene_switch(Game _game) {
  GAME_INTERNAL;
  if (game->next_scene < 0) return;

  index_t scene_count = span_scene_size(game->scenes);
  if (game->next_scene >= scene_count) {
    str_log("[Scene.switch] Scene index out of range: {}", game->next_scene);
    game->next_scene = -1;
    return;
  }

  game_cleanup(_game);
  game_reset(_game);
  scene_load_fn_t load_scene = span_scene_get(game->scenes, game->next_scene);
  assert(load_scene);
  load_scene(_game);
  game->scene = game->next_scene;
  game->next_scene = -1;
}

////////////////////////////////////////////////////////////////////////////////
// TODO: actually reset the game instead of leaking memory?
////////////////////////////////////////////////////////////////////////////////

void game_reset(Game game) {
  input_set(&game->input);
  arr_entity_clear(game->entities);
}

////////////////////////////////////////////////////////////////////////////////
// Adds a game entity
////////////////////////////////////////////////////////////////////////////////

void game_add_entity(Game game, Entity* entity) {
  if (entity->tint.x == 0.0f
  &&  entity->tint.y == 0.0f
  &&  entity->tint.z == 0.0f
  ) {
    entity->tint = v3ones;
  }
  arr_entity_write_back(game->entities, entity);
}

////////////////////////////////////////////////////////////////////////////////
// Per-frame call to update entity behaviors and clear input changes
////////////////////////////////////////////////////////////////////////////////

void game_update(Game game, float dt) {
  // If a scene change is requested, do that now
  if (game->next_scene >= 0) {
    _game_scene_switch(game);
  }

  // Update all entities by their behavior function
  Entity* arr_foreach(entity, game->entities) {
    if (entity->behavior) {
      entity->behavior(game, entity, dt);
    }
  }

  // Reset button triggers (only one frame on trigger/release)
  input_update(&game->input);
}

////////////////////////////////////////////////////////////////////////////////
// Renders visisble game objects
////////////////////////////////////////////////////////////////////////////////

void game_render(Game game) {
  game->camera.projview = camera_projection_view(&game->camera);
  game->camera.view = camera_view(&game->camera);

  Entity* arr_foreach(entity, game->entities) {
    if (entity->render && !entity->hidden) {
      entity->render(game, entity);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Clears game entity pool
////////////////////////////////////////////////////////////////////////////////

void game_cleanup(Game game) {
  Entity* arr_foreach(entity, game->entities) {
    if (entity->delete) {
      entity->delete(game, entity);
    }
  }

  arr_entity_clear(game->entities);
}
