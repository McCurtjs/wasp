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

#define PLANE_SPEED_MIN 30.f

slotkey_t monument_light_left;
slotkey_t monument_light_right;
slotkey_t monument_light_spot;

static float plane_speed = PLANE_SPEED_MIN;

////////////////////////////////////////////////////////////////////////////////

void _behavior_camera_monument(Game game, entity_t* e, float dt) {
  UNUSED(e);

  if (game->scene_time == 0) {
    plane_speed = PLANE_SPEED_MIN;
  }

  vec2 window = v2vi(game->window);

  float xrot = d2r( game->input.mouse.move.y * 180.f / window.h);
  float yrot = d2r(-game->input.mouse.move.x * 180.f / window.x);
  float epsilon = 0.000001f;

  if (game->input.touch.count == 1) {
    xrot = d2r( game->input.touch.first->move.y * 180.f / window.h);
    yrot = d2r(-game->input.touch.first->move.x * 180.f / window.w);
  }

  vec2 rotation = v2f(xrot, yrot);
  if (v2mag(rotation) > epsilon) {
    rotation = v2norm(rotation);
    rotation = v2scale(rotation, dt * 1.8f);
    rotation.x *= -1.f;
    entity_translate(e, v23xz(rotation));
  }

  if (v3mag(e->pos) > epsilon) {
    vec3 half = v3scale(e->pos, 0.6f * dt);
    camera_rotate_local(&game->camera, half);
    entity_set_position(e, v3sub(e->pos, half));
  }

  if (input_triggered(IN_CLICK)) {
    input_pointer_lock();
  }

  if (input_triggered(IN_INCREASE)) {
    game->demo->monument_extent += 1;
    game->next_scene = game->scene;
  }

  if (input_triggered(IN_DECREASE)) {
    game->demo->monument_extent -= 1;
    game->next_scene = game->scene;
  }

  if (input_triggered(IN_INCREASE_FAST)) {
    game->demo->monument_extent += 10;
    game->next_scene = game->scene;
  }

  if (input_triggered(IN_DECREASE_FAST)) {
    game->demo->monument_extent -= 10;
    game->next_scene = game->scene;
  }

  if (game->next_scene == game->scene) {
    str_log("[Game.monument] Extent: {}", game->demo->monument_extent);
  }

  vec3 direction = game->camera.front;
  plane_speed += v3dot(direction, v3down) * dt * 9.8f;
  if (plane_speed < PLANE_SPEED_MIN) {
    plane_speed = PLANE_SPEED_MIN;
  }

  direction = v3scale(game->camera.front, plane_speed * dt);
  game->camera.pos = v3add(game->camera.pos, direction);

  vec3 left = v3cross(game->camera.up, game->camera.front);
  left = v3rescale(left, 18.f);
  vec3 right = v3neg(left);
  light_t* light_left = light_ref(monument_light_left);
  light_t* light_right = light_ref(monument_light_right);
  light_t* light_spot = light_ref(monument_light_spot);
  if (light_left) {
    light_left->pos = v3add(game->camera.pos, left);
  }
  if (light_right) {
    light_right->pos = v3add(game->camera.pos, right);
  }
  if (light_spot) {
    light_spot->pos = game->camera.pos;
    light_spot->dir = game->camera.front;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Sun-gear rotation around the Y axis
////////////////////////////////////////////////////////////////////////////////

void _behavior_gear_rotate_sun(Game game, entity_t* e, float dt) {
  UNUSED(game);
  entity_rotate_a(e, v3down, dt);
}

////////////////////////////////////////////////////////////////////////////////
// Scene unload function returned by the scene loader. Unlocks the cursor
////////////////////////////////////////////////////////////////////////////////

void _scene_unload_monument(Game game) {
  UNUSED(game);
  input_pointer_unlock();
}

////////////////////////////////////////////////////////////////////////////////
// Loading function to initialize the scene
////////////////////////////////////////////////////////////////////////////////

scene_unload_fn_t scene_load_monument(Game game) {

  demo_t* demo = game->demo;

  game->camera.pos = v3f(0, 0, 1);
  game->camera.front = v3front;
  game->demo->target = v3origin;

  input_pointer_lock();

  camera_look_at(&game->camera, game->demo->target);

  // Debug Renderer
  entity_add(&(entity_desc_t) {
    .model = demo->models.grid,
    .onrender = render_debug,
    .is_hidden = true,
    .behavior = behavior_grid_toggle,
  });

  // Camera Controller
  entity_add(&(entity_desc_t) {
    .pos = v3zero, // accumulated rotation angles
    .behavior = _behavior_camera_monument,
  });

  float offset = 400.f;

  // Big monumental cubes (they give you flying and indestructible)
  float ext = (float)demo->monument_extent;
  float size = (float)demo->monument_size;
  for (float z = -ext; z < ext; ++z) {
    for (float y = -ext; y < ext; ++y) {
      for (float x = -ext; x < ext; ++x) {
        vec3 pos = v3f(x, y, z);
        pos = v3scale(pos, size);
        pos.y -= offset;

        if (v3mag(v3sub(pos, v3origin)) < 0.5f) {
          continue;
        }

        entity_add(&(entity_desc_t) {
          .model = demo->models.box,
          .material = demo->materials.mudds,
          .tint = b4white,
          .pos = pos,
          .scale = 120.f - 2.f * (y + ext),
          .renderer = renderer_pbr,
          .is_static = true,
        });
      }
    }
  }

  vec3 sun_pos = v3f(200, 1000, 600);

  // Gear sun
  entity_add(&(entity_desc_t) {
    .name = S("Sungear"),
    .model = demo->models.gear,
    .onrender = render_pbr,
    .tint = b4white,
    .material = demo->materials.renderite,
    .pos = v3add(sun_pos, v3f(0, 30, 0)),
    .rot = q4axang(v3x, d2r(-90.f)),
    .scale = 15.f,
    .behavior = _behavior_gear_rotate_sun,
  });

  // Ground
  float ground_scale = 2500;
  entity_add(&(entity_desc_t) {
    .model = demo->models.box,
    .material = demo->materials.grass,
    .tint = b4white,
    .pos = v3f(0, -(ext * size + size) - (ground_scale / 2.f) - offset, 0),
    .scale = ground_scale,
    .onrender = render_pbr,
  });

  // Light 1 is left indicator (red)
  monument_light_left = light_add((light_t) {
    .intensity = 4000.0f,
    .pos = v3f(-18, 0, 0),
    .color = v3f(1.0f, 0.3f, 0.3f),
  });

  // Light 2 is right indicator (green)
  monument_light_right = light_add((light_t) {
    .intensity = 4000.0f,
    .pos = v3f(18, 0, 0),
    .color = v3f(0.3f, 1.0f, 0.3f),
  });

  // Light 2 is right indicator (green)
  monument_light_spot = light_add((light_t) {
    .intensity = 10000.0f,
    .pos = v3origin,
    .color = v3f(1.0f, 1.0f, 0.7f),
    .dir = v3front,
    .spot_inner = 0.9f,
    .spot_outer = 0.85f,
  });

  // Sun
  light_add((light_t) {
    .intensity = 10000000.0f,
    .pos = sun_pos,
    .color = v3f(0.9f, 0.9f, 0.75f),
  });

  return _scene_unload_monument;
}
