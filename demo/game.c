#include "game.h"

#include "test_behaviors.h"

#include "wasm.h"

void game_init(Game* game) {
  game->entities = arr_ety_new();
}

void game_add_entity(Game* game, const Entity* entity) {
  arr_ety_write_back(game->entities, entity);
}

void game_update(Game* game, float dt) {

  Entity* array_foreach(entity, game->entities) {
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

  Entity* array_foreach(entity, game->entities) {
    if (entity->render && !entity->hidden) {
      entity->render(entity, game);
    }
  }
}

void game_cleanup(Game* game) {
  Entity* array_foreach(entity, game->entities) {
    if (entity->delete) {
      entity->delete(entity);
    }
  }

  arr_ety_delete(&game->entities);
}
