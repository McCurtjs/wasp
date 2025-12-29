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

#define con_type entity_t
#define con_prefix entity
#include "slotmap.h"
#undef con_prefix
#undef con_type

#define con_type slotkey_t
#define con_prefix id
#include "span.h"
#include "array.h"
#undef con_prefix
#undef con_type

typedef struct Game_Internal {
  void* game;

  // const properties
  vec2i window;
  String title;
  index_t scene;
  float scene_time;

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

static Game_Internal _game_singleton;
Game game_singleton = (Game)&_game_singleton;

////////////////////////////////////////////////////////////////////////////////
// Game initialization function
////////////////////////////////////////////////////////////////////////////////

Game export(game_init) (int x, int y) {
  return game_new(str_copy("Game"), v2i(x, y));
}

////////////////////////////////////////////////////////////////////////////////

Game game_new(String title, vec2i window_size) {
  _game_singleton = (Game_Internal) {
    .window = window_size,
    .title = title,
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
    .entities = smap_entity_new(),
    .entity_actors = arr_id_new(),
    .entity_removals = arr_id_new(),
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

  // A value of -1 means "continue playing current scene"
  if (game->next_scene < 0) return;

  // Validate scene index is within count of available scenes
  index_t scene_count = span_scene_size(game->scenes);
  if (game->next_scene >= scene_count) {
    str_log("[Scene.switch] Scene index out of range: {}", game->next_scene);
    game->next_scene = -1;
    return;
  }

  // Unload the current scene and swap to the new scene (or reload)
  game_cleanup(_game);
  game_reset(_game);
  scene_load_fn_t load_scene = span_scene_get(game->scenes, game->next_scene);
  assert(load_scene);
  game->scene_unload = load_scene(_game);
  game->scene = game->next_scene;
  game->next_scene = -1;
}

////////////////////////////////////////////////////////////////////////////////
// TODO: actually reset the game instead of leaking memory?
////////////////////////////////////////////////////////////////////////////////

void game_reset(Game _game) {
  GAME_INTERNAL;
  game->scene_time = 0.0f;
  input_set(&game->input);
  smap_entity_clear(game->entities);
  camera_build(&game->camera);
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
  entity_t* entity = smap_entity_ref(game->entities, id);
  if (!entity) return;

  // TODO: remove entity from the rendering list

  if (entity->ondelete) {
    entity->ondelete(_game, entity);
  }

  smap_entity_remove(game->entities, id);
}

////////////////////////////////////////////////////////////////////////////////
// Per-frame call to update entity behaviors and clear input changes
////////////////////////////////////////////////////////////////////////////////

void game_update(Game _game, float dt) {
  GAME_INTERNAL;
  // If a scene change is requested, do that now
  if (game->next_scene >= 0) {
    _game_scene_switch(_game);
  }

  //for (index_t i = 0; i < game->entity_removals->size; ++i) {
  //  entity_id_t id = game->entity_removals->begin[i];
  //  entity_wrapper_t* e = arr_entity_ref(game->entities, (index_t)id.index);
  //  if (!e || !e->active || e->id.unique != id.unique) continue;
  //}

  game->scene_time += dt;

  for (index_t i = 0; i < game->entity_actors->size; ) {
    slotkey_t id = game->entity_actors->begin[i];
    entity_t* entity = smap_entity_ref(game->entities, id);

    if (!entity || !entity->behavior) {
      arr_id_remove_unstable(game->entity_actors, i);
      continue;
    }

    entity->behavior(_game, entity, dt);

    ++i;
  }

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

////////////////////////////////////////////////////////////////////////////////
// Clears game entity pool
////////////////////////////////////////////////////////////////////////////////

void game_cleanup(Game _game) {
  GAME_INTERNAL;

  if (game->scene_unload) {
    game->scene_unload(_game);
    game->scene_unload = NULL;
  }

  entity_t* smap_foreach(entity, game->entities) {
    if (entity->ondelete) {
      entity->ondelete(_game, entity);
    }
  }

  smap_entity_clear(game->entities);
  arr_id_clear(game->entity_actors);
  light_clear();

  game->camera.front = v4front;
  game->camera.up = v4up;
}
