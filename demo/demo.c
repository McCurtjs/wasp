#include "SDL3/SDL.h"

#include "wasm.h"
#include "shader.h"
#include "mat.h"
#include "model.h"
#include "camera.h"
#include "texture.h"
#include "draw.h"
#include "str.h"
#include "utility.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "gl.h"

#include "game.h"
#include "system_events.h"
#include "test_behaviors.h"
#include "render_target.h"

#include "levels.h"
#include "input_map.h"

#include "wasp.h"

//#define SDL_MAIN_USE_CALLBACKS
//#include "SDL3/SDL_main.h"

#define GAME_ON 1

#if GAME_ON == 1
// Async loaders
//static File file_model_test = NULL;
//static File file_model_level_1;
static File file_model_gear = NULL;
static Image image_crate;
//static Image image_level;
static Image image_tiles;
static Image image_brass;
//static Image image_anim_test;
#endif

#ifdef __clang__
# pragma clang diagnostic ignored "-Wconstant-logical-operand"
#endif

#ifdef __WASM__
int export(canary) (int _) {
  UNUSED(_);
  slice_write = wasm_write;
  str_write("WASM is connected!");
  return 0;
}
#endif

static Model model_frame = { .type = MODEL_FRAME };// { .type = MODEL_FRAME };

static keybind_t input_map[] = {
  { .name = IN_JUMP, .key = 'w' },
  { .name = IN_LEFT, .key = 'a' },
  { .name = IN_DOWN, .key = 's' },
  { .name = IN_RIGHT, .key = 'd' },
  { .name = IN_KICK, .key = SDLK_LEFT },
  { .name = IN_REWIND, .key = 'd' },
  { .name = IN_CAMERA_LOCK, .key = 'c' },
  { .name = IN_CLICK, .key = SDL_BUTTON_LEFT, .mouse = true },
  { .name = IN_ROTATE_LIGHT, .key = SDL_BUTTON_RIGHT, .mouse = true },
  { .name = IN_SHIFT, .key = 16 },
  { .name = IN_RELOAD, .key = 'p' },
  { .name = IN_LEVEL_1, .key = '1' },
};

void wasp_init(app_defaults_t* game) {
  game->window = v2i(1024, 768);
  game->title = "WASP Demo";
}

bool export(wasp_preload) (Game* game) {
  #if GAME_ON == 1
  //file_model_test = file_new(S("./res/models/test.obj"));
  file_model_gear = file_new(S("./res/models/gear.obj"));
  //file_open_async(&file_model_level_1, "./res/models/level_1.obj");

  image_crate = img_load(S("./res/textures/crate.png"));
  image_brass = img_load(S("./res/textures/brass.jpg"));
  image_tiles = img_load(S("./res/textures/tiles.png"));
  #endif

  camera_build_perspective(&game->camera);
  //camera_build_orthographic(&game.camera);

  // load this first since it's the loading screen spinner
  game->models.color_cube.type = MODEL_CUBE_COLOR;
  model_build(&game->models.color_cube);
  model_build(&model_frame);

  game->shaders.basic = shader_new(S("basic"));
  game->shaders.light = shader_new_load(S("light"));
  game->shaders.frame = shader_new_load(S("frame"));
  game->shaders.warhol = shader_new_load(S("warhol"));

  game->textures.render_target = rt_new(2, (texture_format_t[]) {
    TF_RGBA_8, TF_RGBA_8
  });
  rt_build(game->textures.render_target, game->window);

  game->inputs = span_keymap(input_map, ARRAY_COUNT(input_map));

  return true;
}

static void cheesy_loading_animation(Game* game, float dt) {
  static float cubespin = 0;

  rt_bind_default();

  mat4 projview = camera_projection_view(&game->camera);

  shader_bind(game->shaders.basic);
  int loc_pvm = shader_uniform_loc(game->shaders.basic, "projViewMod");

  mat4 model = m4translation(v3f(0, 0, 0));
  model = m4mul(model, m4rotation(v3norm(v3f(1.f, 1.5f, -.7f)), cubespin));
  model = m4mul(model, m4rotation(v3norm(v3f(-4.f, 1.5f, 1.f)), cubespin/3.6f));

  glUniformMatrix4fv(loc_pvm, 1, 0, m4mul(projview, model).f);
  model_render(&game->models.color_cube);
  cubespin += 2 * dt;
}

bool export(wasp_load) (Game* game, int await_count, float dt) {

  if (await_count || !GAME_ON) {
    cheesy_loading_animation(game, dt);
    return 0;
  };

  str_write("Async load finished");

  #if GAME_ON == 1

  shader_build(game->shaders.light);
  shader_build(game->shaders.frame);
  shader_build(game->shaders.warhol);

  //model_load_obj(&game.models.level_test, file_model_test);
  model_load_obj(&game->models.gear, file_model_gear);
  //model_load_obj(&game.models.level_1, &file_model_level_1);

  // Build textures from async data
  game->textures.crate = tex_from_image(image_crate);
  game->textures.tiles = tex_from_image(image_tiles);
  game->textures.brass = tex_from_image(image_brass);
  //texture_build_from_image(&game.textures.level, &image_level);
  //texture_build_from_image(&game.textures.player, &image_anim_test);

  // Delete async loaded resources
  //file_delete(&file_model_test);
  file_delete(&file_model_gear);
  //file_delete(&file_model_level_1);
  //image_delete(&image_level);
  img_delete(&image_brass);
  img_delete(&image_crate);
  img_delete(&image_tiles);
  //image_delete(&image_anim_test);

  // Set up game models
  game->models.grid.grid = (Model_Grid) {
    .type = MODEL_GRID,
    .basis = {v3x, v3y, v3z},
    .primary = {0, 2},
    .extent = 100
  };

  game->models.player.sprites = (Model_Sprites) {
    .type = MODEL_SPRITES,
    .grid = {.w = 16, .h = 16},
  };

  model_build(&game->models.player);
  model_build(&game->models.level_test);
  //model_build(&game.models.level_1);
  model_build(&game->models.gear);
  model_grid_set_default(&game->models.gizmo, -2);
  game->models.box.type = MODEL_CUBE;
  model_build(&game->models.box);
  model_build(&game->models.grid);
  model_build(&game->models.gizmo);

  // Load the first game level
  level_switch(game, game->level);

  #endif

  return 1;
}

bool export(wasp_update) (Game* game, float dt) {
  UNUSED(dt);
  //process_system_events(&game);
  level_switch_check(game);
  game_update(game, dt);

  return true;
}

void export(wasp_render) (Game* game) {
  rt_bind(game->textures.render_target);
  game_render(game);

  rt_bind_default();
  /*
  shader_program_use(&shader_frame);
  tex_apply(game.textures.render_target->textures[0], 0, 0);
  /*/
  shader_bind(game->shaders.warhol);
  int tex_sampler = shader_uniform_loc(game->shaders.warhol, "texSamp");
  int norm_sampler = shader_uniform_loc(game->shaders.warhol, "normSamp");
  tex_apply(game->textures.render_target->textures[0], 0, tex_sampler);
  tex_apply(game->textures.render_target->textures[1], 1, norm_sampler);
  //*/
  model_render(&model_frame);
}
