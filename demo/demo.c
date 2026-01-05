/*******************************************************************************
* MIT License
*
* Copyright (c) 2025 Curtis McCoy
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include "demo.h"
#include "wasp.h"
#include "wasm.h"

#include "SDL3/SDL.h"
#include "gl.h"

static File file_model_gear = NULL;

#ifdef __clang__
# pragma clang diagnostic ignored "-Wconstant-logical-operand"
#endif

static demo_t demo;

static int active_shader = 1;

static Model model_frame = { .type = MODEL_FRAME };// { .type = MODEL_FRAME };

static renderer_t _renderer_basic = {
  .render_entity = render_basic2,
};
renderer_t* renderer_basic = &_renderer_basic;

static renderer_t _renderer_pbr = {
  .register_entity = render_pbr_register,
  .unregister_entity = render_pbr_unregister,
  .render_entity = render_pbr2,
  .render = render_pbr3,
};
renderer_t* renderer_pbr = &_renderer_pbr;

static renderer_t _renderer_debug = {
  .render_entity = render_basic2,
  .render = render_debug3,
};
renderer_t* renderer_debug = &_renderer_debug;

static renderer_t* renderers[] = {
  &_renderer_basic, &_renderer_pbr, &_renderer_debug
};

////////////////////////////////////////////////////////////////////////////////
// Mapping of keys to actions
////////////////////////////////////////////////////////////////////////////////

static keybind_t input_map[] = {
  { .name = IN_CLOSE, .key = SDLK_ESCAPE },
  { .name = IN_JUMP, .key = 'w' },
  { .name = IN_LEFT, .key = 'a' },
  { .name = IN_DOWN, .key = 's' },
  { .name = IN_RIGHT, .key = 'd' },
  { .name = IN_KICK, .key = SDLK_LEFT },
  { .name = IN_SNAP_LIGHT, .key = 'r' },
  { .name = IN_CAMERA_LOCK, .key = 'c' },
  { .name = IN_CLICK, .key = SDL_BUTTON_LEFT, .mouse = true },
  { .name = IN_CREATE_OBJECT, .key = SDL_BUTTON_RIGHT, .mouse = true },
  { .name = IN_CLICK_MOVE, .key = SDL_BUTTON_RIGHT, .mouse = true },
  { .name = IN_ROTATE_LIGHT, .key = 'e' },
  { .name = IN_SHIFT, .key = 16 },
  { .name = IN_RELOAD, .key = 'p' },
  { .name = IN_LEVEL_1, .key = '1' },
  { .name = IN_LEVEL_2, .key = '2' },
  { .name = IN_LEVEL_3_1, .key = '3' },
  { .name = IN_LEVEL_3_2, .key = '4' },
  { .name = IN_LEVEL_3_3, .key = '5' },
  { .name = IN_LEVEL_3_4, .key = '6' },
  { .name = IN_LEVEL_3_5, .key = '7' },
  { .name = IN_TOGGLE_SHADER, .key = 'm' },
  { .name = IN_TOGGLE_GRID, .key = 'g' },
};

static scene_load_fn_t demo_scenes[] =
{ scene_load_gears
, scene_load_wizard
, scene_load_monument
, scene_load_monument2
, scene_load_monument3
, scene_load_monument4
, scene_load_monument5
};

////////////////////////////////////////////////////////////////////////////////
// Pre-initializer to set window size and title
////////////////////////////////////////////////////////////////////////////////

void wasp_init(app_defaults_t* game) {
  game->window = v2i(1024, 768);
  game->title = str_copy("WASP Demo");

  demo = (demo_t) {
    .target = v3origin,
    .light_pos = v4f(4, 3, 5, 1),
  };

  game->demo = &demo;
}

////////////////////////////////////////////////////////////////////////////////
// Callback to update demo-specific render target size when window size changes
////////////////////////////////////////////////////////////////////////////////

void demo_callback_window_resize(Game game) {
  rt_resize(game->demo->render_target, game->window);
}

////////////////////////////////////////////////////////////////////////////////
// Spinny cube to show while loading
////////////////////////////////////////////////////////////////////////////////

static void cheesy_loading_animation(Game game, float dt) {
  static float cubespin = 0;

  rt_bind_default();

  mat4 projview = camera_projection_view(&game->camera);

  shader_bind(demo.shaders.loading);
  int loc_pvm = shader_uniform_loc(demo.shaders.loading, "in_pvm_matrix");

  mat4 model = m4translation(v3f(0, 0, -3));
  model = m4mul(model, m4rotation(v3norm(v3f(1.f, 1.5f, -.7f)), cubespin));
  model = m4mul(model, m4rotation(v3norm(v3f(-4.f, 1.5f, 1.f)), cubespin / 3.6f));

  glUniformMatrix4fv(loc_pvm, 1, 0, m4mul(projview, model).f);
  model_render(&demo.models.color_cube);
  cubespin += 2 * dt;

  GLenum err = glGetError();
  if (err) {
    str_log("[Demo.load_anim] Error: 0x{!x}", err);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Game preloader - starts asynchronous loading
////////////////////////////////////////////////////////////////////////////////

bool wasp_preload(Game game) {
  UNUSED(game);

  file_model_gear = file_new(S("./res/models/gear.obj"));

  demo.materials.grass = material_new(S("grass_rocky"));
  demo.materials.grass->use_normal_map = true;
  demo.materials.grass->use_roughness_map = true;
  demo.materials.grass->weight_roughness = 1.0f;

  demo.materials.crate = material_new(S("crate"));
  demo.materials.crate->weight_roughness = 1.0f;

  demo.materials.tiles = material_new(S("tiles"));

  demo.materials.sands = material_new(S("brass2"));
  demo.materials.sands->weight_roughness = 1.0f;
  demo.materials.sands->use_normal_map = true;

  demo.materials.mudds = material_new(S("rustediron2"));
  demo.materials.mudds->use_normal_map = true;
  demo.materials.mudds->use_roughness_map = true;
  demo.materials.mudds->use_metalness_map = true;
  demo.materials.mudds->weight_roughness = 0.8f;
  demo.materials.mudds->weight_metalness = 1.0f;

  demo.materials.renderite = material_new(S("renderite"));
  demo.materials.renderite->use_diffuse_map = false;
  demo.materials.renderite->weight_roughness = 0.2f;
  demo.materials.renderite->weight_metalness = 0.8f;

  material_load_all_async();

  demo.shaders.basic = shader_new_load_async(S("basic_deferred"));
  demo.shaders.light = shader_new_load_async(S("light"));
  demo.shaders.frame = shader_new_load_async(S("frame"));
  demo.shaders.warhol = shader_new_load_async(S("warhol"));
  demo.shaders.light_inst = shader_new(S("light_inst"));
  shader_file_frag(demo.shaders.light_inst, S("light"));
  shader_load_async(demo.shaders.light_inst);

  return true;
}

////////////////////////////////////////////////////////////////////////////////
// Game loader - after async loading, process loaded objects for the game
////////////////////////////////////////////////////////////////////////////////

#ifdef far
# undef far // why does windows define "far", lol
#endif

bool wasp_load (Game game, int await_count, float dt) {

  // load this first since it's the loading screen spinner
  if (!demo.shaders.loading) {
    demo.shaders.loading = shader_new(S("basic"));
    demo.models.color_cube.type = MODEL_CUBE_COLOR;
    model_build(&demo.models.color_cube);
    model_build(&model_frame);
  }

  if (await_count) {
    cheesy_loading_animation(game, dt);
    return 0;
  };

  str_write("[Demo.Load] Async load finished");

  game->camera.perspective.far = 3000.0f;
  game->on_window_resize = demo_callback_window_resize;

  game->demo->render_target = rt_new(
    TF_RGB_8, TF_RG_16, TF_RGB_10_A_2, TF_DEPTH_32
  );
  rt_build(game->demo->render_target, game->window);

  game->input.keymap = span_keymap(input_map, ARRAY_COUNT(input_map));
  game->scenes = span_scene(demo_scenes, ARRAY_COUNT(demo_scenes));
  game->renderers = span_renderer(renderers, ARRAY_COUNT(renderers));

  shader_build_all();
  material_build_all();
  model_load_obj(&demo.models.gear, file_model_gear);

  _renderer_pbr.shader = game->demo->shaders.light_inst;
  _renderer_pbr.groups = map_rg_new();

  // Delete async loaded resources
  file_delete(&file_model_gear);

  // Set up game models
  demo.models.grid.grid = (Model_Grid) {
    .type = MODEL_GRID,
    .basis = {v3x, v3y, v3z},
    .primary = {0, 2},
    .extent = 100
  };

  demo.models.player.sprites = (Model_Sprites) {
    .type = MODEL_SPRITES,
    .grid = {.w = 16, .h = 16},
  };

  demo.models.box.type = MODEL_CUBE;

  demo.models.box_inst.type = MODEL_CUBE;

  model_build(&demo.models.player);
  model_build(&demo.models.level_test);
  //model_build(&game.models.level_1);
  model_build(&demo.models.gear);
  model_grid_set_default(&demo.models.gizmo, -2);
  model_build(&demo.models.box);
  model_build(&demo.models.box_inst);
  model_build(&demo.models.grid);
  model_build(&demo.models.gizmo);

  str_write("[Demo.Load] Loading complete!");

  return 1;
}

////////////////////////////////////////////////////////////////////////////////
// Game per-frame update
////////////////////////////////////////////////////////////////////////////////

bool wasp_update (Game game, float dt) {
  if (input_triggered(IN_TOGGLE_SHADER)) {
    active_shader = (active_shader + 1) % 2;
  }

  game_update(game, dt);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// Game rendering after update
////////////////////////////////////////////////////////////////////////////////

#include "light.h"

#include <stdlib.h>
#include <string.h>

void wasp_render(Game game) {
  rt_bind(game->demo->render_target);
  game_render(game);

  texture_t lights = tex_from_lights();

  rt_bind_default();
  /*
  shader_program_use(&shader_frame);
  tex_apply(game.textures.render_target->textures[0], 0, 0);
  /*/
  Shader shader = demo.shaders.warhol;
  if (active_shader == 1) {
    shader = demo.shaders.frame;
  }

  shader_bind(shader);
  int tex_sampler = shader_uniform_loc(shader, "texSamp");
  int norm_sampler = shader_uniform_loc(shader, "normSamp");
  int prop_sampler = shader_uniform_loc(shader, "propSamp");
  int depth_sampler = shader_uniform_loc(shader, "depthSamp");
  int light_sampler = shader_uniform_loc(shader, "lightSamp");
  int loc_invproj = shader_uniform_loc(shader, "in_proj_inverse");
  int loc_light_pos = shader_uniform_loc(shader, "lightPos");
  tex_apply(game->demo->render_target->textures[0], 0, tex_sampler);
  tex_apply(game->demo->render_target->textures[1], 1, norm_sampler);
  tex_apply(game->demo->render_target->textures[2], 2, prop_sampler);
  tex_apply(game->demo->render_target->textures[3], 3, depth_sampler);
  tex_apply(lights, 4, light_sampler);
  glUniformMatrix4fv(loc_invproj, 1, 0, m4inverse(game->camera.projection).f);
  glUniform4fv(loc_light_pos, 1, mv4mul(game->camera.view, demo.light_pos).f);

  model_render(&model_frame);

  tex_free(&lights);
}
