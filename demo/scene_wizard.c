/*******************************************************************************
* MIT License
*
* Copyright (c) 2026 Curtis McCoy
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
#include "graphics.h"

#include <math.h>

#define WIZARD_SPEED 8.0f
#define WIZARD_FAKE_SPEED (WIZARD_SPEED * 4.0f)
#define WIZARD_FIRE_RATE 0.05f
#define WIZARD_BADDY_SPAWN_RATE 3.0f
#define BADDY_SPEED 4.0f

typedef struct wizard_bullet_t {
  slotkey_t parent_entity_id;
  slotkey_t light_id;
  vec3 pos;
  vec3 velocity;
} wizard_bullet_t;

#define con_type wizard_bullet_t
#define con_prefix bullet
#include "slotmap.h"
#undef con_type
#undef con_prefix

SlotMap_bullet wizard_bullets = NULL;

////////////////////////////////////////////////////////////////////////////////
// Behavior for controlling camera rotation and movement
////////////////////////////////////////////////////////////////////////////////

void _behavior_wizard_level(Game game, entity_t* e, float dt) {
  UNUSED(dt);
  if (input_pressed(IN_CLICK_MOVE) || game->input.touch.count > 0) {
    vec3 ray = camera_ray(&game->camera, game->input.mouse.pos);
    if (game->input.touch.count > 0) {
      ray = camera_ray(&game->camera, game->input.touch.first->pos);
    }

    float t;
    if (v3ray_plane(game->camera.pos, ray, v3origin, v3up, &t)) {
      game->demo->target = v3add(game->camera.pos, v3scale(ray, t));
    }
  }
  entity_set_position(e, game->demo->target);
  entity_rotate_a(e, v3down, dt);
}

////////////////////////////////////////////////////////////////////////////////

static void _wizard_movement(Game game, entity_t* e, float dt) {
  static vec3 fake_target = svNzero;

  if (game->scene_time == 0.0f) fake_target = v3zero;

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
  if (input_pressed(IN_CLICK) || game->input.touch.count >= 2) {
    max_move /= 2.0f;
  }
  if (distance <= max_move) {
    entity_set_position(e, target);
  }
  else {
    vec3 dir = v3norm(to_target);
    entity_translate(e, v3scale(dir, max_move));
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

  game->camera.pos = v3add(e->pos, v3f(6, 12, 7));

  vec3 cam_to_target = v3dir(fake_target, game->camera.pos);
  vec3 cam_to_wizard = v3dir(e->pos, game->camera.pos);
  //vec3 diff = v3sub(cam_to_target, cam_to_wizard);
  cam_to_target = v3add(cam_to_wizard, v3rescale(cam_to_target, 0.3f));
  cam_to_target = v3norm(cam_to_target);

  game->camera.front = cam_to_target;
}

////////////////////////////////////////////////////////////////////////////////

static void _behavior_projectile(Game game, entity_t* e, float dt) {
  UNUSED(game);
  assert(e);

  wizard_bullet_t* bullet = smap_bullet_ref(wizard_bullets, e->user_id);
  if (!bullet) return;

  entity_translate(e, v3scale(bullet->velocity, dt * 25.0f));

  light_t* light = light_ref(bullet->light_id);
  if (light) {
    light->pos = e->pos;
  }

  if (e->pos.y < 0) {
    entity_remove(e->id);
  }
}

////////////////////////////////////////////////////////////////////////////////

static void _ondelete_projectile(Game game, entity_t* e) {
  UNUSED(game);
  wizard_bullet_t* bullet = smap_bullet_ref(wizard_bullets, e->user_id);
  if (!bullet) return;

  light_remove(bullet->light_id);
  smap_bullet_remove(wizard_bullets, e->user_id);
}

////////////////////////////////////////////////////////////////////////////////

static void _wizard_projectile(Game game, entity_t* e, float dt) {
  demo_t* demo = game->demo;
  static float shot_timer = WIZARD_FIRE_RATE;
  shot_timer += dt;
  if ((input_pressed(IN_CLICK) || game->input.touch.count >= 2)
  &&  shot_timer > WIZARD_FIRE_RATE
  ) {
    vec3 click_ray = camera_ray(&game->camera, game->input.mouse.pos);
    if (game->input.touch.count >= 2) {
      click_ray = camera_ray(&game->camera, game->input.touch.second->pos);
    }

    float t;
    if (v3ray_plane(game->camera.pos, click_ray, v3origin, v3up, &t)) {
      vec3 click_pos = v3add(game->camera.pos, v3scale(click_ray, t));
      click_pos.y = 1.5f;
      vec3 launch_point = v3f(e->pos.x, 2, e->pos.z);

      if (!wizard_bullets) {
        wizard_bullets = smap_bullet_new();
      }

      slotkey_t eid = entity_add(&(entity_desc_t) {
        .pos = launch_point,
        .scale = 0.4f,
        .model = demo->models.color_cube,
        .onrender = render_basic,
        .behavior = _behavior_projectile,
        .ondelete = _ondelete_projectile,
      });
      Entity bullet = entity_ref(eid);

      bullet->user_id = smap_bullet_add(wizard_bullets, (wizard_bullet_t) {
        .parent_entity_id = bullet->id,
        .light_id = light_add((light_t) {
          .color = v3f(1.0f, 0.7f, 0.8f),
          .intensity = 8.0f,
          .pos = launch_point,
        }),
        .pos = launch_point,
        .velocity = v3dir(click_pos, launch_point),
      });
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static void _behavior_baddy(Game game, entity_t* e, float dt) {
  vec3 target = game->demo->target;
  target.y = e->pos.y;
  vec3 dir = v3dir(target, e->pos);
  float move_speed = BADDY_SPEED * dt;
  if (v3mag(dir) <= move_speed) {
    entity_set_position(e, target);
  }
  else {
    entity_translate(e, v3rescale(dir, move_speed));
  }

  if (wizard_bullets && wizard_bullets->size) {
    wizard_bullet_t* smap_foreach(bullet_data, wizard_bullets) {
      Entity bullet = entity_ref(bullet_data->parent_entity_id);
      if (!bullet) continue;
      if (v3dist(e->pos, bullet->pos) < 2.0f) {
        entity_remove(bullet->id);
        entity_translate(e, v3f(0, -0.025f, 0));
        if (e->pos.y <= 0.0f) {
          entity_remove(e->id);
        }
        return;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

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
    entity_add(&(entity_desc_t) {
      .model = game->demo->models.box,
      .material = game->demo->materials.crate,
      .pos = pos,
      .scale = 2.0f,
      .tint = v4cv(v4f(1.0f, 0.6f, 0.6f, 1.0f)),
      .renderer = renderer_pbr,
      .behavior = _behavior_baddy,
    });
  }
}

////////////////////////////////////////////////////////////////////////////////

void _behavior_wizard(Game game, entity_t* e, float dt) {
  _wizard_movement(game, e, dt);
  _wizard_projectile(game, e, dt);
  _wizard_baddies(game, e, dt);
}

////////////////////////////////////////////////////////////////////////////////

scene_unload_fn_t scene_load_wizard(Game game) {

  demo_t* demo = game->demo;

  game->camera.pos = v3f(3, 9, 5);
  game->camera.front = v3front;
  game->demo->target = v3origin;

  camera_look_at(&game->camera, game->demo->target);

  // Debug Renderer
  entity_add(&(entity_desc_t) {
    .name = S("Grid"),
    .model = demo->models.grid,
    .onrender = render_debug,
    .behavior = behavior_grid_toggle,
  });
  
  // Target gizmo
  entity_add(&(entity_desc_t) {
    .name = S("Target"),
    .model = demo->models.gear,
    .onrender = render_pbr,
    .tint = b4black,
    .material = demo->materials.renderite,
    .rot = q4axang(v3x, d2r(90.f)),
    .scale = 0.06f,
    .behavior = _behavior_wizard_level,
  });

  // Wizard crate
  entity_add(&(entity_desc_t) {
    .name = S("Player"),
    .pos = v3f(0, 0.5, 0),
    .model = demo->models.box,
    .material = demo->materials.crate,
    .behavior = _behavior_wizard,
    .renderer = renderer_pbr,
  });

  // Ground tiles
  float ext = 20.0f;
  for (float y = -ext; y < ext; ++y) {
    for (float x = -ext; x < ext; ++x) {
      vec3 pos = v3f(5.f * x, -2.51f, 5.f * y);
      Material material = demo->materials.grass;

      if (x + y > 10 && y > 6)
        material = demo->materials.sands;
      else if (v3mag(v3sub(pos, v3f(-70, -1, -30))) < 40)
        material = demo->materials.tiles;
      else if (x < 8 && y > 8)
        material = demo->materials.renderite;
      else if
      (  ( x > 5 && y < 5 )
      && (  ((int)x + (int)y) % 7 == 0
         || ((int)x - (int)y) % 7 == 0
         )
      )
        material = demo->materials.mudds;

      entity_add(&(entity_desc_t) {
        .model = demo->models.box,
        .material = material,
        .tint = b4white,
        .pos = pos,
        .scale = 5.0f,
        .renderer = renderer_pbr,
        .is_static = true,
      });
    }
  }

  // Sun
  light_add((light_t) {
    .intensity = 100000.0f,
    .pos = v3f(40, 120, -50),
    .color = v3f(0.9f, 0.9f, 0.75f),
  });

  return NULL;
}
