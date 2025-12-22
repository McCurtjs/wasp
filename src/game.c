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

game_t game_singleton;

////////////////////////////////////////////////////////////////////////////////
// Game initialization function
////////////////////////////////////////////////////////////////////////////////

game_t* export(game_init) (int x, int y) {
  vec2i default_window = v2i(x, y);
  game_singleton = (game_t) {
    .window = default_window,
    .title = NULL,
    .camera = {
      .pos = v4f(0, 0, 60, 1),
      .front = v4front,
      .up = v4y,
      .persp = {d2r(60), i2aspect(default_window), 0.1f, 500}
      //.ortho = {-6 * i2aspect(windim), 6 * i2aspect(windim), 6, -6, 0.1, 500}
    },
    .level = 0,
  };

  game_reset(&game_singleton);

  return &game_singleton;
}

////////////////////////////////////////////////////////////////////////////////
// TODO: actually reset the game instead of leaking memory?
////////////////////////////////////////////////////////////////////////////////

void game_reset(game_t* game) {
  input_set(&game->input);
  game->entities = arr_entity_new();
}

////////////////////////////////////////////////////////////////////////////////
// Close the game successfully at end of current frame
////////////////////////////////////////////////////////////////////////////////

void game_quit(game_t* game) {
  input_unset(&game->input);
  game->should_exit = true;
}

////////////////////////////////////////////////////////////////////////////////
// Adds a game entity
////////////////////////////////////////////////////////////////////////////////

void game_add_entity(game_t* game, Entity* entity) {
  if (entity->tint.x == 0.0f
  &&  entity->tint.y == 0.0f
  &&  entity->tint.z == 0.0f) {
    entity->tint = v3ones;
  }
  arr_entity_write_back(game->entities, entity);
}

////////////////////////////////////////////////////////////////////////////////
// Per-frame call to update entity behaviors and clear input changes
////////////////////////////////////////////////////////////////////////////////

void game_update(game_t* game, float dt) {
  // Reset button triggers (only one frame on trigger/release)
  Entity* arr_foreach(entity, game->entities) {
    if (entity->behavior) {
      entity->behavior(entity, game, dt);
    }
  }

  input_update(&game->input);
}

////////////////////////////////////////////////////////////////////////////////
// Renders visisble game objects
////////////////////////////////////////////////////////////////////////////////

void game_render(game_t* game) {
  game->camera.projview = camera_projection_view(&game->camera);
  game->camera.view = camera_view(&game->camera);

  Entity* arr_foreach(entity, game->entities) {
    if (entity->render && !entity->hidden) {
      entity->render(entity, game);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Clears game entity pool
////////////////////////////////////////////////////////////////////////////////

void game_cleanup(game_t* game) {
  Entity* arr_foreach(entity, game->entities) {
    if (entity->delete) {
      entity->delete(entity);
    }
  }

  arr_entity_delete(&game->entities);
}
