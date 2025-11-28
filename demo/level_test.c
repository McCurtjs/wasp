#include "levels.h"
#include "game.h"
#include "test_behaviors.h"

void level_load_og_test(Game* game) {

  game->camera.pos = v4f(3, 2, 45, 1);
  game->camera.front = v4front;

  camera_look_at(&game->camera, game->target);

  // Debug Renderer
  game_add_entity(game, &(Entity) {
    .model = &game->models.grid,
    .transform = m4identity,
    .render = render_debug,
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

  //* Gear
  game_add_entity(game, &(Entity) {
    .model = &game->models.gear,
    .texture = game->textures.brass,
    .transform = m4translation(v3f(0, 7, -5)),
    .render = render_phong,
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

  //* LORGE Cube
  game_add_entity(game, &(Entity) {
    .model = &game->models.box,
    .texture = game->textures.tiles,
    .transform = m4mul(m4translation(v3f(0, -10.5, -5)), m4uniform(19)),
    .render = render_phong,
  }); //*/
}
