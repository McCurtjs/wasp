#include "levels.h"
#include "game.h"
#include "test_behaviors.h"

#include <math.h>

void level_load_og_test(Game* game) {

  game->camera.pos = v4f(3, 2, 45, 1);
  game->camera.front = v4front;

  camera_look_at(&game->camera, game->target);

  // Debug Renderer
  game_add_entity(game, &(Entity) {
    .model = &game->models.grid,
    .transform = m4identity,
    .render = render_debug,
    .behavior = behavior_grid_toggle,
  });

  // Camera Controller
  game_add_entity(game, &(Entity) {
    .behavior = behavior_test_camera,
  });

  //* Spinny Cube
  game_add_entity(game, &(Entity) {
    .model = &game->models.color_cube,
    .pos = v3f(-2, 0, 0),
    .angle = 0,
    .transform = m4identity,
    .render = render_basic,
    .behavior = behavior_cubespin,
  }); //*/

  //* Staring Cube
  game_add_entity(game, &(Entity) {
    .model = &game->models.color_cube,
    .pos = v3f(0, 0, 2),
    .render = render_basic,
    .behavior = behavior_stare,
  }); //*/

  //* Gizmos
  game_add_entity(game, &(Entity) {
    .model = &game->models.gizmo,
    .render = render_basic,
    .behavior = behavior_attach_to_camera_target,
  }); //*/

  game_add_entity(game, &(Entity) {
    .model = &game->models.gizmo,
    .render = render_basic,
    .behavior = behavior_attach_to_light,
  }); //*/

  //* Gear 1
  game_add_entity(game, &(Entity) {
    .model = &game->models.gear,
    .texture = game->textures.grass,
    .transform = m4translation(v3f(0, 7, -12)),
    .render = render_phong,
    .behavior = behavior_gear_rotate_cw,
  }); //*/

  //* Gear 2
  game_add_entity(game, &(Entity) {
    .model = &game->models.gear,
    .texture = game->textures.brass,
    .transform = m4translation(v3f(20.5f, -1.5f, -12)),
    .render = render_phong,
    .behavior = behavior_gear_rotate_ccw,
  }); //*/

  //* Gear 2
  game_add_entity(game, &(Entity) {
    .model = &game->models.gear,
    .texture = game->textures.brass,
    .transform = m4translation(v3f(43.f, -1.5f, -12)),
    .render = render_phong,
    .behavior = behavior_gear_rotate_cw,
  }); //*/

  //* Crate
  game_add_entity(game, &(Entity) {
    .model = &game->models.box,
    .texture = game->textures.crate,
    .transform = m4translation(v3f(0, -0.5, 0)),
    .render = render_phong,
  }); //*/

  //* Bigger Crate
  game_add_entity(game, &(Entity) {
    .model = &game->models.box,
    .texture = game->textures.crate,
    .transform = m4mul(m4translation(v3f(2, 0, 0)), m4uniform(2)),
    .render = render_phong,
  }); //*/

  //* Even Bigger Crate
  game_add_entity(game, &(Entity) {
    .model = &game->models.box,
    .texture = game->textures.crate,
    .transform = m4mul(m4translation(v3f(5, 0.5, 0)), m4uniform(3)),
    .render = render_phong,
  }); //*/

  //* LORGE Cube(s)
  for (int j = -2; j < 3; ++j) {
    for (int i = -2; i < 3; ++i) {
      float x = 23.0f * i;
      float y = 23.0f * j;
      float angle = 0.02f; // 3.0f / (x * y);
      vec3 axis = v3norm(v3f(cosf(y), 1, cosf(x)));
      vec3 pos = v3f(x, -10.5, y);
      if (!(i == 0 && i == j)) {
        angle = 0.f;
        pos.y -= v3mag(pos) / 10.f;
      }

      game_add_entity(game, &(Entity) {
        .model = &game->models.box,
        .texture = i == j ? game->textures.grass : game->textures.tiles,
        .transform =
          m4mul(
            m4mul(m4translation(pos), m4rotation(axis, angle)),
            m4uniform(19)
          ),
        .render = render_phong,
      }); //*/

    }
  }
}
