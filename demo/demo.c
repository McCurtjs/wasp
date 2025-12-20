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
static File file_model_gear = NULL;
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

static int active_shader = 1;

static Model model_frame = { .type = MODEL_FRAME };// { .type = MODEL_FRAME };

static keybind_t input_map[] = {
  { .name = IN_CLOSE, .key = SDLK_ESCAPE },
  { .name = IN_JUMP, .key = 'w' },
  { .name = IN_LEFT, .key = 'a' },
  { .name = IN_DOWN, .key = 's' },
  { .name = IN_RIGHT, .key = 'd' },
  { .name = IN_KICK, .key = SDLK_LEFT },
  { .name = IN_REWIND, .key = 'r' },
  { .name = IN_CAMERA_LOCK, .key = 'c' },
  { .name = IN_CLICK, .key = SDL_BUTTON_LEFT, .mouse = true },
  { .name = IN_ROTATE_LIGHT, .key = SDL_BUTTON_RIGHT, .mouse = true },
  { .name = IN_ROTATE_LIGHT, .key = 'e' },
  { .name = IN_SHIFT, .key = 16 },
  { .name = IN_RELOAD, .key = 'p' },
  { .name = IN_LEVEL_1, .key = '1' },
  { .name = IN_TOGGLE_SHADER, .key = 'm' },
  { .name = IN_TOGGLE_GRID, .key = 'g' },
};

void wasp_init(app_defaults_t* game) {
  game->window = v2i(1024, 768);
  game->title = str_copy("WASP Demo");
}

bool export(wasp_preload) (Game* game) {
  #if GAME_ON == 1

  file_model_gear = file_new(S("./res/models/gear.obj"));

  game->materials.grass = material_new(S("grass_rocky"));
  game->materials.grass->use_normal_map = true;
  game->materials.grass->use_specular_map = true;
  game->materials.grass->roughness = 1.0f;

  game->materials.crate = material_new(S("crate"));
  game->materials.crate->roughness = 0.2f;

  game->materials.tiles = material_new(S("tiles"));

  game->materials.sands = material_new(S("brass2"));
  game->materials.sands->roughness = 1.0f;
  game->materials.sands->use_normal_map = true;

  game->materials.mudds = material_new(S("lava"));
  game->materials.mudds->use_normal_map = true;
  game->materials.mudds->roughness = 1.0f;

  game->materials.renderite = material_new(S("renderite"));
  game->materials.renderite->use_diffuse_map = false;
  game->materials.renderite->roughness = 0.0f;
  game->materials.renderite->metalness = 0.4f;

  material_load_all_async();

  #endif

  camera_build_perspective(&game->camera);
  //camera_build_orthographic(&game.camera);

  // load this first since it's the loading screen spinner
  game->models.color_cube.type = MODEL_CUBE_COLOR;
  model_build(&game->models.color_cube);
  model_build(&model_frame);

  game->shaders.loading = shader_new(S("basic"));
  game->shaders.basic = shader_new_load_async(S("basic_deferred"));
  game->shaders.light = shader_new_load_async(S("light"));
  game->shaders.frame = shader_new_load_async(S("frame"));
  game->shaders.warhol = shader_new_load_async(S("warhol"));

  game->textures.render_target = rt_new(
    TF_RGB_8, TF_RG_16, TF_RGB_10_A_2, TF_DEPTH_32
  );
  rt_build(game->textures.render_target, game->window);

  game->input.keymap = span_keymap(input_map, ARRAY_COUNT(input_map));

  return true;
}

static void cheesy_loading_animation(Game* game, float dt) {
  static float cubespin = 0;

  rt_bind_default();

  mat4 projview = camera_projection_view(&game->camera);

  shader_bind(game->shaders.loading);
  int loc_pvm = shader_uniform_loc(game->shaders.loading, "in_pvm_matrix");

  mat4 model = m4translation(v3f(0, 0, 0));
  model = m4mul(model, m4rotation(v3norm(v3f(1.f, 1.5f, -.7f)), cubespin));
  model = m4mul(model, m4rotation(v3norm(v3f(-4.f, 1.5f, 1.f)), cubespin/3.6f));

  glUniformMatrix4fv(loc_pvm, 1, 0, m4mul(projview, model).f);
  model_render(&game->models.color_cube);
  cubespin += 2 * dt;

  GLenum err = glGetError();
  if (err) {
    str_log("[Demo.load_anim] Error: 0x{!x}", err);
  }
}

bool export(wasp_load) (Game* game, int await_count, float dt) {

  if (await_count || !GAME_ON) {
    cheesy_loading_animation(game, dt);
    return 0;
  };

  str_write("Async load finished");

  #if GAME_ON == 1

  shader_build_all();
  material_build_all();
  model_load_obj(&game->models.gear, file_model_gear);

  // Delete async loaded resources
  file_delete(&file_model_gear);

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

  if (input_triggered(game, IN_TOGGLE_SHADER)) {
    active_shader = (active_shader + 1) % 2;
  }

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
  Shader shader = game->shaders.warhol;
  if (active_shader == 1) {
    shader = game->shaders.frame;
  }

  shader_bind(shader);
  int tex_sampler = shader_uniform_loc(shader, "texSamp");
  int norm_sampler = shader_uniform_loc(shader, "normSamp");
  int prop_sampler = shader_uniform_loc(shader, "propSamp");
  int depth_sampler = shader_uniform_loc(shader, "depthSamp");
  int loc_invproj = shader_uniform_loc(shader, "in_proj_inverse");
  int loc_light_pos = shader_uniform_loc(shader, "lightPos");
  tex_apply(game->textures.render_target->textures[0], 0, tex_sampler);
  tex_apply(game->textures.render_target->textures[1], 1, norm_sampler);
  tex_apply(game->textures.render_target->textures[2], 2, prop_sampler);
  tex_apply(game->textures.render_target->textures[3], 3, depth_sampler);
  glUniformMatrix4fv(loc_invproj, 1, 0, m4inverse(game->camera.projection).f);
  glUniform4fv(loc_light_pos, 1, mv4mul(game->camera.view, game->light_pos).f);

  model_render(&model_frame);
}
  