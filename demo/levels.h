#ifndef _LEVEL_H_
#define _LEVEL_H_

#include "types.h"
#include "input_map.h"

typedef struct Game Game;

void level_switch_check(Game* game);
void level_switch(Game* game, uint level);

typedef void (*LoadLevelFn)(Game* game);

void level_load_og_test(Game* game);

extern LoadLevelFn game_levels[LEVEL_COUNT];

#endif
