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
#include "draw.h"
#include "gl.h"

#define CAMERA_SPEED 9.0f

////////////////////////////////////////////////////////////////////////////////
// Behavior for controlling camera rotation and movement
////////////////////////////////////////////////////////////////////////////////

void behavior_test_camera(Entity* e, game_t* game, float dt) {
  UNUSED(e);

  demo_t* demo = game->demo;

  float xrot = d2r(-game->input.mouse.move.y * 180 / (float)game->window.h);
  float yrot = d2r(-game->input.mouse.move.x * 180 / (float)game->window.x);

  //if (game->input.pressed.lmb) {
  if (input_pressed(IN_CLICK)) {
    vec3 angles = v3f(xrot, yrot, 0);
    camera_orbit(&game->camera, demo->target, angles.xy);
  }

  float cam_speed = CAMERA_SPEED * dt;

  if (input_pressed(IN_CLOSE)) {
    game_quit(game);
  }

  if (input_pressed(IN_ROTATE_LIGHT)) { //game->input.pressed.rmb) {
    mat4 light_rotation = m4rotation(v3y, 4.f * dt);//yrot);
    demo->light_pos = mv4mul(light_rotation, demo->light_pos);
  }

  if (input_pressed(IN_JUMP)) //game->input.pressed.forward)
    game->camera.pos.xyz = v3add(
      game->camera.pos.xyz,
      v3scale(game->camera.front.xyz, cam_speed)
    );

  if (input_pressed(IN_DOWN)) //game->input.pressed.back)
    game->camera.pos.xyz = v3add(
      game->camera.pos.xyz,
      v3scale(game->camera.front.xyz, -cam_speed)
    );

  if (input_pressed(IN_RIGHT)) { //game->input.pressed.right) {
    vec3 right = v3norm(v3cross(game->camera.front.xyz, game->camera.up.xyz));
    right = v3scale(right, cam_speed);
    game->camera.pos.xyz = v3add(game->camera.pos.xyz, right);
    demo->target = v3add(demo->target, right);
  }

  if (input_pressed(IN_LEFT)) { //game->input.pressed.left) {
    vec3 left = v3norm(v3cross(game->camera.front.xyz, game->camera.up.xyz));
    left = v3scale(left, -cam_speed);
    game->camera.pos.xyz = v3add(game->camera.pos.xyz, left);
    demo->target = v3add(demo->target, left);
  }

  if (input_pressed(IN_REWIND)) {
    demo->light_pos = game->camera.pos;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Toggles the visibility of the grid
////////////////////////////////////////////////////////////////////////////////

void behavior_grid_toggle(Entity* e, game_t* game, float dt) {
  UNUSED(dt);
  UNUSED(game);
  if (input_triggered(IN_TOGGLE_GRID)) {
    e->hidden = !e->hidden;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Cube spinning behavior
////////////////////////////////////////////////////////////////////////////////

void behavior_cubespin(Entity* e, game_t* game, float dt) {
  UNUSED(game);

  e->transform = m4translation(e->pos);
  e->transform = m4mul(e->transform, m4rotation(v3norm(v3f(1.f, 1.5f, -.7f)), e->angle));
  e->transform = m4mul(e->transform, m4rotation(v3norm(v3f(-4.f, 1.5f, 1.f)), e->angle/3.6f));
  e->angle += 2 * dt;
}

////////////////////////////////////////////////////////////////////////////////
// Clockwise spin around the Z axis
////////////////////////////////////////////////////////////////////////////////

void behavior_gear_rotate_cw(Entity* e, game_t* game, float dt) {
  UNUSED(game);
  e->transform = m4mul(e->transform, m4rotation(v3front, dt));
}

////////////////////////////////////////////////////////////////////////////////
// Counter-clockwise spin around the Z axis
////////////////////////////////////////////////////////////////////////////////

void behavior_gear_rotate_ccw(Entity* e, game_t* game, float dt) {
  UNUSED(game);
  e->transform = m4mul(e->transform, m4rotation(v3front, -dt));
}

////////////////////////////////////////////////////////////////////////////////
// Orients the entity to face the camera along its local +Z axis
////////////////////////////////////////////////////////////////////////////////

void behavior_stare(Entity* e, game_t* game, float dt) {
  UNUSED(dt);
  e->transform = m4look(e->pos, game->camera.pos.xyz, v3y);
}

////////////////////////////////////////////////////////////////////////////////
// Sets the location of the entity to the light position
////////////////////////////////////////////////////////////////////////////////

void behavior_attach_to_light(Entity* e, game_t* game, float dt) {
  UNUSED(dt);
  e->transform = m4translation(game->demo->light_pos.xyz);
}

////////////////////////////////////////////////////////////////////////////////
// Sets the location of the entity to the camera target position
////////////////////////////////////////////////////////////////////////////////

void behavior_attach_to_camera_target(Entity* e, game_t* game, float dt) {
  UNUSED(dt);
  e->transform = m4translation(game->demo->target);
}

////////////////////////////////////////////////////////////////////////////////
// Render function for un-shaded objects with vertex color
////////////////////////////////////////////////////////////////////////////////

void render_basic(Entity* e, game_t* game) {
  Shader shader = game->demo->shaders.basic;
  shader_bind(shader);

  int loc_pvm = shader_uniform_loc(shader, "in_pvm_matrix");

  mat4 pvm = m4mul(game->camera.projview, e->transform);
  glUniformMatrix4fv(loc_pvm, 1, GL_FALSE, pvm.f);

  model_render(e->model);
}

////////////////////////////////////////////////////////////////////////////////
// Renders the debug objects
////////////////////////////////////////////////////////////////////////////////

void render_debug(Entity* e, game_t* game) {
  render_basic(e, game);
  draw_render();
}

////////////////////////////////////////////////////////////////////////////////
// Render function for physically-based lighting
////////////////////////////////////////////////////////////////////////////////

void render_pbr(Entity* e, game_t* game) {
  Shader shader = game->demo->shaders.light;
  shader_bind(shader);

  int loc_pvm = shader_uniform_loc(shader, "in_pvm_matrix");
  int loc_sampler_tex = shader_uniform_loc(shader, "texSamp");
  int loc_sampler_norm = shader_uniform_loc(shader, "normSamp");
  int loc_sampler_spec = shader_uniform_loc(shader, "specSamp");
  int loc_norm = shader_uniform_loc(shader, "in_normal_matrix");
  int loc_props = shader_uniform_loc(shader, "in_props");
  int loc_tint = shader_uniform_loc(shader, "in_tint");

  mat4 pvm = m4mul(game->camera.projview, e->transform);
  mat4 norm = m4transpose(m4inverse(m4mul(game->camera.view, e->transform)));

  glUniformMatrix4fv(loc_pvm, 1, 0, pvm.f);
  glUniformMatrix4fv(loc_norm, 1, 0, norm.f);

  vec2 props = v2f(e->material->roughness, e->material->metalness);
  glUniform2fv(loc_props, 1, props.f);
  glUniform3fv(loc_tint, 1, e->tint.f);

  //glUniform4fv(loc_light_pos, 1, demo->light_pos.f);
  //glUniform4fv(loc_camera_pos, 1, game->camera.pos.f);

  //GLint use_color = false;
  //if (e->model->type == MODEL_OBJ) use_color = e->model->mesh.use_color;
  //glUniform1i(loc_use_vert_color, use_color);

  tex_apply(e->material->diffuse, 0, loc_sampler_tex);
  tex_apply(e->material->normals, 1, loc_sampler_norm);
  tex_apply(e->material->specular, 2, loc_sampler_spec);

  model_render(e->model);

  glBindTexture(GL_TEXTURE_2D, 0);
}
