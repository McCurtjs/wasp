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

#define WASP_ENTITY_INTERNAL
#include "game.h"

#include "quat.h"
#include "wasm.h"
#include "light.h"
#include "graphics.h"
#include "wasp.h"

#define con_type struct entity_t
#define con_prefix entity
#include "slotmap.h"
#undef con_prefix
#undef con_type

#define con_type slotkey_t
#define con_prefix id
#include "array.h"
#undef con_prefix
#undef con_type

typedef struct behavior_key_t {
  slotkey_t entity_id;
  entity_update_fn_t behavior;
} behavior_key_t;

#define con_type behavior_key_t
#define con_prefix bk
#include "array.h"
#undef con_prefix
#undef con_type

#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////
// Internal Game struct
////////////////////////////////////////////////////////////////////////////////

typedef struct Game_Internal {

  // _opaque_Game_t pub;
  void* user_game;

  // const properties
  vec2i window;
  String title;
  index_t scene;
  float scene_time;
  float frame_time;

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

  Array_bk entity_actors;   // entities with attached behaviors
  Array_id entity_updates;  // entities that have transforms to update
  Array_id entity_removals; // ids of entities to remove at end of frame

} Game_Internal;

////////////////////////////////////////////////////////////////////////////////

#define GAME_INTERNAL                             \
  Game_Internal* game = (Game_Internal*)(_game);  \
  assert(game)                                    //

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

  //smap_entity_clear(game->entities);
  smap_entity_free(game->entities);
  light_clear();
  arr_bk_clear(game->entity_actors);
  arr_id_clear(game->entity_updates);
  arr_id_clear(game->entity_removals);

  // Clear out instance data from the renderers
  gfx_clear_instances(game->graphics);
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
    .user_game = NULL,
  };

  wasp_init(&defaults);

#ifdef __WASM__
  defaults.window = v2i(x, y);
#endif

  Game game = game_new(defaults.title, defaults.window);
  game->user_game = defaults.user_game;
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
    .scene_time = 0,
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
    .next_scene = 0,
    .entities = smap_entity_new(),
    .entity_actors = arr_bk_new(),
    .entity_updates = arr_id_new(),
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
  arr_bk_delete(&game->entity_actors);
  arr_id_delete(&game->entity_removals);
  arr_id_delete(&game->entity_updates);
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
  assert(_game_instance_primary);
  return _game_instance_primary;
}

// Commented out for now because it's unused (mingw doesn't like that)
//static Game_Internal* game_get_active_internal(void) {
//  return (Game_Internal*)game_get_active();
//}

////////////////////////////////////////////////////////////////////////////////

void game_set_local(Game game) {
  _game_instance_local = game;
}

////////////////////////////////////////////////////////////////////////////////

Game game_get_local(void) {
  assert(_game_instance_local);
  return _game_instance_local;
}

static Game_Internal* game_get_local_internal(void) {
  return (Game_Internal*)game_get_local();
}

////////////////////////////////////////////////////////////////////////////////

static void _game_entity_remove_execute(Game_Internal* game, slotkey_t key) {
  entity_t* entity = smap_entity_ref(game->entities, key);

  if (entity) {
    if (entity->ondelete) {
      entity->ondelete((Game)game, entity);
    }

    // remove registrations from renderer, physics, etc queues here
    renderer_entity_unregister(entity);

    // remove from the primary list of game entities
    smap_entity_remove(game->entities, entity->id);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Per-entity update functions
////////////////////////////////////////////////////////////////////////////////

static void _game_entity_update_execute(Game_Internal* game, slotkey_t key) {
  Entity e = smap_entity_ref(game->entities, key);
  if (!e || !e->renderer || !e->is_dirty_renderer) return;

  if (e->render_id.hash) {
    if (!e->is_hidden) {
      renderer_entity_update(e);
    }
    else {
      renderer_entity_unregister(e);
    }
  }
  else if (!e->is_hidden) {
    renderer_entity_register(e->renderer, e);
  }
  // if there is no render id and it's hidden, no action needed

  e->is_dirty_renderer = false;
  e->is_dirty_static = false;
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
    game->frame_time = dt;
    game->scene_time += dt;
  }

  // Go through the list of "acting" entities with behaviors and update.
  // Clean up the actor list as we go by removing any stale keys or keys of
  //    entities that no longer have a behavior function.
  for (index_t i = 0; i < game->entity_actors->size; ) {
    behavior_key_t key = game->entity_actors->begin[i];
    entity_t* entity = smap_entity_ref(game->entities, key.entity_id);

    if (!entity || !entity->behavior || entity->behavior != key.behavior) {
      arr_bk_remove_unstable(game->entity_actors, i);
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
    _game_entity_remove_execute(game, *key);
  }
  arr_id_clear(game->entity_removals);

  // Update values of entities flagged as having changed to reflect their
  //    current state in other systems, namely, updating the transform for
  //    rendering and converting it to view space.
  arr_foreach(key, game->entity_updates) {
    _game_entity_update_execute(game, *key);
  }
  arr_id_clear(game->entity_updates);

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

  // TODO: remove, all drawable entities should have a renderer (there should be
  //    no "loop over all entities" tasks)
  entity_t* smap_foreach(entity, game->entities) {
    if (entity->render && !entity->is_hidden) {
      entity->render(_game, entity);
    }
    if (entity->renderer && entity->renderer->render_entity) {
      entity->renderer->render_entity(entity->renderer, _game, entity);
    }
  }

  gfx_render(game->graphics, _game);
}
  
////////////////////////////////////////////////////////////////////////////////
// Entity management functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Adds a game entity
////////////////////////////////////////////////////////////////////////////////

slotkey_t entity_add(const entity_desc_t* proto) {
  Game_Internal* game = game_get_local_internal();
  assert(proto);

  vec3 tint = proto->tint;
  if (tint.x == 0.0f
  &&  tint.y == 0.0f
  &&  tint.z == 0.0f
  ) {
    tint = v3ones;
  }

  slotkey_t key;
  entity_t* entity = smap_entity_emplace(game->entities, &key);

  quat rotation;
  if (memcmp(&proto->rot, &q4zero, sizeof(quat)) == 0) rotation = q4identity;
  else rotation = proto->rot;

  float scale = proto->scale ? proto->scale : 1.0f;

  *entity = (entity_t) {
    .id = key,
    .parent_id = SK_NULL,
    .name = proto->name.begin ? str_copy(proto->name) : str_empty,
    .pos = proto->pos,
    .rot = rotation,
    .scale = scale,
    .create_time = game->scene_time,
    .model = proto->model,
    .material = proto->material,
    .render = proto->render,
    .renderer = proto->renderer,
    .behavior = proto->behavior,
    .oncreate = proto->oncreate,
    .ondelete = proto->ondelete,
    .is_static = proto->is_static,
    .is_hidden = proto->is_hidden,
  };

  entity->name = proto->name.begin ? str_copy(proto->name) : str_empty;
  entity->id = key;


  // Register new entity's behavior function if it has one
  if (entity->behavior) {
    behavior_key_t bkey = { .entity_id = key, .behavior = entity->behavior };
    arr_bk_push_back(game->entity_actors, bkey);
  }

  // Register it with a renderer if it has one
  if (entity->renderer) {
    renderer_entity_register(entity->renderer, entity);
  }

  // Run on-create callback
  if (entity->oncreate) {
    entity->oncreate((Game)game, entity);
  }

  return key;
}

////////////////////////////////////////////////////////////////////////////////
// Don't delete entities right away, flag them for removal after updates.
////////////////////////////////////////////////////////////////////////////////

void entity_remove(slotkey_t id) {
  Game_Internal* game = game_get_local_internal();
  arr_id_push_back(game->entity_removals, id);
}

////////////////////////////////////////////////////////////////////////////////

index_t entity_count(void) {
  Game_Internal* game = game_get_local_internal();
  return game->entities ? game->entities->size : 0;
}

////////////////////////////////////////////////////////////////////////////////

entity_t* entity_next(slotkey_t* entity_id) {
  Game_Internal* game = game_get_local_internal();
  return smap_next((SlotMap)game->entities, entity_id);
}

////////////////////////////////////////////////////////////////////////////////

entity_t* entity_ref(slotkey_t id) {
  Game_Internal* game = game_get_local_internal();
  return smap_entity_ref(game->entities, id);
}

////////////////////////////////////////////////////////////////////////////////
// Setters for entity properties
////////////////////////////////////////////////////////////////////////////////

void entity_set_behavior(Entity entity, entity_update_fn_t behavior) {
  assert(entity);
  if (entity->behavior == behavior) return;
  entity->behavior = behavior;
  if (!behavior) return;
  Game_Internal* game = game_get_local_internal();
  behavior_key_t key = { .entity_id = entity->id, behavior };
  arr_bk_push_back(game->entity_actors, key);
}

////////////////////////////////////////////////////////////////////////////////

mat4 entity_transform(Entity entity) {
  assert(entity);
  return m4trs(entity->pos, entity->rot, entity->scale);
}

void entity_set_renderer(Entity entity, renderer_t* renderer) {
  assert(entity);
  if (!renderer) {
    renderer_entity_unregister(entity);
  }
  else {
    renderer_entity_register(renderer, entity);
  }
}

static inline void _entity_set_dirty(Entity entity) {
  if (!entity->is_dirty_renderer) {
    Game_Internal* game = game_get_local_internal();
    entity->is_dirty_renderer = true;
    arr_id_add_back(game->entity_updates, entity->id);
  }
}

void entity_set_hidden(Entity entity, bool is_hidden) {
  assert(entity);
  if (entity->is_hidden == is_hidden) return;
  entity->is_hidden = is_hidden;
  _entity_set_dirty(entity);
}

void entity_set_static(Entity entity, bool is_static) {
  assert(entity);
  if (entity->is_static == is_static) return;
  // if the value is changed multiple times, make sure the "dirty" flag always
  //    represents whether or not it's actually different from before. This is
  //    important because we need to know the previous flag in order to index
  //    into the correct render group table. This operation has to be handled
  //    in the user-provided callbacks.
  entity->is_static = is_static;
  entity->is_dirty_static = !entity->is_dirty_static;
  _entity_set_dirty(entity);
}

void entity_set_position(Entity entity, vec3 new_pos) {
  assert(entity);
  entity->pos = new_pos;
  _entity_set_dirty(entity);
}

void entity_set_rotation(Entity entity, quat new_rotation) {
  assert(entity);
  entity->rot = new_rotation;
  _entity_set_dirty(entity);
}

void entity_set_rotation_a(Entity entity, vec3 axis, float angle) {
  assert(entity);
  entity->rot = q4axis(axis, angle);
  _entity_set_dirty(entity);
}

void entity_set_scale(Entity entity, float new_scale) {
  assert(entity);
  entity->scale = new_scale;
  _entity_set_dirty(entity);
}

void entity_teleport(Entity entity, vec3 new_pos, quat new_rotation) {
  assert(entity);
  entity->pos = new_pos;
  entity->rot = new_rotation;
  _entity_set_dirty(entity);
}

void entity_teleport_a(Entity entity, vec3 new_pos, vec3 axis, float angle) {
  assert(entity);
  entity->pos = new_pos;
  entity->rot = q4axis(axis, angle);
  _entity_set_dirty(entity);
}

void entity_translate(Entity entity, vec3 dir) {
  assert(entity);
  entity->pos = v3add(entity->pos, dir);
  _entity_set_dirty(entity);
}

void entity_rotate(Entity entity, quat rotation) {
  assert(entity);
  entity->rot = q4mul(entity->rot, rotation);
  _entity_set_dirty(entity);
}

void entity_rotate_a(Entity entity, vec3 axis, float angle) {
  assert(entity);
  entity->rot = q4mul(entity->rot, q4axis(axis, angle));
  _entity_set_dirty(entity);
}


