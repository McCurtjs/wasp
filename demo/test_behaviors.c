#include "test_behaviors.h"

#include "gl.h"

#include "types.h"
#include "draw.h"

void behavior_test_camera(Entity* e, Game* game, float dt) {
  PARAM_UNUSED(e);

  float xrot = d2r(-game->input.mouse.move.y * 180 / (float)game->window.h);
  float yrot = d2r(-game->input.mouse.move.x * 180 / (float)game->window.x);

  if (game->input.pressed.lmb) {
    vec3 angles = v3f(xrot, yrot, 0);
    camera_orbit(&game->camera, game->target, angles.xy);
  }

  if (game->input.pressed.rmb) {
    mat4 light_rotation = m4rotation(v3y, yrot);
    game->light_pos = mv4mul(light_rotation, game->light_pos);
  }

  if (game->input.pressed.forward)
    game->camera.pos.xyz = v3add(game->camera.pos.xyz, v3scale(game->camera.front.xyz, dt));

  if (game->input.pressed.back)
    game->camera.pos.xyz = v3add(game->camera.pos.xyz, v3scale(game->camera.front.xyz, -dt));

  if (game->input.pressed.right) {
    vec3 right = v3norm(v3cross(game->camera.front.xyz, game->camera.up.xyz));
    right = v3scale(right, dt);
    game->camera.pos.xyz = v3add(game->camera.pos.xyz, right);
    game->target = v3add(game->target, right);
  }

  if (game->input.pressed.left) {
    vec3 left = v3norm(v3cross(game->camera.front.xyz, game->camera.up.xyz));
    left = v3scale(left, -dt);
    game->camera.pos.xyz = v3add(game->camera.pos.xyz, left);
    game->target = v3add(game->target, left);
  }
}

void behavior_cubespin(Entity* e, Game* game, float dt) {
  PARAM_UNUSED(game);

  e->transform = m4translation(e->pos);
  e->transform = m4mul(e->transform, m4rotation(v3norm(v3f(1.f, 1.5f, -.7f)), e->angle));
  e->transform = m4mul(e->transform, m4rotation(v3norm(v3f(-4.f, 1.5f, 1.f)), e->angle/3.6f));
  e->angle += 2 * dt;
}

void behavior_stare(Entity* e, Game* game, float dt) {
  PARAM_UNUSED(dt);

  e->transform = m4look(e->pos, game->camera.pos.xyz, v3y);
}

void behavior_attach_to_light(Entity* e, Game* game, float dt) {
  PARAM_UNUSED(dt);

  e->transform = m4translation(game->light_pos.xyz);
}

void behavior_attach_to_camera_target(Entity* e, Game* game, float dt) {
  PARAM_UNUSED(dt);

  e->transform = m4translation(game->target);
}

void render_basic(Entity* e, Game* game) {
  shader_program_use(e->shader);
  int projViewMod_loc = e->shader->uniform.projViewMod;

  mat4 pvm = m4mul(game->camera.projview, e->transform);
  glUniformMatrix4fv(projViewMod_loc, 1, GL_FALSE, pvm.f);
  model_render(e->model);
}

void render_debug(Entity* e, Game* game) {
  render_basic(e, game);
  draw_render();
}

void render_phong(Entity* e, Game* game) {
  shader_program_use(e->shader);
  UniformLocsPhong uniforms = e->shader->uniform.phong;
  uint pvm = e->shader->uniform.projViewMod;
  // should also pass transpose(inverse(model)) to multiply against normal

  glUniformMatrix4fv(pvm, 1, 0, m4mul(game->camera.projview, e->transform).f);
  glUniform4fv(uniforms.lightPos, 1, game->light_pos.f);
  glUniform4fv(uniforms.cameraPos, 1, game->camera.pos.f);

  glActiveTexture(GL_TEXTURE0);
  glUniform1i(uniforms.sampler, 0);

  glBindTexture(GL_TEXTURE_2D, e->texture.handle);
  glUniformMatrix4fv(uniforms.world, 1, 0, e->transform.f);
  model_render(e->model);

  glBindTexture(GL_TEXTURE_2D, 0);
}
