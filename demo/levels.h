#ifndef _LEVEL_H_
#define _LEVEL_H_

#include "types.h"

typedef struct Game Game;

void level_switch_check(Game* game);
void level_switch(Game* game, uint level);

typedef void (*LoadLevelFn)(Game* game);

void level_load_test(Game* game);
void level_load_level_1(Game* game);
void level_load_level_2(Game* game);
void level_load_level_3(Game* game);

void level_load_editor(Game* game);
void level_load_editor_test(Game* game);
void level_load_og_test(Game* game);

#define LEVEL_COUNT 1

extern LoadLevelFn game_levels[LEVEL_COUNT];

#endif
