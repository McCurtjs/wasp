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

static Game game;

#define GAME_ON 0

#if GAME_ON == 1
// Async loaders
static File file_vert;
static File file_frag;
static File file_model_test;
//static File file_model_level_1;
static File file_model_gear;
static Image image_crate;
//static Image image_level;
static Image image_tiles;
static Image image_brass;
static Image image_anim_test;
#endif

int export(canary) (int _) {
  PARAM_UNUSED(_);
  print("WASM is connected!");
  return 0;
}

void export(wasm_preload) (uint w, uint h) {
  #if GAME_ON == 1
  file_open_async(&file_vert, "./res/shaders/basic.vert");
  file_open_async(&file_frag, "./res/shaders/basic.frag");
  file_open_async(&file_model_test, "./res/models/test.obj");
  //file_open_async(&file_model_level_1, "./res/models/level_1.obj");
  file_open_async(&file_model_gear, "./res/models/gear.obj");

  image_open_async(&image_crate, "./res/textures/crate.png");
  //image_open_async(&image_level, "./res/textures/levels.jpg");
  image_open_async(&image_brass, "./res/textures/brass.jpg");
  image_open_async(&image_tiles, "./res/textures/tiles.png");
  image_open_async(&image_anim_test, "./res/textures/spritesheet.png");
  #endif

  vec2i windim = v2i(w, h);
  game = (Game) {
    .window = windim,
    .camera = {
      .pos = v4f(0, 0, 60, 1),
      .front = v4front,
      .up = v4y,
      .persp = {d2r(20), i2aspect(windim), 0.1f, 500}
      //.ortho = {-6 * i2aspect(windim), 6 * i2aspect(windim), 6, -6, 0.1, 500}
    },
    .target = v3zero,
    .light_pos = v4f(4, 3, 5, 1),
    .input.mapping.keys = {'w', 's', 'a', 'd', 'c', 'r',
      /* // Attack button, useful on F for testing
      'f',
      /*/
      SDLK_LEFT, // kick button
      //*/
      16, // shift keky, for editor
      'p', // restart level
      // Level Select
      '1',
    },
    .level = 0,
  };
  game_init(&game);
  camera_build_perspective(&game.camera);
  //camera_build_orthographic(&game.camera);

  // load this first since it's the loading screen spinner
  game.models.color_cube.type = MODEL_CUBE_COLOR;
  model_build(&game.models.color_cube);

  shader_program_build_basic(&game.shaders.basic);
}

static void cheesy_loading_animation(float dt) {
  static float cubespin = 0;
  mat4 projview = camera_projection_view(&game.camera);

  shader_program_use(&game.shaders.basic);
  int projViewMod_loc = game.shaders.basic.uniform.projViewMod;

  mat4 model = m4translation(v3f(0, 0, 0));
  model = m4mul(model, m4rotation(v3norm(v3f(1.f, 1.5f, -.7f)), cubespin));
  model = m4mul(model, m4rotation(v3norm(v3f(-4.f, 1.5f, 1.f)), cubespin/3.6f));

  glUniformMatrix4fv(projViewMod_loc, 1, 0, m4mul(projview, model).f);
  model_render(&game.models.color_cube);
  cubespin += 2 * dt;
}

int export(wasm_load) (int await_count, float dt) {
  if (await_count || !GAME_ON) {
    cheesy_loading_animation(dt);
    return 0;
  };

  #if GAME_ON == 1

  str_log("Shader1");

  // Build shaders from async data
  Shader light_vert, light_frag;
  shader_build_from_file(&light_vert, &file_vert);
  str_log("Shader2");
  shader_build_from_file(&light_frag, &file_frag);

  str_log("Test model");
  model_load_obj(&game.models.level_test, &file_model_test);
  str_log("Gear model");
  model_load_obj(&game.models.gear, &file_model_gear);
  //model_load_obj(&game.models.level_1, &file_model_level_1);

  str_log("Building shaders");

  shader_program_build(&game.shaders.light, &light_vert, &light_frag);
  shader_program_load_uniforms(&game.shaders.light, UNIFORMS_PHONG);

  str_log("Loading textures");

  // Build textures from async data
  texture_build_from_image(&game.textures.crate, &image_crate);
  str_log("texture2");
  //texture_build_from_image(&game.textures.level, &image_level);
  texture_build_from_image(&game.textures.brass, &image_brass);
  str_log("texture3");
  texture_build_from_image(&game.textures.tiles, &image_tiles);
  str_log("texture4");
  texture_build_from_image(&game.textures.player, &image_anim_test);

  str_log("Deleting extras");

  // Delete async loaded resources
  file_delete(&file_vert);
  str_log("deleting frag");
  file_delete(&file_frag);
  str_log("deleting test");
  file_delete(&file_model_test);
  str_log("deleting gear");
  file_delete(&file_model_gear);
  //file_delete(&file_model_level_1);
  str_log("Deleting images");
  //image_delete(&image_level);
  image_delete(&image_crate);
  image_delete(&image_brass);
  image_delete(&image_tiles);
  image_delete(&image_anim_test);

  str_log("Grids");

  // Set up game models
  game.models.grid.grid = (Model_Grid) {
    .type = MODEL_GRID,
    .basis = {v3x, v3y, v3z},
    .primary = {0, 1},
    .extent = 100
  };

  game.models.player.sprites = (Model_Sprites) {
    .type = MODEL_SPRITES,
    .grid = {.w = 16, .h = 16},
  };

  str_log("Building models");

  model_build(&game.models.player);
  model_build(&game.models.level_test);
  //model_build(&game.models.level_1);
  model_build(&game.models.gear);
  model_grid_set_default(&game.models.gizmo, -2);
  game.models.box.type = MODEL_CUBE;
  model_build(&game.models.box);
  model_build(&game.models.grid);
  model_build(&game.models.gizmo);

  // Load the first game level
  level_switch(&game, game.level);

  str_log("Finished setup");

  #endif

  return 1;
}

void export(wasm_update) (float dt) {
  PARAM_UNUSED(dt);
  process_system_events(&game);
  level_switch_check(&game);
  game_update(&game, dt);
}

void export(wasm_render) () {
  game_render(&game);
}

#ifndef __WASM__

bool game_continue = true;
bool game_loaded = false;

int main(int argc, char* argv[]) {
  PARAM_UNUSED(argc);
  PARAM_UNUSED(argv);

  str_log("Here we go!");

  SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
  SDL_Window* window = SDL_CreateWindow("Window Title", 400, 400, flags);

  if (!window) goto close_window;

  SDL_GLContext ctx = SDL_GL_CreateContext(window);

  if (!ctx) goto close_gl_context;

  glClearColor(0, 0, 1, 1);
  glClearDepth(1);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDepthFunc(GL_LEQUAL);

  wasm_preload(400, 400);

  while (game_continue) {
    float dt = 0.016f;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!game_loaded) {
      process_system_events(&game);

      if (wasm_load(0, dt)) {
        str_log("Loaded");
        game_loaded = true;
      }

    } else {
      wasm_update(dt);
      wasm_render();
    }

    SDL_GL_SwapWindow(window);
  }

  fflush(stdout);

  close_gl_context:
  SDL_GL_DestroyContext(ctx);

  close_window:
  SDL_DestroyWindow(window);

  return 0;
}

#endif
