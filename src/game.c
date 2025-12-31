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
#include "light.h"
#include "graphics.h"
#include "wasp.h"

#define con_type entity_t
#define con_prefix entity
#include "slotmap.h"
#undef con_prefix
#undef con_type

#define con_type slotkey_t
#define con_prefix id
#include "array.h"
#undef con_prefix
#undef con_type

#include <stdlib.h>

typedef struct Game_Internal {
  void* game;

  // const properties
  vec2i window;
  String title;
  index_t scene;
  float scene_time;

  // systems
  Graphics graphics;

  // setup properties
  input_t input;
  span_scene_t scenes;

  // reactive properties
  camera_t camera;
  index_t next_scene;
  bool should_exit;

  // settable events
  event_resize_window_fn_t on_window_resize;

  // secrets
  scene_unload_fn_t scene_unload;
  SlotMap_entity entities;
  Array_id entity_actors;
  Array_id entity_removals;

} Game_Internal;

#define GAME_INTERNAL \
  Game_Internal* game = (Game_Internal*)(_game); \
  assert(game)

static Game _game_instance_primary = NULL;
static thread_local Game _game_instance_local = NULL;

#ifdef __WASM__
int export(canary)(int _) {
  UNUSED(_);
  slice_write = wasm_write;
  str_write("[Canary] WASM is connected!");
  return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Closing and changing the game level
////////////////////////////////////////////////////////////////////////////////

static void _game_scene_close(Game_Internal* game) {
  if (game->scene_unload) {
    game->scene_unload((Game)game);
    game->scene_unload = NULL;
  }

  // Run all remaining entity ondelete functions in case they need to clear out
  //    other resources.
  // Should this be removed? We probably don't want game-related events to
  //    trigger here, like playing sounds or affecting score or the like.
  entity_t* smap_foreach(entity, game->entities) {
    if (entity->ondelete) {
      entity->ondelete((Game)game, entity);
    }
  }

  smap_entity_clear(game->entities);
  light_clear();
  arr_id_clear(game->entity_actors);
  arr_id_clear(game->entity_removals);
}

////////////////////////////////////////////////////////////////////////////////

static void _game_scene_switch(Game_Internal* game) {

  // A value of -1 means "continue playing current scene"
  if (game->next_scene < 0) return;

  // Validate scene index is within count of available scenes
  index_t scene_count = span_scene_size(game->scenes);
  if (game->next_scene >= scene_count) {
    str_log("[Scene.switch] Scene index out of range: {}", game->next_scene);
    game->next_scene = -1;
    return;
  }

  _game_scene_close(game);

  // Reset camera to the default
  game->camera.up = v4up;
  game->camera.front = v4front;
  camera_build(&game->camera);
  game->scene_time = 0.0f;

  // Load the next scene
  scene_load_fn_t load_scene = span_scene_get(game->scenes, game->next_scene);
  assert(load_scene);
  game->scene_unload = load_scene((Game)game);
  game->scene = game->next_scene;
  game->next_scene = -1;
}

////////////////////////////////////////////////////////////////////////////////
// Game initialization function
////////////////////////////////////////////////////////////////////////////////

Game export(game_init) (int x, int y) {
  app_defaults_t defaults = (app_defaults_t) {
    .window = v2i(x, y),
    .title = str_empty,
    .game = NULL,
  };

  wasp_init(&defaults);

#ifdef __WASM__
  defaults.window = v2i(x, y);
#endif

  Game game = game_new(defaults.title, defaults.window);
  game->game = defaults.game;
  return game;
}

////////////////////////////////////////////////////////////////////////////////

Game game_new(String title, vec2i window_size) {
  Game_Internal* ret = malloc(sizeof(Game_Internal));
  assert(ret);
  *ret = (Game_Internal){
    .window = window_size,
    .title = title,
    .scene = 0,
    .scene_time = 0.0,
    .graphics = gfx_new(),
    .camera = {
      .type = CAMERA_PERSPECTIVE,
      .pos = p4origin,//v4f(0, 0, 60, 1),
      .front = v4front,
      .up = v4y,
      .perspective =
      { CAMERA_DEFAULT_FOV
      , i2aspect(window_size)
      , CAMERA_DEFAULT_NEAR
      , CAMERA_DEFAULT_FAR
      }
      //.ortho = {-6 * i2aspect(windim), 6 * i2aspect(windim), 6, -6, 0.1, 500}
    },
    .scene = 0,
    .next_scene = 0,
    .scene_time = 0,
    .entities = smap_entity_new(),
    .entity_actors = arr_id_new(),
    .entity_removals = arr_id_new(),
  };

  Game p_ret = (Game)ret;

  if (!_game_instance_primary) {
    _game_instance_primary = p_ret;
  }

  if (!_game_instance_local) {
    _game_instance_local = p_ret;
  }

  camera_build(&ret->camera);

  return p_ret;
}

////////////////////////////////////////////////////////////////////////////////
// Deletes and cleans up the game
////////////////////////////////////////////////////////////////////////////////

void game_delete(Game* _game) {
  if (!_game || !*_game) return;
  Game_Internal* game = (Game_Internal*)*_game;
  _game_scene_close(game);
  smap_entity_delete(&game->entities);
  arr_id_delete(&game->entity_actors);
  arr_id_delete(&game->entity_removals);
  if (_game_instance_primary == *_game) _game_instance_primary = NULL;
  if (_game_instance_local == *_game) _game_instance_local = NULL;
  free(game);
  *_game = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Assign or get active primary or thread-local Game object
////////////////////////////////////////////////////////////////////////////////

void game_set_active(Game game) {
  _game_instance_primary = game;
}

////////////////////////////////////////////////////////////////////////////////

Game game_get_active(void) {
  return _game_instance_primary;
}

////////////////////////////////////////////////////////////////////////////////

void game_set_local(Game game) {
  _game_instance_local = game;
}

////////////////////////////////////////////////////////////////////////////////

Game game_get_local(void) {
  return _game_instance_local;
}

////////////////////////////////////////////////////////////////////////////////
// Adds a game entity
////////////////////////////////////////////////////////////////////////////////

slotkey_t game_entity_add(Game _game, const entity_t* input_entity) {
  vec3 tint = input_entity->tint;
  GAME_INTERNAL;
  if (tint.x == 0.0f
  &&  tint.y == 0.0f
  &&  tint.z == 0.0f
  ) {
    tint = v3ones;
  }

  slotkey_t key;
  entity_t* entity = smap_entity_emplace(game->entities, &key);

  *entity = *input_entity;
  entity->id = key;
  entity->tint = tint;

  // Register new entity's behavior function if it has one
  if (entity->behavior) {
    arr_id_push_back(game->entity_actors, key);
  }

  entity->create_time = game->scene_time;

  // Register renderable
  // TODO
  // store object transform in hierarchy of render types
  // render function -> materials -> geometry
  // update the transforms buffer all at once

  // Run on-create callback
  if (entity->oncreate) {
    entity->oncreate(_game, entity);
  }

  return key;
}

////////////////////////////////////////////////////////////////////////////////

entity_t* game_entity_ref(Game _game, slotkey_t id) {
  GAME_INTERNAL;
  return smap_entity_ref(game->entities, id);
}

////////////////////////////////////////////////////////////////////////////////

void game_entity_remove(Game _game, slotkey_t id) {
  GAME_INTERNAL;
  arr_id_push_back(game->entity_removals, id);
}

static void _game_entity_remove_execute(Game _game, slotkey_t key) {
  GAME_INTERNAL;
  entity_t* entity = smap_entity_ref(game->entities, key);

  if (entity) {
    if (entity->ondelete) {
      entity->ondelete(_game, entity);
    }

    // remove from rendering, physics, etc queues here

    smap_entity_remove(game->entities, entity->id);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Per-frame call to update entity behaviors and clear input changes
////////////////////////////////////////////////////////////////////////////////

void game_update(Game _game, float dt) {
  GAME_INTERNAL;
  // If a scene change is requested, do that now
  if (game->next_scene >= 0) {
    _game_scene_switch(game);
  }
  else {
    game->scene_time += dt;
  }

  // Go through the list of "acting" entities with behaviors and update.
  // Clean up the actor list as we go by removing any stale keys or keys of
  //    entities that no longer have a behavior function.
  for (index_t i = 0; i < game->entity_actors->size; ) {
    slotkey_t key = game->entity_actors->begin[i];
    entity_t* entity = smap_entity_ref(game->entities, key);

    if (!entity || !entity->behavior) {
      arr_id_remove_unstable(game->entity_actors, i);
      continue;
    }

    entity->behavior(_game, entity, dt);
    ++i;
  }

  // Remove all the entities that were flagged for removal by either by their
  //    own behaviors or by another entity or system.
  // If an ondelete function causes more entities to be deleted, those entities
  //    should also be processed and removed on the same frame.
  slotkey_t* arr_foreach(key, game->entity_removals) {
    _game_entity_remove_execute(_game, *key);
  }
  arr_id_clear(game->entity_removals);

  // Reset button triggers (only one frame on trigger/release)
  input_update(&game->input);
}

////////////////////////////////////////////////////////////////////////////////
// Renders visisble game objects
////////////////////////////////////////////////////////////////////////////////

void game_render(Game _game) {
  GAME_INTERNAL;
  game->camera.projview = camera_projection_view(&game->camera);
  game->camera.view = camera_view(&game->camera);

  entity_t* smap_foreach(entity, game->entities) {
    if (entity->render && !entity->hidden) {
      entity->render(_game, entity);
    }
  }
}
