#include "levels.h"

#include "game.h"
#include "input_map.h"

void level_switch_check(Game* game) {
  if (input_triggered(game, IN_RELOAD)) {
    level_switch(game, game->level);
    return;
  }

  for (uint i = 0; i < LEVEL_COUNT; ++i) {
    if (input_triggered(game, IN_LEVEL_1)) {
      game->level = i;
      level_switch(game, i);
      return;
    }
  }
}

void level_switch(Game* game, uint level) {
  if (level >= LEVEL_COUNT) return;
  if (!game_levels[level]) return;
  game_cleanup(game);
  game_reset(game);
  game_levels[level](game);
  game->level = level;
}

LoadLevelFn game_levels[LEVEL_COUNT] = {
  level_load_og_test,
};
