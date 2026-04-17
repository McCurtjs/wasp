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

#define CAMERA_SPEED 0.8f

////////////////////////////////////////////////////////////////////////////////
// Behavior for controlling camera rotation and movement
////////////////////////////////////////////////////////////////////////////////

#include <math.h>

#define CAM_ANGLE_THRESHOLD 0.4f

#include "array_byte.h"

void behavior_camera_test(Game game, entity_t* e, float dt) {
  UNUSED(e);

  demo_t* demo = game->demo;
  input_t* input = &game->input;

  // Camera rotation
  if (input_pressed(IN_CAM_ROTATE)) {
    float xrot = d2r(game->input.mouse.move.y * 180);
    float yrot = d2r(game->input.mouse.move.x * 180);
    vec2 angles = v2f(xrot, -yrot);
    camera_orbit(&game->camera, demo->target, angles);
    //str_log("Mouse pos: {:.03}, move: {:.03}", game->input.mouse.pos, game->input.mouse.move);
  }
  else if (input->touch.count == 1) {
    float xrot = d2r(input->touch.first->move.y * 180.0f);
    float yrot = d2r(input->touch.first->move.x * 180.0f);
    vec2 angles = v2f(xrot, -yrot);
    camera_orbit(&game->camera, demo->target, angles);
    //str_log("Touch id: {}, pos: {:.03}, move: {:.03}", input->touch.first->id, input->touch.first->pos, input->touch.first->move);
  }

  if (input->touch.count) {
    Array_byte str = arr_byte_copy_str("[Touch IDs] ");
    for (index_t i = 0; i < input->touch.count; ++i) {
      arr_byte_append_format(str, " {}", input->touch.fingers.begin[i].id);
    }
    str_write(slice_from_arr(str));
    arr_byte_delete(&str);
  }

  // Camera dragging/panning
  if (input_pressed(IN_CAM_DRAG) || input->touch.count == 2) {
    vec3 ray = camera_ray(&game->camera, input->mouse.pos);
    vec2 m_prev = v2sub(game->input.mouse.pos, input->mouse.move);
    vec3 ray_prev = camera_ray(&game->camera, m_prev);

    // touch...
    if (input->touch.count == 2) {
      vec2 mid = v2mid(input->touch.first->pos, input->touch.second->pos);
      vec2 mid_move = v2mid(input->touch.first->move, input->touch.second->move);
      ray = camera_ray(&game->camera, mid);
      m_prev = v2sub(mid, mid_move);
      ray_prev = camera_ray(&game->camera, m_prev);
    }

    float t, k;
    vec3 plane = v3y;
    if (fabs(v3dot(game->camera.front, v3x)) > 1.0f - CAM_ANGLE_THRESHOLD) {
      plane = v3x;
    }
    else if (fabs(v3dot(game->camera.front, v3y)) < CAM_ANGLE_THRESHOLD) {
      plane = v3z;
    }
    if (v3ray_plane(game->camera.pos, ray, demo->target, plane, &t)
    &&  v3ray_plane(game->camera.pos, ray_prev, demo->target, plane, &k)
    ) {
      vec3 old_pos = v3add(demo->target, v3scale(ray, t));
      vec3 new_pos = v3add(demo->target, v3scale(ray_prev, k));
      vec3 offset = v3sub(new_pos, old_pos);
      if (v3mag(offset) > 1000.0f) {
        offset = v3rescale(offset, 1000.0f);
      }

      demo->target = v3add(demo->target, offset);
      game->camera.pos = v3add(game->camera.pos, offset);
    }
  }

  // Camera zoom
  vec3 view_dir = v3sub(game->demo->target, game->camera.pos);
  float cam_speed = CAMERA_SPEED * dt * v3mag(view_dir);

  float zoom = game->input.mouse.scroll * 30.f;

  if (input_pressed(IN_JUMP)) zoom += 1;
  if (input_pressed(IN_DOWN)) zoom -= 1;

  if (input->touch.count == 2) {
    vec2 prev1 = v2sub(input->touch.first->pos, input->touch.first->move);
    vec2 prev2 = v2sub(input->touch.second->pos, input->touch.second->move);
    vec2 curr1 = input->touch.first->pos;
    vec2 curr2 = input->touch.second->pos;
    prev1.x *= game->camera.perspective.aspect;
    prev2.x *= game->camera.perspective.aspect;
    curr1.x *= game->camera.perspective.aspect;
    curr2.x *= game->camera.perspective.aspect;

    float dist_old = v2dist(prev1, prev2);
    float dist_new = v2dist(curr1, curr2);
    zoom += (dist_new - dist_old) * 500.0f;
  }

  if (zoom != 0) {
    game->camera.pos = v3add(
      game->camera.pos, v3scale(game->camera.front, zoom * cam_speed)
    );
  }

  // Camera keyboard controls
  if (input_pressed(IN_JUMP)) //game->input.pressed.forward)
    game->camera.pos = v3add(
      game->camera.pos, v3scale(game->camera.front, cam_speed)
    );

  if (input_pressed(IN_DOWN)) //game->input.pressed.back)
    game->camera.pos = v3add(
      game->camera.pos, v3scale(game->camera.front, -cam_speed)
    );

  if (input_pressed(IN_RIGHT)) { //game->input.pressed.right) {
    vec3 right = v3norm(v3cross(game->camera.front, game->camera.up));
    right = v3scale(right, cam_speed);
    game->camera.pos = v3add(game->camera.pos, right);
    demo->target = v3add(demo->target, right);
  }

  if (input_pressed(IN_LEFT)) { //game->input.pressed.left) {
    vec3 left = v3norm(v3cross(game->camera.front, game->camera.up));
    left = v3scale(left, -cam_speed);
    game->camera.pos = v3add(game->camera.pos, left);
    demo->target = v3add(demo->target, left);
  }

  // Other random inputs that aren't actually related to camera motion

  if (input_pressed(IN_SNAP_LIGHT)) {
    demo->light_pos = game->camera.pos;
  }

  if (input_pressed(IN_ROTATE_LIGHT)) { //game->input.pressed.rmb) {
    mat4 light_rotation = m4rotation(v3y, 4.f * dt);//yrot);
    demo->light_pos = mv4mul(light_rotation, v34(demo->light_pos)).xyz;
  }

  if (input_triggered(IN_RELOAD)) {
    game->next_scene = 0;
  }
}

////////////////////////////////////////////////////////////////////////////////

void behavior_wizard_level(Game game, entity_t* e, float dt) {
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

#define WIZARD_SPEED 8.0f
#define WIZARD_FAKE_SPEED (WIZARD_SPEED * 4.0f)
#define WIZARD_FIRE_RATE 0.05f
#define WIZARD_BADDY_SPAWN_RATE 3.0f

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

static void behavior_projectile(Game game, entity_t* e, float dt) {
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

static void ondelete_projectile(Game game, entity_t* e) {
  UNUSED(game);
  wizard_bullet_t* bullet = smap_bullet_ref(wizard_bullets, e->user_id);
  if (!bullet) return;

  light_remove(bullet->light_id);
  smap_bullet_remove(wizard_bullets, e->user_id);
}

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
        .behavior = behavior_projectile,
        .ondelete = ondelete_projectile,
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

#include <math.h>

#define BADDY_SPEED 4.0f
static void behavior_baddy(Game game, entity_t* e, float dt) {
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
static float plane_speed = PLANE_SPEED_MIN;
void behavior_camera_monument(Game game, entity_t* e, float dt) {
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
// Toggles the visibility of the grid
////////////////////////////////////////////////////////////////////////////////

#ifndef __WASM__
#include "ui.h"

static void _normalize_floats_fixed(float* floats, int count, int fixed_ind) {
  assert(count >= 0 && fixed_ind >= 0);
  assert(fixed_ind < count);

  float mag_sq = 0;
  for (int i = 0; i < count; ++i) {
    if (i == fixed_ind) continue;
    mag_sq += floats[i] * floats[i];
  }

  float fixed = floats[fixed_ind];
  float rem_sq = 1.0f - fixed * fixed;
  if (rem_sq < 0.0f) rem_sq = 0.0f;

  if (mag_sq <= 0.00001f) {
    float rem = sqrtf(rem_sq);
    for (int i = 0; i < count; ++i) {
      if (i == fixed_ind) continue;
      floats[i] = rem / 3.0f;
    }
    return;
  }

  float scale = sqrtf(rem_sq / mag_sq);
  for (int i = 0; i < count; ++i) {
    if (i == fixed_ind) continue;
    floats[i] *= scale;
  }
}

#endif // ifndef __WASM__

#include <float.h>

typedef enum editor_mode_t {
  EM_SELECT,
  EM_TRANSLATE,
  EM_ROTATE,
  EM_CREATE
} editor_mode_t;

void behavior_grid_toggle(Game game, entity_t* e, float dt) {
  UNUSED(dt);
  if (input_triggered(IN_TOGGLE_GRID)) {
    entity_set_hidden(e, !e->is_hidden);
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

  if (input_triggered(IN_LEVEL_3)) {
    game->next_scene = 2;
  }

  if (input_triggered(IN_TOGGLE_LOCK) && game->scene == 2) {
    input_pointer_unlock();
  }

#ifndef __WASM__
  static bool show_ui = false;

  if (input_triggered(IN_TOGGLE_UI)) {
    show_ui ^= 1;
  }

  if (!show_ui) return;

  ImVec2_c v2imzero = { 0 };
  ImVec2_c v2imwinsize = { 250, (float)game->window.h };
  ImVec2_c v2imsubmenu = { 234, 0 };
  ImVec2_c v2imbtnsize = { 20, 20 };
  ImVec4_c v4imbtnselcolor = { 0, 0.4f, 0.8f, 1 };

  ImGuiWindowFlags flags_information
    = ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoScrollbar
    //| ImGuiWindowFlags_AlwaysAutoResize
    ;

  ImGuiWindowFlags flags_inspector
    = ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoScrollbar
    ;

  ImGuiChildFlags child_flags
    = ImGuiChildFlags_AlwaysAutoResize
    | ImGuiChildFlags_Borders
    | ImGuiChildFlags_AutoResizeY
    | ImGuiChildFlags_AutoResizeX
    ;

  ImGuiTooltipFlags tooltip_flags
    = ImGuiHoveredFlags_DelayNormal
    ;

  static slotkey_t selected = { 0 };

  static editor_mode_t edit_mode = EM_SELECT;

  igSetNextWindowPos(v2imzero, ImGuiCond_Appearing, v2imzero);
  igSetNextWindowSize(v2imwinsize, ImGuiCond_Always);

  if (igBegin("Information", NULL, flags_information)) {
    //float screen_y = igGetCursorScreenPos().y;

    igText("Scene:");
    const char* scenes[] = { "1 - Editor", "2 - Magicks", "3 - Flight" };
    igPushItemWidth(-1);
    if (igBeginCombo("##scene_select", scenes[game->scene], ImGuiComboFlags_NoArrowButton)) {
      for (int i = 0; i < (int)ARRAY_COUNT(scenes); ++i) {
        if (igSelectable_Bool(scenes[i], i == game->scene, 0, v2imzero)) {
          game->next_scene = i;
        }
      }
      igEndCombo();
    }
    igPopItemWidth();

    igText("Tools:");
    igBeginChild_Str("panel_tools", v2imsubmenu, child_flags, 0);
    editor_mode_t new_mode = edit_mode;

    if (edit_mode == EM_SELECT) igPushStyleColor_Vec4(ImGuiCol_Button, v4imbtnselcolor);
    if (igButton("S", v2imbtnsize)) {
      new_mode = EM_SELECT;
    }
    if (igIsItemHovered(tooltip_flags)) igSetTooltip("Select mode");
    if (edit_mode == EM_SELECT) igPopStyleColor(1);

    igSameLine(0, 5);

    if (edit_mode == EM_TRANSLATE) igPushStyleColor_Vec4(ImGuiCol_Button, v4imbtnselcolor);
    if (igButton("T", v2imbtnsize)) {
      new_mode = EM_TRANSLATE;
    }
    if (igIsItemHovered(tooltip_flags)) igSetTooltip("Translate mode");
    if (edit_mode == EM_TRANSLATE) igPopStyleColor(1);

    igSameLine(0, 5);

    if (edit_mode == EM_ROTATE) igPushStyleColor_Vec4(ImGuiCol_Button, v4imbtnselcolor);
    if (igButton("R", v2imbtnsize)) {
      new_mode = EM_ROTATE;
    }
    if (igIsItemHovered(tooltip_flags)) igSetTooltip("Rotate mode");
    if (edit_mode == EM_ROTATE) igPopStyleColor(1);

    igSameLine(0, 5);

    if (edit_mode == EM_CREATE) igPushStyleColor_Vec4(ImGuiCol_Button, v4imbtnselcolor);
    if (igButton("C", v2imbtnsize)) {
      new_mode = EM_CREATE;
    }
    if (igIsItemHovered(tooltip_flags)) igSetTooltip("Create mode");
    if (edit_mode == EM_CREATE) igPopStyleColor(1);

    igEndChild();

    edit_mode = new_mode;

    if (igCollapsingHeader_BoolPtr("Stats", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
      igBeginChild_Str("panel_stats", v2imsubmenu, child_flags, 0);
      igText("FPS: %.3f", 1.0f / game->frame_time);
      igText("Entities: %d", entity_count());
      igText("Lights: %d", light_count());
      igEndChild();
    }

    if (igCollapsingHeader_BoolPtr("Shortcuts", NULL, 0)) {
      igBeginChild_Str("panel_help", v2imsubmenu, child_flags, 0);
      igText("[O] Move camera");
      igText("[I] Rotate camera");
      igText("[C] Snap to selection");
      igText("[R] Set light pos");
      igEndChild();
    }

    if (igCollapsingHeader_BoolPtr("Entities", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
      float y = igGetCursorScreenPos().y;
      float h = (float)game->window.h - 5;
      igBeginChild_Str("panel_entities", (ImVec2_c) { 234, h - y }, child_flags, 0);
      entity_t* entity = entity_ref(selected);
      //const char* selected_str = entity ? entity->name->begin : "<None>";
      //if (igBeginCombo("##entity_select", selected_str, 0)) {

      igPushItemWidth(-1);
      if (igBeginListBox("##entity_select", (ImVec2_c) {-FLT_MIN, -FLT_MIN} )) {
        if (igSelectable_Bool("<None>", selected.hash == 0, 0, v2imzero)) {
          selected = SK_NULL;
        }
        for (slotkey_t id = SK_NULL; entity = entity_next(&id), entity;) {
          bool is_selected = entity->id.hash == selected.hash;
          String label = str_format("{}##{}", entity->name, sk_unique(id));
          if (igSelectable_Bool(label->begin, is_selected, 0, v2imzero)) {
            selected = entity->id;
          }
          str_delete(&label);
        }
        //igEndCombo();
        igEndListBox();
      }

      igPopItemWidth();
      igEndChild();
    }
  }
  igEnd();

  bool information_open = igBegin("Information", NULL, flags_inspector);
  if (information_open) {
    if (game->scene == 2) {
      igText("Scene Info");
      igBeginChild_Str("panel_monument", v2imsubmenu, child_flags, 0);
      igText("Extent");
      igSliderInt(
        "##mon_extent", &game->demo->monument_extent, 0, 100, NULL, 0);

      igText("Spacing");
      igSliderInt(
        "##mon_spacing", &game->demo->monument_size, 0, 500, NULL, 0);

      if (igButton("Apply", v2imzero)) {
        game->next_scene = 2;
      }
      igSameLine(0, 5);
      if (igButton("Reset", v2imzero)) {
        game->demo->monument_extent = 10;
        game->demo->monument_size = 200;
        game->next_scene = 2;
      }
      igEndChild();
    }
  }
  igEnd();

  if (!information_open) return;

  if (selected.hash == 0) {
    return;
  }

  entity_t* entity = entity_ref(selected);

  if (!entity) return;

  bool do_center = false;
  bool do_delete = false;

  if (input_triggered(IN_CAM_CENTER)) {
    do_center = true;
  }

  if (input_triggered(IN_DELETE_OBJECT)) {
    do_delete = true;
  }

  const touch_t* touch = input_touch_get(&game->input, 0);
  if (touch) {
    vec2 pos_adj = v2f((float)game->window.x, (float)game->window.y);
    pos_adj = v2mul(pos_adj, touch->pos);
    vec3 ray = camera_ray(&game->camera, pos_adj);
    float t;
    if (v3ray_plane(game->camera.pos, ray, v3origin, v3up, &t)) {
      entity_set_position(entity, v3add(game->camera.pos, v3scale(ray, t)));
    }
  }

  ImVec2_c top_right = (ImVec2_c){ (float)game->window.w, 0 };
  igSetNextWindowPos(top_right, ImGuiCond_Always, (ImVec2_c) { 1, 0 });
  igSetNextWindowSize(v2imwinsize, ImGuiCond_Always);

  if (igBegin("Entity", NULL, flags_inspector)) {

    if (entity) {
      igText("ID: %d - %llu", sk_index(entity->id), sk_unique(entity->id));

      {
        char name_buffer[1000];
        memcpy(name_buffer, entity->name->begin, MIN(1000, entity->name->size+1));
        igPushItemWidth(-1);
        if (igInputText("##Name", name_buffer, 1000, 0, NULL, NULL)) {
          str_delete(&entity->name);
          entity->name = str_copy(name_buffer);
        }
        igPopItemWidth();
      }

      if (igButton("Center", v2imzero)) {
        do_center = true;
      }

      igSameLine(0, 5);

      if (igButton("Delete", v2imzero)) {
        do_delete = true;
      }

      bool fake_hidden = entity->is_hidden;
      if (igCheckbox("Hidden", &fake_hidden)) {
        entity_set_hidden(entity, fake_hidden);
      }

      bool fake_static = entity->is_static;
      if (igCheckbox("Static", &fake_static)) {
        entity_set_static(entity, fake_static);
      }

      if (igCollapsingHeader_BoolPtr("Transform", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
        igPushItemWidth(-1);
        igBeginChild_Str("panel_transform", v2imsubmenu, child_flags, 0);

        igText("Position:");
        vec3 fake_pos = entity->pos;
        if (igDragFloat3("##ety_position",
          fake_pos.f, 0.1f, -99999.f, 99999.f, "%.2f", 0)
          ) {
          entity_set_position(entity, fake_pos);
        }

        igText("Rotation (quaternion):");
        quat rotation = entity->rot;
        if (igSliderFloat4("##ety_rotation", rotation.f, -1.f, 1.f, "%.3f", 0)) {
          int changed = 0;
          for (int i = 1; i < q4floats; ++i)
            if (rotation.f[i] != entity->rot.f[i]) changed = i;
          _normalize_floats_fixed(rotation.f, 4, changed);
          entity_set_rotation(entity, rotation);
        }

        igText("Rotation (axis-angle):");
        vec3 axis = v3norm(q4axis(entity->rot));
        vec3 test = axis;
        float angle = q4angle(entity->rot);
        if (igSliderFloat3("##ety_rot_axis", axis.f, -1.f, 1.f, "%.2f", 0)) {
          int changed = 0;
          for (int i = 1; i < v3floats; ++i)
            if (axis.f[i] != test.f[i]) changed = i;
          _normalize_floats_fixed(axis.f, 3, changed);
          entity_set_rotation_a(entity, axis, angle);
        }

        if (igDragFloat("##ety_rot_angle", &angle, 0.01f, 0, TAU, "%.2f", ImGuiSliderFlags_WrapAround)) {
          entity_set_rotation_a(entity, axis, angle);
        }


        igText("Scale (uniform):");
        float scale = entity->scale;
        if (igDragFloat("##ety_scale", &scale, 0.025f, 0.01f, 9999.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
          entity_set_scale(entity, scale);
        }

        igEndChild();
        igPopItemWidth();
      }


      if (igCollapsingHeader_BoolPtr("Rendering", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
        igPushItemWidth(-1);
        igBeginChild_Str("panel_rendering", v2imsubmenu, child_flags, 0);

        if (entity->renderer) {
          igText("Renderer: %s", entity->renderer->name);
          igText("Shader: %s", entity->renderer->shader->name.begin);
          igText("Render id: %d - %llu", sk_index(entity->render_id), sk_unique(entity->render_id));
        }

        if (entity->onrender) {
          igText("Renderer: onrender");
        }

        if (entity->model) {
          const char* model_types[MODEL_TYPES_COUNT] = {
            "None",
            "Grid",
            "Cube",
            "Cube_Color",
            "Frame",
            "Sprites",
            "Mesh"
          };
          model_type_t type = entity->model->type;
          if (type >= 0 && type < MODEL_TYPES_COUNT) {
            igText("Model type: %s", model_types[type]);
          }
        }

        if (entity->material) {
          igText("Material:");
          Array_slice material_names = mat_get_names();

          if (!arr_slice_is_null_or_empty(material_names)) {
            if (igBeginCombo("##material_select", entity->material->name.begin, 0)) {
              slice_t* arr_foreach(name, material_names) {
                bool is_selected = slice_eq(*name, entity->material->name);
                if (igSelectable_Bool(name->begin, is_selected, 0, v2imzero)) {
                  Material new_material = mat_get(*name);
                  entity_set_material(entity, new_material);
                }
              }
              igEndCombo();
            }
          }

          arr_slice_delete(&material_names);
        }

        if (entity->renderer) {
          attribute_format_t attrib_format = entity->renderer->shader->attrib_format;

          if (attribute_has_material_index(attrib_format)
            && entity->material && entity->material->layers > 0
            ) {
            int index = (int)entity_get_material_index(entity);
            int layers = (int)entity->material->layers;

            igText("Material Index:");
            if (igSliderInt("##ety_mat_index", &index, 0, layers - 1, NULL, ImGuiSliderFlags_AlwaysClamp)) {
              entity_set_material_index(entity, index);
            }
          }

          if (attribute_has_tint(attrib_format)) {
            color4 color = v4vc(entity_get_tint(entity));

            igText("Tint Color:");
            if (igSliderFloat4("##ety_tint", color.f, 0.f, 1.f, "%.3f", 0)) {
              entity_set_tint(entity, v4cv(color));
            }
          }
        }

        if (!entity->renderer && !entity->onrender) {
          igText("Non-renderable");
        }

        igEndChild();
        igPopItemWidth();
      }
    }
    else {
      igText("No entity selected");
    }

  }

  if (do_center) {
    vec3 target_ray = v3sub(game->demo->target, game->camera.pos);
    game->demo->target = entity->pos;
    game->camera.pos = v3sub(entity->pos, target_ray);
  }

  if (do_delete) {
    entity_remove(entity->id);
  }

  igEnd();
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Cube spinning behavior
////////////////////////////////////////////////////////////////////////////////

void behavior_cubespin(Game game, entity_t* e, float dt) {
  UNUSED(dt);

  entity_set_rotation(e, q4mul(
    q4axang(v3f(1.f, 1.5f, -.7f), game->scene_time),
    q4axang(v3f(-4.f, 1.5f, 1.f), game->scene_time / 3.6f)
  ));
}

////////////////////////////////////////////////////////////////////////////////
// Clockwise spin around the Z axis
////////////////////////////////////////////////////////////////////////////////

void behavior_gear_rotate_cw(Game game, entity_t* e, float dt) {
  UNUSED(game);
  entity_rotate_a(e, v3back, dt);
}

////////////////////////////////////////////////////////////////////////////////
// Counter-clockwise spin around the Z axis
////////////////////////////////////////////////////////////////////////////////

void behavior_gear_rotate_ccw(Game game, entity_t* e, float dt) {
  UNUSED(game);
  entity_rotate_a(e, v3front, dt);
}

////////////////////////////////////////////////////////////////////////////////
// Sun-gear rotation around the Y axis
////////////////////////////////////////////////////////////////////////////////

void behavior_gear_rotate_sun(Game game, entity_t* e, float dt) {
  UNUSED(game);
  entity_rotate_a(e, v3down, dt);
}

////////////////////////////////////////////////////////////////////////////////
// Orients the entity to face the camera along its local +Z axis
////////////////////////////////////////////////////////////////////////////////

void behavior_stare(Game game, entity_t* e, float dt) {
  UNUSED(dt);
  entity_set_rotation(e, q3look(v3sub(game->camera.pos, e->pos), v3up));
}

////////////////////////////////////////////////////////////////////////////////
// Sets the location of the entity to the light position
////////////////////////////////////////////////////////////////////////////////

extern slotkey_t editor_light_bright;
extern slotkey_t editor_light_gizmo;
void behavior_attach_to_light(Game game, entity_t* e, float dt) {
  UNUSED(dt);
  entity_set_position(e, game->demo->light_pos);
  light_t* light = light_ref(editor_light_bright);
  if (!light) return;
  light->pos = game->demo->light_pos;
  light->dir = v3sub(game->demo->target, game->demo->light_pos);
  light = light_ref(editor_light_gizmo);
  light->pos = game->demo->target;
}

////////////////////////////////////////////////////////////////////////////////
// Sets the location of the entity to the camera target position
////////////////////////////////////////////////////////////////////////////////

void behavior_attach_to_camera_target(Game game, entity_t* e, float dt) {
  UNUSED(dt);
  entity_set_position(e, game->demo->target);

  if (input_triggered(IN_CREATE_OBJECT)) {
    Material mats[] = {
      game->demo->materials.crate,
      game->demo->materials.mudds,
      game->demo->materials.grass,
      game->demo->materials.tiles,
      game->demo->materials.sands,
      game->demo->materials.renderite,
    };

    vec3 ray = camera_ray(&game->camera, game->input.mouse.pos);
    float t;
    if (v3ray_plane(game->camera.pos, ray, v3origin, v3up, &t)) {
      entity_add(&(entity_desc_t) {
        .model = game->demo->models.box,
        .material = mats[(uint)e->pos.x % 6],
        .pos = v3add(game->camera.pos, v3scale(ray, t)),
        .scale = 10.0f,
        .onrender = render_pbr,
      });
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Render function for un-shaded objects with vertex color
////////////////////////////////////////////////////////////////////////////////

void render_basic(Game game, entity_t* e) {
  Shader shader = game->demo->shaders.basic;
  shader_bind(shader);

  int loc_pvm = shader_uniform_loc(shader, "in_pvm_matrix");

  mat4 pvm = m4mul(game->camera.projview, entity_transform(e));
  glUniformMatrix4fv(loc_pvm, 1, GL_FALSE, pvm.f);

  model_render(e->model);
}

////////////////////////////////////////////////////////////////////////////////
// Renders the debug objects
////////////////////////////////////////////////////////////////////////////////

void render_debug(Game game, entity_t* e) {
  render_basic(game, e);
  draw_render();
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
  int loc_sampler_tex = shader_uniform_loc(shader, "samp_tex");
  int loc_sampler_norm = shader_uniform_loc(shader, "samp_norm");
  int loc_sampler_rough = shader_uniform_loc(shader, "samp_rough");
  int loc_sampler_metal = shader_uniform_loc(shader, "samp_metal");
  int loc_norm = shader_uniform_loc(shader, "in_normal_matrix");
  int loc_props = shader_uniform_loc(shader, "in_weights");
  int loc_tint = shader_uniform_loc(shader, "in_tint");

  mat4 transform = entity_transform(e);
  mat4 pvm = m4mul(game->camera.projview, transform);
  mat4 norm = m4transpose(m4inverse(m4mul(game->camera.view, transform)));

  //glUniform3fv(loc_tint, 1, e->tint.f);
  glUniform3fv(loc_tint, 1, v3ones.f);

  glUniformMatrix4fv(loc_pvm, 1, 0, pvm.f);
  glUniformMatrix4fv(loc_norm, 1, 0, norm.f);
  glUniform3fv(loc_props, 1, e->material->weights.f);

  //glUniform4fv(loc_light_pos, 1, demo->light_pos.f);
  //glUniform4fv(loc_camera_pos, 1, game->camera.pos.f);

  //GLint use_color = false;
  //if (e->model->type == MODEL_MESH) use_color = e->model->mesh.use_color;
  //glUniform1i(loc_use_vert_color, use_color);

  tex_apply(e->material->map_diffuse, 0, loc_sampler_tex);
  tex_apply(e->material->map_normals, 1, loc_sampler_norm);
  tex_apply(e->material->map_roughness, 2, loc_sampler_rough);
  tex_apply(e->material->map_metalness, 3, loc_sampler_metal);

  model_render(e->model);

  glBindTexture(GL_TEXTURE_2D, 0);
}
