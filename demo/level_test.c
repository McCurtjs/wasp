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

#include <math.h>

#include "light.h"

slotkey_t editor_light_bright;
slotkey_t editor_light_gizmo;
scene_unload_fn_t scene_load_gears(Game game) {

  demo_t* demo = game->demo;

  game->camera.pos = v4f(3, 2, 45, 1);
  game->camera.front = v4front;
  game->demo->target = v3origin;
  game->demo->light_pos = v4f(40, 60, 0, 1);

  camera_look_at(&game->camera, game->demo->target);

  // Debug Renderer
  entity_add(&(entity_desc_t) {
    .name = S("Grid"),
    .model = demo->models.grid,
    .onrender = render_debug,
    .behavior = behavior_grid_toggle,
  });

  // Camera Controller
  entity_add(&(entity_desc_t) {
    .name = S("Camera controller"),
    .behavior = behavior_camera_test,
  });

  //* Spinny Cube
  entity_add(&(entity_desc_t) {
    .name = S("Spinny-cube"),
    .model = demo->models.color_cube,
    .pos = v3f(-2, 0, 0),
    .onrender = render_basic,
    .behavior = behavior_cubespin,
  }); //*/

  //* Staring Cube
  entity_add(&(entity_desc_t) {
    .name = S("Staring Cube"),
    .model = demo->models.color_cube,
    .pos = v3f(0, 0, 2),
    .onrender = render_basic,
    .behavior = behavior_stare,
  }); //*/

  //* Gizmos
  entity_add(&(entity_desc_t) {
    .name = S("Target Gizmo"),
    .model = demo->models.gizmo,
    .onrender = render_basic,
    .behavior = behavior_attach_to_camera_target,
  }); //*/

  entity_add(&(entity_desc_t) {
    .name = S("Light Gizmo"),
    .model = demo->models.gizmo,
    .onrender = render_basic,
    .behavior = behavior_attach_to_light,
  }); //*/

  //* Gear 1
  entity_add(&(entity_desc_t) {
    .name = S("Gear 1"),
    .model = demo->models.gear,
    .material = demo->materials.grass,
    .pos = v3f(0, 7, -12),
    .onrender = render_pbr,
    .behavior = behavior_gear_rotate_cw,
  }); //*/

  //* Gear 2
  entity_add(&(entity_desc_t) {
    .name = S("Gear 2"),
    .model = demo->models.gear,
    .material = demo->materials.sands,
    .pos = v3f(20.5f, -1.5f, -12),
    .onrender = render_pbr,
    .behavior = behavior_gear_rotate_ccw,
  }); //*/

  //* Gear 3
  entity_add(&(entity_desc_t) {
    .name = S("Gear 3"),
    .model = demo->models.gear,
    .material = demo->materials.mudds,
    .pos = v3f(43.f, -1.5f, -12),
    .onrender = render_pbr,
    .behavior = behavior_gear_rotate_cw,
  }); //*/

  //* Crate
  entity_add(&(entity_desc_t) {
    .name = S("Grass Block 1"),
    .model = demo->models.box,
    .material = demo->materials.grass,
    .pos = v3f(0, -0.5, 0),
    .onrender = render_pbr,
  }); //*/

  //* Crate
  entity_add(&(entity_desc_t) {
    .name = S("Grass Block 2"),
    .model = demo->models.box,
    .material = demo->materials.grass,
    .pos = v3f(1, -0.5, 0),
    .onrender = render_pbr,
  }); //*/

  //* Crate
  entity_add(&(entity_desc_t) {
    .name = S("Grass Block 3"),
    .model = demo->models.box,
    .material = demo->materials.grass,
    .pos = v3f(0, -0.5, 1),
    .onrender = render_pbr,
  }); //*/

  //* Crate
  entity_add(&(entity_desc_t) {
    .name = S("Grass Block 4"),
    .model = demo->models.box,
    .material = demo->materials.grass,
    .pos = v3f(1, -0.5, 1),
    .onrender = render_pbr,
  }); //*/

  //* Bigger Crate
  entity_add(&(entity_desc_t) {
    .name = S("Medium Crate"),
    .model = demo->models.box,
    .material = demo->materials.crate,
    .pos = v3f(2, 0, 0),
    .scale = 2.0f,
    .onrender = render_pbr,
  }); //*/

  //* Even Bigger Crate
  entity_add(&(entity_desc_t) {
    .name = S("Big Crate"),
    .model = demo->models.box,
    .material = demo->materials.renderite,
    .tint = v3f(0.8f, 0.3f, 0.6f),
    .pos = v3f(5, 0.5f, 0),
    .scale = 3.f,
    .onrender = render_pbr,
  }); //*/

  //* LORGE Cube(s)
  for (int j = -2; j < 3; ++j) {
    for (int i = -2; i < 3; ++i) {
      float x = 23.0f * i;
      float y = 23.0f * j;
      float angle = 0.02f; // 3.0f / (x * y);
      vec3 axis = v3norm(v3f(cosf(y), 1, sinf(x)));
      vec3 pos = v3f(x, -10.5, y);
      if (!(i == 0 && i == j)) {
        angle = 0.f;
        pos.y -= v3mag(pos) / 10.f;
      }

      Material material = demo->materials.tiles;
      /*
      if (i == 2 && j == 1) {
        axis = v3up;
        angle = 3.14159f;// / 4.0f;
        material = demo->materials.mudds;
      }
      else if (i == j) {
        material = demo->materials.grass;
      }
      else if (i == -j) {
        material = demo->materials.sands;
      }
      else if (i % 2 == 0) {
        material = j % 2 == 0 ?
          demo->materials.renderite : demo->materials.mudds;
      }
      /*/
      material = demo->materials.atlas;
      //*/

      entity_add(&(entity_desc_t) {
        .name = S("Ground Cube"),
        .model = demo->models.box,
        .material = material,
        .pos = pos,
        .rot = q4axang(axis, angle),
        .scale = 19.0f,
        .renderer = renderer_pbr,
      });

    }
  } //*/

  //* Some lights
  editor_light_bright = light_add((light_t) {
    .intensity = 60000.0f,
    .pos = v3f(40, 30, 50),
    .color = v3f(0.9f, 0.9f, 0.75f),
    .dir = v3sub(demo->target, v3f(40, 30, 50)),
    .spot_outer = 0.8f,
    .spot_inner = 0.9f,
  });

  editor_light_gizmo = light_add((light_t) {
    .intensity = 50.0f,
    .pos = v3f(4, 3, 5),
    .color = v3f(0.8f, 0.8f, 0.95f),
  });

  light_add((light_t) {
    .intensity = 700.0f,
    .pos = v3f(20, 7, 20),
    .color = v3f(1.0f, 0.2f, 0.2f),
  });

  light_add((light_t) {
    .intensity = 400.0f,
    .pos = v3f(-20, 7, -20),
    .color = v3f(0.0f, 0.9f, 0.4f),
  });
  //*/

  return NULL;
}

////////////////////////////////////////////////////////////////////////////////

scene_unload_fn_t scene_load_wizard(Game game) {

  demo_t* demo = game->demo;

  game->camera.pos = v4f(3, 9, 5, 1);
  game->camera.front = v4front;
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
    .tint = c4black.rgb,
    .material = demo->materials.renderite,
    .rot = q4axang(v3x, d2r(90.f)),
    .scale = 0.06f,
    .behavior = behavior_wizard_level,
  });

  // Wizard crate
  entity_add(&(entity_desc_t) {
    .name = S("Player"),
    .pos = v3f(0, 0.5, 0),
    .model = demo->models.box,
    .material = demo->materials.crate,
    .behavior = behavior_wizard,
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

////////////////////////////////////////////////////////////////////////////////

#include "input.h"

void scene_unload_monument(Game game) {
  UNUSED(game);
  input_pointer_unlock();
}

slotkey_t monument_light_left;
slotkey_t monument_light_right;
slotkey_t monument_light_spot;

scene_unload_fn_t scene_load_monument(Game game) {

  demo_t* demo = game->demo;

  game->camera.pos = v4f(0, 0, 1, 1);
  game->camera.front = v4front;
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
    .behavior = behavior_camera_monument,
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
    .tint = c4white.rgb,
    .material = demo->materials.renderite,
    .pos = v3add(sun_pos, v3f(0, 30, 0)),
    .rot = q4axang(v3x, d2r(-90.f)),
    .scale = 15.f,
    .behavior = behavior_gear_rotate_sun,
  });

  // Ground
  float ground_scale = 2500;
  entity_add(&(entity_desc_t) {
    .model = demo->models.box,
    .material = demo->materials.grass,
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

  return scene_unload_monument;
}
