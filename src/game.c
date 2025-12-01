#include "game.h"

#include "wasm.h"

Game game_singleton;

Game* export(game_init) (int x, int y) {
  vec2i default_window = v2i(x, y);
  game_singleton = (Game){
    .window = default_window,
    .title = "Game Title",
    .camera = {
      .pos = v4f(0, 0, 60, 1),
      .front = v4front,
      .up = v4y,
      .persp = {d2r(60), i2aspect(default_window), 0.1f, 500}
      //.ortho = {-6 * i2aspect(windim), 6 * i2aspect(windim), 6, -6, 0.1, 500}
    },
    .target = v3origin,
    .light_pos = v4f(4, 3, 5, 1),
    .input.mapping.keys = {'w', 's', 'a', 'd', 'c', 'r',
    /* // Attack button, useful on F for testing
    'f',
    /*/
    0x40000050u, // SDLK_LEFT, // kick button
    //*/
    16, // shift keky, for editor
    'p', // restart level
    // Level Select
    '1',
  },
  .level = 0,
  };

  game_reset(&game_singleton);

  return &game_singleton;
}

void game_reset(Game* game) {
  game->entities = arr_entity_new();
}

void game_add_entity(Game* game, const Entity* entity) {
  arr_entity_write_back(game->entities, entity);
}

void game_update(Game* game, float dt) {
  Entity* arr_foreach(entity, game->entities) {
    if (entity->behavior) {
      entity->behavior(entity, game, dt);
    }
  }

  // reset button triggers (only one frame on trigger)
  for (int i = 0; i < game_button_input_count; ++i) {
    game->input.triggered.buttons[i] = FALSE;
    game->input.released.buttons[i] = FALSE;
  }

  game->input.mouse.move = v2zero;
}

void game_render(Game* game) {
  game->camera.projview = camera_projection_view(&game->camera);

  Entity* arr_foreach(entity, game->entities) {
    if (entity->render && !entity->hidden) {
      entity->render(entity, game);
    }
  }
}

void game_cleanup(Game* game) {
  Entity* arr_foreach(entity, game->entities) {
    if (entity->delete) {
      entity->delete(entity);
    }
  }

  arr_entity_delete(&game->entities);
}
