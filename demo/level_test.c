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

#include <math.h>

#include "light.h"

void level_load_gears(Game game) {

  demo_t* demo = game->demo;

  game->camera.pos = v4f(3, 2, 45, 1);
  game->camera.front = v4front;
  game->demo->target = v3origin;

  camera_look_at(&game->camera, game->demo->target);

  // Debug Renderer
  game_entity_add(game, &(entity_t) {
    .model = &demo->models.grid,
    .transform = m4identity,
    .render = render_debug,
    .behavior = behavior_grid_toggle,
  });

  // Camera Controller
  game_entity_add(game, &(entity_t) {
    .behavior = behavior_camera_test,
  });

  //* Spinny Cube
  game_entity_add(game, &(entity_t) {
    .model = &demo->models.color_cube,
    .pos = v3f(-2, 0, 0),
    .angle = 0,
    .transform = m4identity,
    .render = render_basic,
    .behavior = behavior_cubespin,
  }); //*/

  //* Staring Cube
  game_entity_add(game, &(entity_t) {
    .model = &demo->models.color_cube,
    .pos = v3f(0, 0, 2),
    .render = render_basic,
    .behavior = behavior_stare,
  }); //*/

  //* Gizmos
  game_entity_add(game, &(entity_t) {
    .model = &demo->models.gizmo,
    .render = render_basic,
    .behavior = behavior_attach_to_camera_target,
  }); //*/

  game_entity_add(game, &(entity_t) {
    .model = &demo->models.gizmo,
    .render = render_basic,
    .behavior = behavior_attach_to_light,
  }); //*/

  //* Gear 1
  game_entity_add(game, &(entity_t) {
    .model = &demo->models.gear,
    .material = demo->materials.grass,
    .transform = m4translation(v3f(0, 7, -12)),
    .render = render_pbr,
    .behavior = behavior_gear_rotate_cw,
  }); //*/

  //* Gear 2
  game_entity_add(game, &(entity_t) {
    .model = &demo->models.gear,
    .material = demo->materials.sands,
    .transform = m4translation(v3f(20.5f, -1.5f, -12)),
    .render = render_pbr,
    .behavior = behavior_gear_rotate_ccw,
  }); //*/

  //* Gear 3
  game_entity_add(game, &(entity_t) {
    .model = &demo->models.gear,
    .material = demo->materials.mudds,
    .transform = m4translation(v3f(43.f, -1.5f, -12)),
    .render = render_pbr,
    .behavior = behavior_gear_rotate_cw,
  }); //*/

  //* Crate
  game_entity_add(game, &(entity_t) {
    .model = &demo->models.box,
    .material = demo->materials.grass,
    .transform = m4translation(v3f(0, -0.5, 0)),
    .render = render_pbr,
  }); //*/

  //* Crate
  game_entity_add(game, &(entity_t) {
    .model = &demo->models.box,
    .material = demo->materials.grass,
    .transform = m4translation(v3f(1, -0.5, 0)),
    .render = render_pbr,
  }); //*/

  //* Crate
  game_entity_add(game, &(entity_t) {
    .model = &demo->models.box,
    .material = demo->materials.grass,
    .transform = m4translation(v3f(0, -0.5, 1)),
    .render = render_pbr,
  }); //*/

  //* Crate
  game_entity_add(game, &(entity_t) {
    .model = &demo->models.box,
    .material = demo->materials.grass,
    .transform = m4translation(v3f(1, -0.5, 1)),
    .render = render_pbr,
  }); //*/

  //* Bigger Crate
  game_entity_add(game, &(entity_t) {
    .model = &demo->models.box,
    .material = demo->materials.crate,
    .transform = m4mul(m4translation(v3f(2, 0, 0)), m4scalar(2)),
    .render = render_pbr,
  }); //*/

  //* Even Bigger Crate
  game_entity_add(game, &(entity_t) {
    .model = &demo->models.box,
    .material = demo->materials.renderite,
    .tint = v3f(0.8f, 0.3f, 0.6f),
    .transform = m4mul(m4translation(v3f(5, 0.5, 0)), m4scalar(3)),
    .render = render_pbr,
  }); //*/

  //* LORGE Cube(s)
  for (int j = -2; j < 3; ++j) {
    for (int i = -2; i < 3; ++i) {
      float x = 23.0f * i;
      float y = 23.0f * j;
      float angle = 0.02f; // 3.0f / (x * y);
      vec3 axis = v3norm(v3f(cosf(y), 1, cosf(x)));
      vec3 pos = v3f(x, -10.5, y);
      if (!(i == 0 && i == j)) {
        angle = 0.f;
        pos.y -= v3mag(pos) / 10.f;
      }

      Material material = demo->materials.tiles;

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
        material = j % 2 == 0 ? demo->materials.renderite : demo->materials.mudds;
      }

      game_entity_add(game, &(entity_t) {
        .model = &demo->models.box,
        .material = material,
        .transform =
          m4mul(
            m4mul(m4translation(pos), m4rotation(axis, angle)),
            m4scalar(19)
          ),
        .render = render_pbr,
      });

    }
  } //*/

  //* Some lights
  light_add((light_t) {
    .intensity = 60000.0f,
    .pos = v3f(40, 30, 50),
    .color = v3f(0.9f, 0.9f, 0.75f),
  });

  light_add((light_t) {
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
}

////////////////////////////////////////////////////////////////////////////////

void level_load_monument(Game game) {

  demo_t* demo = game->demo;

  game->camera.pos = v4f(3, 9, 5, 1);
  game->camera.front = v4front;
  game->demo->target = v3origin;

  camera_look_at(&game->camera, game->demo->target);

  // Debug Renderer
  game_entity_add(game, &(entity_t) {
    .model = &demo->models.grid,
    .transform = m4identity,
    .render = render_debug,
    .behavior = behavior_grid_toggle,
  });
  
  // Target gizmo
  game_entity_add(game, &(entity_t) {
    .model = &demo->models.gizmo,
    .render = render_basic,
    .behavior = behavior_click_ground,
  });

  // Wizard crate
  game_entity_add(game, &(entity_t) {
    .pos = v3f(0, 0.5, 0),
    .model = &demo->models.box,
    .material = demo->materials.crate,
    .behavior = behavior_wizard,
    .render = render_pbr,
  });

  // Ground tiles
  float ext = 20.0f;
  for (float y = -ext; y < ext; ++y) {
    for (float x = -ext; x < ext; ++x) {
      vec3 pos = v3f(5.f * x, -1.01f, 5.f * y);
      Material material = demo->materials.grass;

      if (x + y > 10 && y > 6)
        material = demo->materials.sands;
      else if (v3mag(v3sub(pos, v3f(-70, -1, -30))) < 40)
        material = demo->materials.tiles;
      else if (x < 8 && y > 8)
        material = demo->materials.renderite;
      else if
      (  ( x > 10 && y < 5 )
      && (  ((int)x + (int)y) % 7 == 0
         || ((int)x - (int)y) % 7 == 0
         )
      )
        material = demo->materials.mudds;

      game_entity_add(game, &(entity_t) {
        .model = &demo->models.box,
        .material = material,
        .transform = m4mul(
          m4translation(pos), m4vscalar(v3f(5, 2, 5))
        ),
        .render = render_pbr,
      });
    }
  }

  // Sun
  light_add((light_t) {
    .intensity = 100000.0f,
    .pos = v3f(40, 120, -50),
    .color = v3f(0.9f, 0.9f, 0.75f),
  });

}
