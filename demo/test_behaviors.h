#ifndef _TEST_BEHAVIORS_H_
#define _TEST_BEHAVIORS_H_

#include "entity.h"
#include "game.h"

void behavior_test_camera(Entity* entity, Game* game, float dt);
void behavior_grid_toggle(Entity* entity, Game* game, float dt);
void behavior_cubespin(Entity* entity, Game* game, float dt);
void behavior_gear_rotate_cw(Entity* entity, Game* game, float dt);
void behavior_gear_rotate_ccw(Entity* entity, Game* game, float dt);
void behavior_stare(Entity* entity, Game* game, float dt);
void behavior_attach_to_light(Entity* entity, Game* game, float dt);
void behavior_attach_to_camera_target(Entity* entity, Game* game, float dt);

void render_basic(Entity* entity, Game* game);
void render_debug(Entity* entity, Game* game);
void render_phong(Entity* entity, Game* game);
void render_normal(Entity* entity, Game* game);

#endif
