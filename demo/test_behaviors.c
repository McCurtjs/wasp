#include "test_behaviors.h"

#include "input_map.h"

#include "gl.h"

#include "types.h"
#include "draw.h"

void behavior_test_camera(Entity* e, Game* game, float dt) {
  UNUSED(e);

  float xrot = d2r(-game->input.mouse.move.y * 180 / (float)game->window.h);
  float yrot = d2r(-game->input.mouse.move.x * 180 / (float)game->window.x);

  //if (game->input.pressed.lmb) {
  if (input_pressed(game, IN_CLICK)) {
    vec3 angles = v3f(xrot, yrot, 0);
    camera_orbit(&game->camera, game->target, angles.xy);
  }

  if (input_pressed(game, IN_RELOAD)) { //game->input.pressed.rmb) {
    mat4 light_rotation = m4rotation(v3y, yrot);
    game->light_pos = mv4mul(light_rotation, game->light_pos);
  }

  if (input_pressed(game, IN_JUMP)) //game->input.pressed.forward)
    game->camera.pos.xyz = v3add(game->camera.pos.xyz, v3scale(game->camera.front.xyz, dt));

  if (input_pressed(game, IN_DOWN)) //game->input.pressed.back)
    game->camera.pos.xyz = v3add(game->camera.pos.xyz, v3scale(game->camera.front.xyz, -dt));

  if (input_pressed(game, IN_RIGHT)) { //game->input.pressed.right) {
    vec3 right = v3norm(v3cross(game->camera.front.xyz, game->camera.up.xyz));
    right = v3scale(right, dt);
    game->camera.pos.xyz = v3add(game->camera.pos.xyz, right);
    game->target = v3add(game->target, right);
  }

  if (input_pressed(game, IN_LEFT)) { //game->input.pressed.left) {
    vec3 left = v3norm(v3cross(game->camera.front.xyz, game->camera.up.xyz));
    left = v3scale(left, -dt);
    game->camera.pos.xyz = v3add(game->camera.pos.xyz, left);
    game->target = v3add(game->target, left);
  }
}

void behavior_cubespin(Entity* e, Game* game, float dt) {
  UNUSED(game);

  e->transform = m4translation(e->pos);
  e->transform = m4mul(e->transform, m4rotation(v3norm(v3f(1.f, 1.5f, -.7f)), e->angle));
  e->transform = m4mul(e->transform, m4rotation(v3norm(v3f(-4.f, 1.5f, 1.f)), e->angle/3.6f));
  e->angle += 2 * dt;
}

void behavior_stare(Entity* e, Game* game, float dt) {
  UNUSED(dt);

  e->transform = m4look(e->pos, game->camera.pos.xyz, v3y);
}

void behavior_attach_to_light(Entity* e, Game* game, float dt) {
  UNUSED(dt);

  e->transform = m4translation(game->light_pos.xyz);
}

void behavior_attach_to_camera_target(Entity* e, Game* game, float dt) {
  UNUSED(dt);

  e->transform = m4translation(game->target);
}

void render_basic(Entity* e, Game* game) {
  Shader shader = game->shaders.basic;
  shader_bind(shader);

  int loc_pvm = shader_uniform_loc(shader, "projViewMod");

  mat4 pvm = m4mul(game->camera.projview, e->transform);
  glUniformMatrix4fv(loc_pvm, 1, GL_FALSE, pvm.f);

  model_render(e->model);
}

void render_debug(Entity* e, Game* game) {
  render_basic(e, game);
  draw_render();
}

void render_phong(Entity* e, Game* game) {
  Shader shader = game->shaders.light;
  shader_bind(shader);

  int loc_pvm = shader_uniform_loc(shader, "projViewMod");
  int loc_light_pos = shader_uniform_loc(shader, "lightPos");
  int loc_camera_pos = shader_uniform_loc(shader, "cameraPos");
  int loc_use_vert_color = shader_uniform_loc(shader, "useVertexColor");
  int loc_sampler_tex = shader_uniform_loc(shader, "texSamp");
  int loc_world = shader_uniform_loc(shader, "world");

  glUniformMatrix4fv(loc_pvm, 1, 0, m4mul(game->camera.projview, e->transform).f);
  glUniform4fv(loc_light_pos, 1, game->light_pos.f);
  glUniform4fv(loc_camera_pos, 1, game->camera.pos.f);

  GLint use_color = false;
  if (e->model->type == MODEL_OBJ) use_color = e->model->mesh.use_color;
  glUniform1i(loc_use_vert_color, use_color);

  tex_apply(e->texture, 0, loc_sampler_tex);

  glUniformMatrix4fv(loc_world, 1, 0, e->transform.f);
  model_render(e->model);

  glBindTexture(GL_TEXTURE_2D, 0);
}
