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
    .material = game->materials.grass,
    .transform = m4translation(v3f(0, 7, -12)),
    .render = render_pbr,
    .behavior = behavior_gear_rotate_cw,
  }); //*/

  //* Gear 2
  game_add_entity(game, &(Entity) {
    .model = &game->models.gear,
    .material = game->materials.sands,
    .transform = m4translation(v3f(20.5f, -1.5f, -12)),
    .render = render_pbr,
    .behavior = behavior_gear_rotate_ccw,
  }); //*/

  //* Gear 3
  game_add_entity(game, &(Entity) {
    .model = &game->models.gear,
    .material = game->materials.mudds,
    .transform = m4translation(v3f(43.f, -1.5f, -12)),
    .render = render_pbr,
    .behavior = behavior_gear_rotate_cw,
  }); //*/

  //* Crate
  game_add_entity(game, &(Entity) {
    .model = &game->models.box,
    .material = game->materials.grass,
    .transform = m4translation(v3f(0, -0.5, 0)),
    .render = render_pbr,
  }); //*/

  //* Crate
  game_add_entity(game, &(Entity) {
    .model = &game->models.box,
      .material = game->materials.grass,
      .transform = m4translation(v3f(1, -0.5, 0)),
      .render = render_pbr,
  }); //*/

  //* Crate
  game_add_entity(game, &(Entity) {
    .model = &game->models.box,
      .material = game->materials.grass,
      .transform = m4translation(v3f(0, -0.5, 1)),
      .render = render_pbr,
  }); //*/

  //* Crate
  game_add_entity(game, &(Entity) {
    .model = &game->models.box,
      .material = game->materials.grass,
      .transform = m4translation(v3f(1, -0.5, 1)),
      .render = render_pbr,
  }); //*/

  //* Bigger Crate
  game_add_entity(game, &(Entity) {
    .model = &game->models.box,
    .material = game->materials.crate,
    .transform = m4mul(m4translation(v3f(2, 0, 0)), m4uniform(2)),
    .render = render_pbr,
  }); //*/

  //* Even Bigger Crate
  game_add_entity(game, &(Entity) {
    .model = &game->models.box,
    .material = game->materials.renderite,
    .tint = v3f(0.8f, 0.3f, 0.6f),
    .transform = m4mul(m4translation(v3f(5, 0.5, 0)), m4uniform(3)),
    .render = render_pbr,
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

      Material material = game->materials.tiles;

      if (i == j) {
        material = game->materials.grass;
      }
      else if (i == -j) {
        material = game->materials.sands;
      }
      else if (i % 2 == 0) {
        material = j % 2 == 0 ? game->materials.renderite : game->materials.mudds;
      }

      game_add_entity(game, &(Entity) {
        .model = &game->models.box,
        .material = material,
        .transform =
          m4mul(
            m4mul(m4translation(pos), m4rotation(axis, angle)),
            m4uniform(19)
          ),
        .render = render_pbr,
      }); //*/

    }
  }
}
