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
#include "light.h"

#define CAMERA_SPEED 0.8f

////////////////////////////////////////////////////////////////////////////////
// Behavior for controlling camera rotation and movement
////////////////////////////////////////////////////////////////////////////////

void behavior_camera_test(Game game, entity_t* e, float dt) {
  UNUSED(e);

  demo_t* demo = game->demo;

  float xrot = d2r(-game->input.mouse.move.y * 180 / (float)game->window.h);
  float yrot = d2r(-game->input.mouse.move.x * 180 / (float)game->window.x);

  //if (game->input.pressed.lmb) {
  if (input_pressed(IN_CLICK)) {
    vec3 angles = v3f(xrot, yrot, 0);
    camera_orbit(&game->camera, demo->target, angles.xy);
  }

  vec3 view_dir = v3sub(game->demo->target, game->camera.pos.xyz);
  float cam_speed = CAMERA_SPEED * dt * v3mag(view_dir);

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

  // Other random inputs that aren't actaully related to camera motion

  if (input_pressed(IN_SNAP_LIGHT)) {
    demo->light_pos = game->camera.pos;
  }

  if (input_pressed(IN_ROTATE_LIGHT)) { //game->input.pressed.rmb) {
    mat4 light_rotation = m4rotation(v3y, 4.f * dt);//yrot);
    demo->light_pos = mv4mul(light_rotation, demo->light_pos);
  }

  if (input_triggered(IN_RELOAD)) {
    game->next_scene = 0;
  }
}

////////////////////////////////////////////////////////////////////////////////

vec3 game_screen_to_ray(Game game) {
  return camera_screen_to_ray(
    &game->camera, game->window, game->input.mouse.pos
  );
}

void behavior_wizard_level(Game game, entity_t* e, float dt) {
  UNUSED(dt);
  if (input_pressed(IN_CLICK_MOVE)) {
    vec3 ray = game_screen_to_ray(game);
    float t;
    if (v3ray_plane(game->camera.pos.xyz, ray, v3origin, v3up, &t)) {
      game->demo->target = v3add(game->camera.pos.xyz, v3scale(ray, t));
    }
  }
  e->transform.col[3].xyz = v3zero;
  e->transform = m4mul(e->transform, m4rotation(v3z, dt)),
  e->transform.col[3].xyz = game->demo->target;
}

#define WIZARD_SPEED 8.0f
#define WIZARD_FAKE_SPEED (WIZARD_SPEED * 4.0f)
#define WIZARD_FIRE_RATE 0.05f
#define WIZARD_BADDY_SPAWN_RATE 3.0f

static void _wizard_movement(Game game, entity_t* e, float dt) {
  static vec3 fake_target = svNzero;

  demo_t* demo = game->demo;

  vec3 target = demo->target;
  vec3 pos = e->pos;
  target.y = pos.y;
  vec3 to_target = v3sub(target, pos);
  float distance = v3mag(to_target);
  float max_move = WIZARD_SPEED * dt;
  if (distance + 0.1f < WIZARD_SPEED / 3.f) {
    max_move *= (distance + 0.1f) / (WIZARD_SPEED / 3.f);
  }
  if (input_pressed(IN_CLICK)) {
    max_move /= 2.0f;
  }
  if (distance <= max_move) {
    e->pos = target;
  }
  else {
    vec3 dir = v3norm(to_target);
    e->pos = v3add(pos, v3scale(dir, max_move));
  }

  to_target = v3sub(target, fake_target);
  distance = v3mag(to_target);
  max_move = WIZARD_FAKE_SPEED * dt;
  if (distance + 0.1f < WIZARD_FAKE_SPEED / 3.f) {
    max_move *= (distance + 0.1f) / (WIZARD_FAKE_SPEED / 3.f);
  }
  if (distance <= max_move) {
    fake_target = target;
  }
  else {
    vec3 dir = v3norm(to_target);
    fake_target = v3add(fake_target, v3scale(dir, max_move));
  }

  e->transform = m4translation(e->pos);
  game->camera.pos.xyz = v3add(e->pos, v3f(6, 12, 7));

  vec3 cam_to_target = v3dir(fake_target, game->camera.pos.xyz);
  vec3 cam_to_wizard = v3dir(e->pos, game->camera.pos.xyz);
  //vec3 diff = v3sub(cam_to_target, cam_to_wizard);
  cam_to_target = v3add(cam_to_wizard, v3rescale(cam_to_target, 0.3f));
  cam_to_target = v3norm(cam_to_target);

  game->camera.front.xyz = cam_to_target;
}

static void behavior_projectile(Game game, entity_t* e, float dt) {
  UNUSED(game);
  vec3 new_pos = v3add(e->transform.col[3].xyz, v3scale(e->pos, dt * 25.f));
  e->transform.col[3].xyz = new_pos;
  light_t* light = light_ref(e->tmp);
  if (light) {
    light->pos = new_pos;
  }
  if (new_pos.y < 0) {
    entity_remove(game, e->id);
  }
}

#define con_type slotkey_t
#define con_prefix id
#include "span.h"
#include "array.h"
#undef con_type
#undef con_prefix

Array_id wizard_bullets = NULL;

static void ondelete_projectile(Game game, entity_t* e) {
  UNUSED(game);
  light_remove(e->tmp);
  if (wizard_bullets) {
    slotkey_t* arr_foreach_index(id, i, wizard_bullets) {
      if (id->hash == e->id.hash) {
        arr_id_remove_unstable(wizard_bullets, i);
      }
    }
  }
}

static void _wizard_projectile(Game game, entity_t* e, float dt) {
  demo_t* demo = game->demo;
  static float shot_timer = WIZARD_FIRE_RATE;
  shot_timer += dt;
  if (input_pressed(IN_CLICK) && shot_timer > WIZARD_FIRE_RATE) {
    vec3 click_ray = game_screen_to_ray(game);
    float t;
    if (v3ray_plane(game->camera.pos.xyz, click_ray, v3origin, v3up, &t)) {
      vec3 click_pos = v3add(game->camera.pos.xyz, v3scale(click_ray, t));
      click_pos.y = 1.5f;
      vec3 launch_point = v3f(e->pos.x, 2, e->pos.z);

      slotkey_t light_id = light_add((light_t) {
        .color = v3f(1.0f, 0.7f, 0.8f),
        .intensity = 8.f,
        .pos = launch_point
      });

      slotkey_t eid = entity_add(game, &(entity_t) {
        .pos = v3dir(click_pos, launch_point),
        .transform = m4ts(launch_point, 0.4f),
        .model = &demo->models.color_cube,
        .render = render_basic,
        .behavior = behavior_projectile,
        .ondelete = ondelete_projectile,
        .tmp = light_id,
      });

      if (!wizard_bullets) {
        wizard_bullets = arr_id_new();
      }

      arr_id_push_back(wizard_bullets, eid);
    }
  }
}

#include <math.h>

#define BADDY_SPEED 4.0f
static void behavior_baddy(Game game, entity_t* e, float dt) {
  vec3 target = game->demo->target;
  vec3 pos = e->transform.col[3].xyz;
  vec3 dir = v3dir(target, pos);
  pos = v3add(pos, v3scale(dir, BADDY_SPEED * dt));
  pos.y = 1.0f;
  e->transform.col[3].xyz = pos;

  if (wizard_bullets) {
    slotkey_t* arr_foreach(bullet_id, wizard_bullets) {
      entity_t* bullet = entity_ref(game, *bullet_id);
      if (!bullet) continue;
      vec3 bullet_pos = bullet->transform.col[3].xyz;
      if (v3dist(pos, bullet_pos) < 2.f) {
        entity_remove(game, *bullet_id);
        e->tint.r -= 0.025f;
        if (e->tint.r <= 0.0f) {
          entity_remove(game, e->id);
        }
        return;
      }
    }
  }
}

static void _wizard_baddies(Game game, entity_t* e, float dt) {
  static float timer = WIZARD_BADDY_SPAWN_RATE;// *3.f;
  timer -= dt;
  if (timer < 0) {
    timer = WIZARD_BADDY_SPAWN_RATE;
    float theta = game->scene_time * 8483.f;
    vec3 offset = v23xz(v2heading(theta));
    offset = v3scale(offset, 40.f + (cosf(theta * 52.f) + 1) * 20.f);
    vec3 pos = v3add(e->pos, offset);
    pos.y = 1.f;
    entity_add(game, &(entity_t) {
      .pos = pos,
      .model = &game->demo->models.box,
      .material = game->demo->materials.crate,
      .transform = m4ts(pos, 2.f),
      .tint = v3f(1.0f, 0.6f, 0.6f),
      .render = render_pbr,
      .behavior = behavior_baddy,
    });
  }
}

void behavior_wizard(Game game, entity_t* e, float dt) {
  _wizard_movement(game, e, dt);
  _wizard_projectile(game, e, dt);
  _wizard_baddies(game, e, dt);
}

////////////////////////////////////////////////////////////////////////////////
// Monumnet behaviors
////////////////////////////////////////////////////////////////////////////////

extern slotkey_t monument_light_left;
extern slotkey_t monument_light_right;
extern slotkey_t monument_light_spot;
#define PLANE_SPEED_MIN 30.f
void behavior_camera_monument(Game game, entity_t* e, float dt) {
  UNUSED(e);

  float xrot = d2r(-game->input.mouse.move.y * 180.f / (float)game->window.h);
  float yrot = d2r(-game->input.mouse.move.x * 180.f / (float)game->window.x);

  vec2 rotation = v2f(xrot, yrot);
  if (v2mag(rotation) > 0.01f) {
    rotation = v2norm(rotation);
    rotation = v2scale(rotation, dt * 1.8f);
    rotation.x *= -1.f;
    e->pos = v3add(e->pos, v23xz(rotation));
  }

  if (v3mag(e->pos) > 0.01f) {
    vec3 half = v3scale(e->pos, 0.6f * dt);
    camera_rotate_local(&game->camera, half);
    e->pos = v3sub(e->pos, half);
  }

  if (input_triggered(IN_CLICK)) {
    input_pointer_lock();
  }

  float speed = e->angle;

  vec3 direction = game->camera.front.xyz;
  speed += v3dot(direction, v3down) * dt * 9.8f;
  if (speed < PLANE_SPEED_MIN) {
    speed = PLANE_SPEED_MIN;
  }

  direction = v3scale(game->camera.front.xyz, speed * dt);
  game->camera.pos.xyz = v3add(game->camera.pos.xyz, direction);

  vec3 left = v3cross(game->camera.up.xyz, game->camera.front.xyz);
  left = v3rescale(left, 18.f);
  vec3 right = v3scale(left, -1.f);
  light_t* light_left = light_ref(monument_light_left);
  light_t* light_right = light_ref(monument_light_right);
  light_t* light_spot = light_ref(monument_light_spot);
  if (light_left) {
    light_left->pos = v3add(game->camera.pos.xyz, left);
  }
  if (light_right) {
    light_right->pos = v3add(game->camera.pos.xyz, right);
  }
  if (light_spot) {
    light_spot->pos = game->camera.pos.xyz;
    light_spot->dir = game->camera.front.xyz;
  }

  e->angle = speed;
}

////////////////////////////////////////////////////////////////////////////////
// Toggles the visibility of the grid
////////////////////////////////////////////////////////////////////////////////

void behavior_grid_toggle(Game game, entity_t* e, float dt) {
  UNUSED(dt);
  if (input_triggered(IN_TOGGLE_GRID)) {
    e->is_hidden = !e->is_hidden;
  }

  if (input_pressed(IN_CLOSE)) {
    game->should_exit = true;
  }

  if (input_triggered(IN_LEVEL_1)) {
    game->next_scene = 0;
  }

  if (input_triggered(IN_LEVEL_2)) {
    game->next_scene = 1;
  }

  if (input_triggered(IN_LEVEL_3_1)) {
    game->next_scene = 2;
  }

  if (input_triggered(IN_LEVEL_3_2)) {
    game->next_scene = 3;
  }

  if (input_triggered(IN_LEVEL_3_3)) {
    game->next_scene = 4;
  }

  if (input_triggered(IN_LEVEL_3_4)) {
    game->next_scene = 5;
  }

  if (input_triggered(IN_LEVEL_3_5)) {
    game->next_scene = 6;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Cube spinning behavior
////////////////////////////////////////////////////////////////////////////////

void behavior_cubespin(Game game, entity_t* e, float dt) {
  UNUSED(game);

  e->transform = m4translation(e->pos);

  e->transform = m4mul
  ( e->transform
  , m4rotation(v3norm(v3f(1.f, 1.5f, -.7f)), e->angle)
  );

  e->transform = m4mul
  ( e->transform
  , m4rotation(v3norm(v3f(-4.f, 1.5f, 1.f)), e->angle/3.6f)
  );

  e->angle += 2 * dt;
}

////////////////////////////////////////////////////////////////////////////////
// Clockwise spin around the Z axis
////////////////////////////////////////////////////////////////////////////////

void behavior_gear_rotate_cw(Game game, entity_t* e, float dt) {
  UNUSED(game);
  e->transform = m4mul(e->transform, m4rotation(v3front, dt));
}

////////////////////////////////////////////////////////////////////////////////
// Counter-clockwise spin around the Z axis
////////////////////////////////////////////////////////////////////////////////

void behavior_gear_rotate_ccw(Game game, entity_t* e, float dt) {
  UNUSED(game);
  e->transform = m4mul(e->transform, m4rotation(v3front, -dt));
}

////////////////////////////////////////////////////////////////////////////////
// Orients the entity to face the camera along its local +Z axis
////////////////////////////////////////////////////////////////////////////////

void behavior_stare(Game game, entity_t* e, float dt) {
  UNUSED(dt);
  e->transform = m4look(e->pos, game->camera.pos.xyz, v3y);
}

////////////////////////////////////////////////////////////////////////////////
// Sets the location of the entity to the light position
////////////////////////////////////////////////////////////////////////////////

extern slotkey_t editor_light_bright;
extern slotkey_t editor_light_gizmo;
void behavior_attach_to_light(Game game, entity_t* e, float dt) {
  UNUSED(dt);
  e->transform = m4translation(game->demo->light_pos.xyz);
  light_t* light = light_ref(editor_light_bright);
  if (!light) return;
  light->pos = game->demo->light_pos.xyz;
  light->dir = v3sub(game->demo->target, game->demo->light_pos.xyz);
  light = light_ref(editor_light_gizmo);
  light->pos = game->demo->target;
}

////////////////////////////////////////////////////////////////////////////////
// Sets the location of the entity to the camera target position
////////////////////////////////////////////////////////////////////////////////

void behavior_attach_to_camera_target(Game game, entity_t* e, float dt) {
  UNUSED(dt);
  e->transform = m4translation(game->demo->target);

  if (input_triggered(IN_CREATE_OBJECT)) {
    Material mats[] = {
      game->demo->materials.crate,
      game->demo->materials.mudds,
      game->demo->materials.grass,
      game->demo->materials.tiles,
      game->demo->materials.sands,
      game->demo->materials.renderite,
    };

    entity_add(game, &(entity_t) {
      .model = &game->demo->models.box,
      .material = mats[(uint)e->transform.f[12] % 6],
      .transform = m4mul(e->transform, m4scalar(10.0f)),
      .render = render_pbr,
    });
  }
}

////////////////////////////////////////////////////////////////////////////////
// Render function for un-shaded objects with vertex color
////////////////////////////////////////////////////////////////////////////////

void render_basic(Game game, entity_t* e) {
  Shader shader = game->demo->shaders.basic;
  shader_bind(shader);

  int loc_pvm = shader_uniform_loc(shader, "in_pvm_matrix");

  mat4 pvm = m4mul(game->camera.projview, e->transform);
  glUniformMatrix4fv(loc_pvm, 1, GL_FALSE, pvm.f);

  model_render(e->model);
}

void render_basic2(renderer_t* renderer, Game game, entity_t* e) {
  UNUSED(renderer);
  render_basic(game, e);
}

////////////////////////////////////////////////////////////////////////////////
// Renders the debug objects
////////////////////////////////////////////////////////////////////////////////

void render_debug(Game game, entity_t* e) {
  render_basic(game, e);
  draw_render();
}

void render_debug2(renderer_t* renderer, Game game, entity_t* e) {
  UNUSED(renderer);
  render_debug(game, e);
}

void render_debug3(renderer_t* renderer, Game game) {
  UNUSED(renderer);
  UNUSED(game);
  draw_render();
}

////////////////////////////////////////////////////////////////////////////////
// Render function for physically-based lighting
////////////////////////////////////////////////////////////////////////////////

void render_pbr(Game game, entity_t* e) {
  Shader shader = game->demo->shaders.light;
  shader_bind(shader);

  // Per-object properties

  // Model positioning
  int loc_pvm = shader_uniform_loc(shader, "in_pvm_matrix");

  // Material properties
  int loc_sampler_tex = shader_uniform_loc(shader, "texSamp");
  int loc_sampler_norm = shader_uniform_loc(shader, "normSamp");
  int loc_sampler_rough = shader_uniform_loc(shader, "roughSamp");
  int loc_sampler_metal = shader_uniform_loc(shader, "metalSamp");
  int loc_norm = shader_uniform_loc(shader, "in_normal_matrix");
  int loc_props = shader_uniform_loc(shader, "in_weights");
  int loc_tint = shader_uniform_loc(shader, "in_tint");

  mat4 pvm = m4mul(game->camera.projview, e->transform);
  mat4 norm = m4transpose(m4inverse(m4mul(game->camera.view, e->transform)));

  glUniform3fv(loc_tint, 1, e->tint.f);

  glUniformMatrix4fv(loc_pvm, 1, 0, pvm.f);
  glUniformMatrix4fv(loc_norm, 1, 0, norm.f);
  glUniform3fv(loc_props, 1, e->material->weights.f);

  //glUniform4fv(loc_light_pos, 1, demo->light_pos.f);
  //glUniform4fv(loc_camera_pos, 1, game->camera.pos.f);

  //GLint use_color = false;
  //if (e->model->type == MODEL_OBJ) use_color = e->model->mesh.use_color;
  //glUniform1i(loc_use_vert_color, use_color);

  tex_apply(e->material->map_diffuse, 0, loc_sampler_tex);
  tex_apply(e->material->map_normals, 1, loc_sampler_norm);
  tex_apply(e->material->map_roughness, 2, loc_sampler_rough);
  tex_apply(e->material->map_metalness, 3, loc_sampler_metal);

  model_render(e->model);

  glBindTexture(GL_TEXTURE_2D, 0);
}

typedef struct pbr_instance_attributes_t {
  mat4 transform;
  vec3 tint;
} pbr_instance_attributes_t;

void render_pbr_register(entity_t* e, Game game) {
  UNUSED(game);
  assert(e->renderer == renderer_pbr);
  assert(e->model->type == MODEL_CUBE);

  // get associated render group
  render_group_key_t key = { e->model, e->material, e->is_static };
  res_ensure_rg_t group_slot = map_rg_ensure(e->renderer->groups, key);

  if (group_slot.is_new) {
    *group_slot.value = (render_group_t){
      .instances = pmap_new(pbr_instance_attributes_t),
      .material = e->material,
      .model = e->model,
      .is_static = e->is_static,
    };
  }

  // add instance to the group and save its instance id
  e->render_id = pmap_insert(group_slot.value->instances, 
    &(pbr_instance_attributes_t) {
      .transform = e->transform,
      .tint = e->tint,
    }
  );

  ((render_group_t*)group_slot.value)->needs_update = true;
}

void render_pbr_unregister(entity_t* e) {
  assert(e->renderer == renderer_pbr);
  render_group_key_t key = { e->model, e->material, e->is_static };
  render_group_t* group = map_rg_ref(e->renderer->groups, key);
  if (!group) return;
  pmap_remove(group->instances, e->render_id);
  group->needs_update = true;
  e->renderer = NULL;
  e->render_id = SK_NULL;
}

void render_pbr2(renderer_t* renderer, Game game, entity_t* e) {
  UNUSED(renderer);
  UNUSED(game);
  UNUSED(e);
  //render_pbr(game, e);
}

void render_pbr3(renderer_t* renderer, Game game) {
  if (!renderer->groups || !renderer->groups->size) return;
  Shader shader = renderer->shader;
  shader_bind(shader);

  // Material properties
  int loc_sampler_tex = shader_uniform_loc(shader, "texSamp");
  int loc_sampler_norm = shader_uniform_loc(shader, "normSamp");
  int loc_sampler_rough = shader_uniform_loc(shader, "roughSamp");
  int loc_sampler_metal = shader_uniform_loc(shader, "metalSamp");
  int loc_props = shader_uniform_loc(shader, "in_weights");

  tex_apply(game->demo->materials.mudds->map_diffuse, 0, loc_sampler_tex);
  tex_apply(game->demo->materials.mudds->map_normals, 1, loc_sampler_norm);
  tex_apply(game->demo->materials.mudds->map_roughness, 2, loc_sampler_rough);
  tex_apply(game->demo->materials.mudds->map_metalness, 3, loc_sampler_metal);
  glUniform3fv(loc_props, 1, game->demo->materials.mudds->weights.f);

  // Shared uniforms
  int loc_pv = shader_uniform_loc(shader, "in_pv_matrix");
  int loc_view = shader_uniform_loc(shader, "in_view_matrix");
  glUniformMatrix4fv(loc_pv, 1, 0, game->camera.projview.f);
  glUniformMatrix4fv(loc_view, 1, 0, game->camera.view.f);

  // Render each model group in a batch
  render_group_t* map_foreach(group, renderer->groups) {
    if (!group->instances || group->instances->size == 0) continue;

    // create the vao and instance buffer handle on first run
    if (!group->vao) {
      glGenVertexArrays(1, &group->vao);
      glBindVertexArray(group->vao);

      model_bind(group->model);

      glGenBuffers(1, &group->instance_buffer);
      glBindBuffer(GL_ARRAY_BUFFER, group->instance_buffer);
      glBufferData(GL_ARRAY_BUFFER
      , group->instances->size_bytes
      , group->instances->begin
      , group->is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW
      );

      group->needs_update = false;

      GLsizei stride = sizeof(pbr_instance_attributes_t);
      glEnableVertexAttribArray(5); // Transform
      glEnableVertexAttribArray(6);
      glEnableVertexAttribArray(7);
      glEnableVertexAttribArray(8);
      //glEnableVertexAttribArray(9); // Tint color
      glVertexAttribPointer(5, 4, GL_FLOAT, false, stride, (void*)0);
      glVertexAttribPointer(6, 4, GL_FLOAT, false, stride, (void*)(1*v4bytes));
      glVertexAttribPointer(7, 4, GL_FLOAT, false, stride, (void*)(2*v4bytes));
      glVertexAttribPointer(8, 4, GL_FLOAT, false, stride, (void*)(3*v4bytes));
      //glVertexAttribPointer(9, 3, GL_FLOAT, false, stride, (void*)(4*v4bytes));
      glVertexAttribDivisor(5, 1);
      glVertexAttribDivisor(6, 1);
      glVertexAttribDivisor(7, 1);
      glVertexAttribDivisor(8, 1);
      //glVertexAttribDivisor(9, 1);

      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0);
    }

    // update the instance buffer if items have changed
    if (group->needs_update) {
      glBindBuffer(GL_ARRAY_BUFFER, group->instance_buffer);
      glBufferData(GL_ARRAY_BUFFER
      , group->instances->size_bytes
      , group->instances->begin
      , group->is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW
      );
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      group->needs_update = false;
    }

    // set up render and draw call
    glBindVertexArray(group->vao);

    if (group->model->index_count) {
      // indexed draw
      glDrawElementsInstanced
      ( GL_TRIANGLES
      , (GLsizei)group->model->vert_count
      , GL_UNSIGNED_INT
      , 0
      , (GLsizei)group->instances->size
      );
    }
    else {
      // non-indexed draw
      glDrawArraysInstanced
      ( GL_TRIANGLES
      , 0
      , (GLsizei)group->model->vert_count
      , (GLsizei)group->instances->size
      );
    }

    glBindVertexArray(0);
  }
}
