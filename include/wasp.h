#ifndef WASP_H_
#define WASP_H_

#include "types.h"
#include "game.h"
#include "vec.h"

#ifdef __WASM__
# define export(fn_name) __attribute__((export_name(#fn_name))) fn_name
#else
# define export(fn_name) fn_name
#endif

void wasp_init(app_defaults_t* defaults);

bool wasp_preload(Game* game);
bool wasp_load(Game* game, int await_count, float dt);
bool wasp_update(Game* game, float dt);
void wasp_render(Game* game);

#endif
