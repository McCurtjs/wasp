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
#include "str.h"
#include "particles.h"

#define CAMERA_SPEED 0.8f

////////////////////////////////////////////////////////////////////////////////
// Behavior for controlling camera rotation and movement
////////////////////////////////////////////////////////////////////////////////

#include <math.h>

#define CAM_ANGLE_THRESHOLD 0.4f

#include "array_byte.h"

void _behavior_camera_test(Game game, entity_t* e, float dt) {
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

////////////////////////////////////////////////////////////////////////////////

#include <float.h>

typedef enum object_mode_t {
  OM_ENTITIES,
  OM_EFFECTS,
  OM_EMITTERS,
  OM_COUNT
} object_mode_t;

typedef enum editor_mode_t {
  EM_SELECT,
  EM_TRANSLATE,
  EM_ROTATE,
  EM_CREATE,
  EM_COUNT
} editor_mode_t;

ImVec2_c v2imzero = { 0 };
ImVec2_c v2imsubmenu = { 234, 0 };
ImVec2_c v2imbtnsize = { 20, 20 };
ImVec4_c v4imbtnselcolor = { 0, 0.4f, 0.8f, 1 };
ImVec2_c v2imfillspace = { -FLT_MIN, -FLT_MIN };

ImVec2_c _get_winsize(Game game) {
  return (ImVec2_c) { 250, (float)game->window.h };
}

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

static bool           _show_ui = false;
static bool           _panel_open_information = true;
static int            _rotation_format = 0;
static object_mode_t  _object_mode = OM_ENTITIES;
static editor_mode_t  _edit_mode = EM_SELECT;
static slotkey_t      _selected_entity = { 0 };
static slotkey_t      _selected_emitter = { 0 };
static ParticleEffect _selected_effect = NULL;
static quat           _selected_rot = { 0 };
static vec3           _selected_axis = { 0 };
static vec3           _selected_euler = { 0 };
static float          _selected_angle = 0;

void _editor_panel_info_scenes(Game game) {
  const char* scenes[] =
  { "1 - Editor"
  , "2 - Magicks"
  , "3 - Flight"
  };

  _panel_open_information = igBegin("Information", NULL, flags_information);

  if (_panel_open_information) {
    igText("Scene:");
    ImGuiComboFlags flags = ImGuiComboFlags_NoArrowButton;
    igPushItemWidth(-1);
    if (igBeginCombo("##scene_select", scenes[game->scene], flags)
    ) {
      for (int i = 0; i < (int)ARRAY_COUNT(scenes); ++i) {
        if (igSelectable_Bool(scenes[i], i == game->scene, 0, v2imzero)) {
          game->next_scene = i;
        }
      }
      igEndCombo();
    }
    igPopItemWidth();
  }
  igEnd();
}

void _editor_panel_info_tools(Game game) {
  UNUSED(game);

  if (igBegin("Information", NULL, flags_information)) {
    igText("Tools:");
    igBeginChild_Str("panel_tools", v2imsubmenu, child_flags, 0);
    editor_mode_t new_mode = _edit_mode;

    if (_edit_mode == EM_SELECT) igPushStyleColor_Vec4(ImGuiCol_Button, v4imbtnselcolor);
    if (igButton("S", v2imbtnsize)) {
      new_mode = EM_SELECT;
    }
    if (igIsItemHovered(tooltip_flags)) igSetTooltip("Select mode");
    if (_edit_mode == EM_SELECT) igPopStyleColor(1);

    igSameLine(0, 5);

    if (_edit_mode == EM_TRANSLATE) igPushStyleColor_Vec4(ImGuiCol_Button, v4imbtnselcolor);
    if (igButton("T", v2imbtnsize)) {
      new_mode = EM_TRANSLATE;
    }
    if (igIsItemHovered(tooltip_flags)) igSetTooltip("Translate mode");
    if (_edit_mode == EM_TRANSLATE) igPopStyleColor(1);

    igSameLine(0, 5);

    if (_edit_mode == EM_ROTATE) igPushStyleColor_Vec4(ImGuiCol_Button, v4imbtnselcolor);
    if (igButton("R", v2imbtnsize)) {
      new_mode = EM_ROTATE;
    }
    if (igIsItemHovered(tooltip_flags)) igSetTooltip("Rotate mode");
    if (_edit_mode == EM_ROTATE) igPopStyleColor(1);

    igSameLine(0, 5);

    if (_edit_mode == EM_CREATE) igPushStyleColor_Vec4(ImGuiCol_Button, v4imbtnselcolor);
    if (igButton("C", v2imbtnsize)) {
      new_mode = EM_CREATE;
    }
    if (igIsItemHovered(tooltip_flags)) igSetTooltip("Create mode");
    if (_edit_mode == EM_CREATE) igPopStyleColor(1);

    igEndChild();

    _edit_mode = new_mode;
  }

  igEnd();
}

void _editor_panel_info_stats(Game game) {
  if (igBegin("Information", NULL, flags_information)) {
    if (igCollapsingHeader_BoolPtr("Stats", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
      igBeginChild_Str("panel_stats", v2imsubmenu, child_flags, 0);
      igText("FPS: %.3f", 1.0f / game->frame_time);

      bool vsync = gfx_get_vsync();
      if (igCheckbox("V-Sync", &vsync)) {
        gfx_set_vsync(vsync);
      }

      igText("Entities: %d", entity_count());
      igText("Lights: %d", light_count());

      igText("Resolution");
      if (igInputInt2("##resolution", game->resolution.i, ImGuiInputTextFlags_None)) {
        game->on_window_resize(game);
      }

      igEndChild();
    }
  }
  igEnd();
}

void _editor_panel_info_shortcuts(Game game) {
  UNUSED(game);

  if (igBegin("Information", NULL, flags_information)) {
    if (igCollapsingHeader_BoolPtr("Shortcuts", NULL, 0)) {
      igBeginChild_Str("panel_help", v2imsubmenu, child_flags, 0);
      igText("[O] Move camera");
      igText("[I] Rotate camera");
      igText("[C] Snap to selection");
      igText("[R] Set light pos");
      igEndChild();
    }
  }
  igEnd();
}

Entity _editor_set_entity(Game game, slotkey_t entity_id) {
  UNUSED(game);

  Entity ret = entity_ref(entity_id);

  if (ret) {
    _selected_entity = entity_id;
    _selected_euler = v3euler(ret->rot);
    _selected_axis = q4axis(ret->rot);
    _selected_angle = q4angle(ret->rot);
    _selected_rot = ret->rot;
  }
  else {
    _selected_entity = SK_NULL;
  }

  _selected_emitter = SK_NULL;
  _selected_effect = NULL;

  return ret;
}

ParticleEffect _editor_set_effect(Game game, slice_t effect_name) {
  ParticleEffect ret = ps_get_effect(game->particle_system, effect_name);

  _selected_effect = ret;
  _selected_entity = SK_NULL;

  ParticleEmitter emitter = NULL;
  if (_selected_emitter.hash) {
    emitter = ps_get_emitter(game->particle_system, _selected_emitter);
  }

  if (!emitter || emitter->effect != ret) {
    _selected_emitter = SK_NULL;
  }

  return ret;
}

ParticleEmitter _editor_set_emitter(Game game, slotkey_t emitter_id) {
  ParticleEmitter ret = ps_get_emitter(game->particle_system, emitter_id);

  if (ret) {
    _selected_emitter = emitter_id;
    _selected_euler = v3euler(ret->dir);
    _selected_axis = q4axis(ret->dir);
    _selected_angle = q4angle(ret->dir);
    _selected_rot = ret->dir;
  }
  else {
    _selected_emitter = SK_NULL;
  }

  return ret;
}

void _editor_set_object_mode(Game game, object_mode_t mode) {
  switch (mode) {

  case OM_ENTITIES:
    _editor_set_entity(game, _selected_entity);
    break;

  case OM_EFFECTS:
    _editor_set_entity(game, SK_NULL);
    break;

  case OM_EMITTERS:
    _editor_set_entity(game, SK_NULL);
    break;

  default:
    assert(false);
    break;
  }

  _object_mode = mode;
}

void _editor_panel_info_object_mode_select(Game game) {
  const char* object_modes[OM_COUNT] =
  { "Entities"
  , "Effects"
  , "Emitters"
  };

  _panel_open_information = igBegin("Information", NULL, flags_information);

  if (_panel_open_information) {
    ImGuiComboFlags flags = ImGuiComboFlags_NoArrowButton;
    igPushItemWidth(-1);

    static bool is_open = false;
    igSetNextItemOpen(true, ImGuiCond_Always);
    if (igCollapsingHeader_BoolPtr(object_modes[_object_mode], NULL, 0)) {

      if (is_open) {
        if (igBeginListBox("##object_mode_select", v2imzero)) {

          for (object_mode_t mode = 0; mode < OM_COUNT; ++mode) {
            if (igSelectable_Bool(object_modes[mode]
            , mode == _object_mode, 0, v2imzero)
            ) {
              _editor_set_object_mode(game, mode);
              is_open = false;
            }
          }

          igEndListBox();
        }
      }
    }
    else {
      is_open = !is_open;
    }

    igPopItemWidth();
  }
  igEnd();
}

ParticleEmitter _editor_selectable_emitters(
  Game game, const char* label_format, slotkey_t selected, slotkey_t parent
) {
  ParticleEmitter ret = NULL;
  slotkey_t iter = SK_NULL;

  loop {
    ParticleEmitter emitter = ps_get_next_emitter(game->particle_system, &iter);
    until(emitter == NULL);

    if (parent.hash && parent.hash != emitter->entity_id.hash) continue;

    String label =
      str_format(label_format, emitter->effect->name, sk_unique(emitter->id));

    if (igSelectable_Bool(
      label->begin, emitter->id.hash == selected.hash, 0, v2imzero
    )) {
      ret = emitter;
    }

    str_delete(&label);
  }

  return ret;
}

void _editor_panel_info_entities(Game game) {
  entity_t* entity = entity_ref(_selected_entity);

  igPushItemWidth(-1);
  if (igBeginListBox("##entity_select", v2imfillspace)) {

    if (igSelectable_Bool("<None>", _selected_entity.hash == 0, 0, v2imzero)) {
      _selected_entity = SK_NULL;
    }

    for (slotkey_t id = SK_NULL; entity = entity_next(&id), entity;) {
      bool is_selected = entity->id.hash == _selected_entity.hash;
      String label = str_format("{}##{}", entity->name, sk_unique(id));
      if (igSelectable_Bool(label->begin, is_selected, 0, v2imzero)) {
        _editor_set_entity(game, entity->id);
      }
      str_delete(&label);

      if (!is_selected) continue;

      ParticleEmitter emitter = _editor_selectable_emitters(
        game, "- Emitter: {0} {1}##{1}", _selected_emitter, _selected_entity
      );

      if (emitter) {
        _selected_emitter = emitter->id;
      }
    }

    igEndListBox();
  }

  igPopItemWidth();
}

void _editor_panel_info_effects(Game game) {

  if (igButton("New Effect", v2imzero)) {
    ps_add_effect(game->particle_system, S("New Effect"), PF_POINT, 0);
  }

  igPushItemWidth(-1);
  if (igBeginListBox("##effect_select", v2imfillspace)) {
    if (igSelectable_Bool("<None>", _selected_effect == NULL, 0, v2imzero)) {
      _selected_effect = NULL;
    }

    Array_slice effect_names = ps_get_effect_names(game->particle_system);

    arr_slice_sort(effect_names);

    slice_t* arr_foreach(name, effect_names) {
      bool is_selected = false;
      if (_selected_effect) {
        is_selected = slice_eq(_selected_effect->name, *name);
      }

      slice_t label = *name;
      if (label.length == 0) label = S("##");
      if (igSelectable_Bool(label.begin, is_selected, 0, v2imzero)) {
        _editor_set_effect(game, *name);
      }
    }

    arr_slice_delete(&effect_names);

    igEndListBox();
  }

  igPopItemWidth();
}

void _editor_panel_info_emitters(Game game) {
  igPushItemWidth(-1);
  if (igBeginListBox("##entity_select", v2imfillspace)) {

    if (igSelectable_Bool("<None>", _selected_entity.hash == 0, 0, v2imzero)) {
      _selected_emitter = SK_NULL;
    }

    ParticleEmitter emitter = _editor_selectable_emitters(
      game, "{0} {1}##{1}", _selected_emitter, SK_NULL
    );

    if (emitter) {
      _selected_emitter = emitter->id;
    }

    igEndListBox();
  }
}

void _editor_panel_info_objects(Game game) {
  ImVec2_c v2imwinsize = _get_winsize(game);

  if (game->scene == 2) return;

  igSetNextWindowPos(v2imzero, ImGuiCond_Appearing, v2imzero);
  igSetNextWindowSize(v2imwinsize, ImGuiCond_Always);

  if (igBegin("Information", NULL, flags_information)) {
    float y = igGetCursorScreenPos().y;
    float h = (float)game->window.h - 5;
    igBeginChild_Str("panel_objects", (ImVec2_c) { 234, h - y }, child_flags, 0);

    switch (_object_mode) {
    case OM_ENTITIES:
      _editor_panel_info_entities(game);
      break;

    case OM_EFFECTS:
      _editor_panel_info_effects(game);
      break;

    case OM_EMITTERS:
      _editor_panel_info_emitters(game);
      break;
    }

    igEndChild();

  }

  igEnd();
}

void _editor_panel_info_ext_monument(Game game) {
  if (game->scene != 2) return;

  if (igBegin("Information", NULL, flags_inspector)) {
    if (igCollapsingHeader_BoolPtr("Scene Info", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
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
}

void _entity_center(Game game, Entity entity) {
  vec3 target_ray = v3sub(game->demo->target, game->camera.pos);
  game->demo->target = entity->pos;
  game->camera.pos = v3sub(entity->pos, target_ray);
}

#define TEXT_INPUT_MAX 1024
String _editor_text_input(const char* label, slice_t target) {
  char name_buffer[TEXT_INPUT_MAX + 1];
  memcpy(name_buffer, target.begin, MIN(TEXT_INPUT_MAX, target.size + 1));
  String ret = NULL;

  igPushItemWidth(-1);
  if (igInputText(label, name_buffer, TEXT_INPUT_MAX, 0, NULL, NULL)) {
    ret = str_copy(&name_buffer[0]);
  }
  igPopItemWidth();

  return ret;
}

void _editor_panel_entity_basic(Game game, Entity entity) {
  ImVec2_c top_right = (ImVec2_c){ (float)game->window.w, 0 };
  igSetNextWindowPos(top_right, ImGuiCond_Always, (ImVec2_c) { 1, 0 });
  igSetNextWindowSize(_get_winsize(game), ImGuiCond_Always);

  if (igBegin("Entity", NULL, flags_inspector)) {
    igText("ID: %d - %llu", sk_index(entity->id), sk_unique(entity->id));

    String new_name = _editor_text_input("##Name", entity->name->slice);
    if (new_name) {
      str_delete(&entity->name);
      entity->name = new_name;
    }
  }
  igEnd();
}

void _editor_panel_entity_tools(Game game, Entity entity) {
  if (igBegin("Entity", NULL, flags_inspector)) {

    if (igButton("Center", v2imzero)) {
      _entity_center(game, entity);
    }

    igSameLine(0, 5);

    if (igButton("Delete", v2imzero)) {
      entity_remove(entity->id);
    }

    bool fake_hidden = entity->is_hidden;
    if (igCheckbox("Hidden", &fake_hidden)) {
      entity_set_hidden(entity, fake_hidden);
    }

    bool fake_static = entity->is_static;
    if (igCheckbox("Static", &fake_static)) {
      entity_set_static(entity, fake_static);
    }
  }
  igEnd();
}

void _editor_panel_entity_transform(Game game, Entity entity) {
  UNUSED(game);

  if (igBegin("Entity", NULL, flags_inspector)) {

    if (igCollapsingHeader_BoolPtr("Transform"
    , NULL, ImGuiTreeNodeFlags_DefaultOpen)
    ) {
      igPushItemWidth(-1);
      igBeginChild_Str("panel_transform", v2imsubmenu, child_flags, 0);

      const char* rotation_formats[] =
      { "Euler"
      , "Axis-Angle"
      , "Quaternion"
      };

      igText("Rotation format:");
      igPushItemWidth(-1);
      if (igBeginCombo("##scene_select"
      , rotation_formats[_rotation_format], ImGuiComboFlags_NoArrowButton)
      ) {
        for (int i = 0; i < (int)ARRAY_COUNT(rotation_formats); ++i) {
          if (igSelectable_Bool
          ( rotation_formats[i]
          , i == _rotation_format
          , 0
          , v2imzero
          )) {
            _rotation_format = i;
            _selected_euler = v3euler(entity->rot);
            _selected_axis = q4axis(entity->rot);
            _selected_angle = q4angle(entity->rot);
            _selected_rot = entity->rot;
          }
        }
        igEndCombo();
      }
      igPopItemWidth();

      igText("Position:");
      vec3 fake_pos = entity->pos;
      if (igDragFloat3("##ety_position"
      , fake_pos.f, 0.1f, -99999.f, 99999.f, "%.2f", 0)
      ) {
        entity_set_position(entity, fake_pos);
      }

      igText("Rotation:");
      if (_rotation_format == 0) {
        vec3 euler = _selected_euler;
        if (igDragFloat3("##ety_rot_euler"
        , euler.f, 0.01f, -TAU, TAU, "%.2f", ImGuiSliderFlags_WrapAround)
        ) {
          entity_set_rotation(entity, q4euler(euler));
          _selected_euler = euler;
        }
      }
      else if (_rotation_format == 1) {
        vec3 axis = _selected_axis;
        float angle = _selected_angle;
        if (igDragFloat3("##ety_rot_axis"
        , axis.f, 0.005f, -1.f, 1.f, "%.2f", ImGuiSliderFlags_WrapAround)
        ) {
          entity_set_rotation_a(entity, v3norm(axis), angle);
          _selected_axis = axis;
        }

        if (igDragFloat("##ety_rot_angle"
        , &angle, 0.005f, 0, TAU, "%.2f", ImGuiSliderFlags_WrapAround)
        ) {
          entity_set_rotation_a(entity, v3norm(axis), angle);
          _selected_angle = angle;
        }
      }
      else if (_rotation_format == 2) {
        quat rotation = entity->rot;
        if (igSliderFloat4("##ety_rotation"
        , rotation.f, -1.f, 1.f, "%.3f", 0)
        ) {
          int changed = 0;
          for (int i = 1; i < q4floats; ++i)
             if (rotation.f[i] != entity->rot.f[i]) changed = i;
          _normalize_floats_fixed(rotation.f, 4, changed);
          entity_set_rotation(entity, rotation);
        }
      }

      igText("Scale (uniform):");
      float scale = entity->scale;
      if (igDragFloat("##ety_scale"
      , &scale, 0.1f, 0.01f, 9999.0f, "%.2f", ImGuiSliderFlags_Logarithmic)) {
        entity_set_scale(entity, scale);
      }

      igEndChild();
      igPopItemWidth();
    }
  }
  igEnd();
}


void render_basic(Game game, Entity e);
void render_debug(Game game, Entity e);
void render_pbr(Game game, Entity e);

void _editor_panel_entity_rendering(Game game, Entity e) {
  UNUSED(game);

  if (igBegin("Entity", NULL, flags_inspector)) {

    if (igCollapsingHeader_BoolPtr("Renderer", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
      igPushItemWidth(-1);
      igBeginChild_Str("panel_renderer", v2imsubmenu, child_flags, 0);

      if (e->renderer) {
        igText("Renderer: %s", e->renderer->name);
        igText("Shader: %s", e->renderer->shader->name.begin);
        igText("Render id: %d - %llu", sk_index(e->render_id), sk_unique(e->render_id));
      }

      if (e->onrender == render_basic)      igText("On-Render: basic");
      else if (e->onrender == render_debug) igText("On-Render: debug");
      else if (e->onrender == render_pbr)   igText("On-Render: PBR (single)");
      else if (e->onrender != NULL)         igText("On-Render: unknown");

      igEndChild();
      igPopItemWidth();
    }

  }

  igEnd();
}

void _editor_panel_entity_model(Game game, Entity entity) {
  UNUSED(game);

  if (igBegin("Entity", NULL, flags_inspector)) {

    if (igCollapsingHeader_BoolPtr("Model", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
      igPushItemWidth(-1);
      igBeginChild_Str("panel_model", v2imsubmenu, child_flags, 0);

      if (entity->model) {

        Array_slice model_names = model_get_names_inst();

        if (!arr_slice_is_null_or_empty(model_names)) {
          if (igBeginCombo("##model_select", entity->model->name.begin, 0)) {
            slice_t* arr_foreach(name, model_names) {
              bool is_selected = slice_eq(*name, entity->material->name);
              if (igSelectable_Bool(name->begin, is_selected, 0, v2imzero)) {
                Model new_model = model_get(*name);
                entity_set_model(entity, new_model);
              }
            }
            igEndCombo();
          }
        }

        arr_slice_delete(&model_names);
      }

      igEndChild();
      igPopItemWidth();
    }

  }

  igEnd();
}

void _editor_panel_entity_material(Game game, Entity entity) {
  UNUSED(game);

  if (igBegin("Entity", NULL, flags_inspector)) {

    if (igCollapsingHeader_BoolPtr("Material", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
      igBeginChild_Str("panel_material", v2imsubmenu, child_flags, 0);

      if (entity->material) {
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
      else {
        igText("None");
      }

      igEndChild();
    }
  }
  igEnd();
}

void _editor_panel_entity_attributes(Game game, Entity entity) {
  UNUSED(game);

  if (!entity->renderer) return;

  if (igBegin("Entity", NULL, flags_inspector)) {
    if (igCollapsingHeader_BoolPtr("Instance", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
      igBeginChild_Str("panel_instance", v2imsubmenu, child_flags, 0);

      attribute_format_t attrib_format = entity->renderer->shader->attrib_format;

      if (attribute_has_material_index(attrib_format)
        && entity->material && entity->material->layers > 0
      ) {
        int index = (int)entity->material_index;
        int layers = (int)entity->material->layers;

        igText("Material Index:");
        if (igSliderInt("##ety_mat_index", &index, 0, layers - 1, NULL, ImGuiSliderFlags_AlwaysClamp)) {
          entity_set_material_index(entity, index);
        }
      }

      if (attribute_has_tint(attrib_format)) {
        color4 color = v4vc(entity->tint);

        igText("Tint Color:");
        if (igSliderFloat3("##ety_tint", color.f, 0.f, 1.f, "%.3f", 0)) {
          entity_set_tint(entity, v4cv(color));
        }
      }

      igEndChild();
    }
  }

  igEnd();
}

void _editor_panel_effect(Game game, ParticleEffect effect) {
  UNUSED(game);

  if (!effect) return;

  ImVec2_c top_right = (ImVec2_c){ (float)game->window.w, 0 };
  igSetNextWindowPos(top_right, ImGuiCond_Always, (ImVec2_c) { 1, 0 });
  igSetNextWindowSize(_get_winsize(game), ImGuiCond_Always);

  if (igBegin("Effect", NULL, flags_inspector)) {

    String new_name = _editor_text_input("##effect_name", effect->name);
    if (new_name) {
      ps_set_effect_name(effect, new_name->slice);
      str_delete(&new_name);
    }

  }

  igEnd();
}

void _editor_panel_emitter_basic(Game game, ParticleEmitter emitter) {
  ImVec2_c top_right = (ImVec2_c){ (float)game->window.w, 0 };
  igSetNextWindowPos(top_right, ImGuiCond_Always, (ImVec2_c) { 1, 0 });
  igSetNextWindowSize(_get_winsize(game), ImGuiCond_Always);

  if (igBegin("Emitter", NULL, flags_inspector)) {
    igText("ID: %d - %llu", sk_index(emitter->id), sk_unique(emitter->id));

    igText("Effect:");

    Array_slice effect_names = ps_get_effect_names(game->particle_system);
    if (igBeginCombo("##emitter_select", emitter->effect->name.begin, 0)) {
      slice_t* arr_foreach(name, effect_names) {
        bool is_selected = slice_eq(*name, emitter->effect->name);
        if (igSelectable_Bool(name->begin, is_selected, 0, v2imzero)) {
        }
      }
      igEndCombo();
    }
    arr_slice_delete(&effect_names);

    if (igCollapsingHeader_BoolPtr("Parameters", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
      igBeginChild_Str("panel_emitter_params", v2imsubmenu, child_flags, 0);

      const char* emitter_shape_names[ES_SHAPE_COUNT] = {
        "Point", "Sphere", "Box (aligned)", "Box", "Disc"
      };

      // Emitter shape

      igText("Shape:");
      if (igBeginCombo("##shape_select", emitter_shape_names[emitter->shape], 0)) {
        for (int i = 0; i < ES_SHAPE_COUNT; ++i) {
          if (igSelectable_Bool
          ( emitter_shape_names[i]
          , i == emitter->shape
          , 0
          , v2imzero
          )) {
            emitter->shape = i;
          }
        }
        igEndCombo();
      }

      igEndChild();

      // Emitter offset

      igText("Offset:");
      igBeginChild_Str("emitter_offset", v2imsubmenu, child_flags, 0);

      igText("Angular Variance:");
      igSliderFloat("##emitter_offset", &emitter->offset, 0.0f, PI, "%.3f", 0);

      igEndChild();

      // Emitter size

      igText("Size:");
      igBeginChild_Str("emitter_size", v2imsubmenu, child_flags, 0);

      if (emitter->shape == ES_POINT) {
        igText("Point size: 0\n  Change shape to set emitter size.");
      }
      else if (emitter->shape == ES_DISC || emitter->shape == ES_SPHERE) {
        igText("Radius:");
        igSliderFloat("##emitter_radius_f", &emitter->radius
        , 0.1f, 20.f, "%.3f", 0);

        igText("Inner:");
        igSliderFloat("##emitter_radius_inner", &emitter->inner_radius
        , 0.f, 1.f, "%.3f", 0);
      }
      else if (emitter->shape == ES_BOX || emitter->shape == ES_BOX_ALIGNED) {
        igText("Size:");
        igSliderFloat3("##emitter_radius", emitter->size.f, 0.f, 20.f, "%.3f", 0);
      }

      igEndChild();

      // Emitter rate

      igText("Rate:");
      igBeginChild_Str("emitter_rate", v2imsubmenu, child_flags, 0);

      igText("Starting:");
      igSliderFloat("##emitter_rate", &emitter->rate, 0.01f, 1000.f, "%.3f", 0);

      igText("Variance:");
      igSliderFloat("##variance_rate", &emitter->rate_variance, 0.f, 1000.f, "%.3f", 0);

      igEndChild();
    }

    if (igCollapsingHeader_BoolPtr("Particles", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {

      // Particle size

      igText("Size:");
      igBeginChild_Str("particle_size", v2imsubmenu, child_flags, 0);

      igText("Starting:");
      if (emitter->effect->format == PF_DEFAULT) {
        igSliderFloat3("##particle_size", &emitter->particle_defaults.size, 0.1f, 20.f, "%.3f", 0);
      }
      else if (emitter->effect->format == PF_POINT) {
        igSliderFloat("##particle_size_f", &emitter->particle_defaults.size, 0.1f, 20.f, "%.3f", 0);
      }

      igText("Variance:");
      if (emitter->effect->format == PF_DEFAULT) {
        igSliderFloat3("##variance_size", &emitter->particle_variance.size, 0.0f, 20.f, "%.3f", 0);
      }
      else if (emitter->effect->format == PF_POINT) {
        igSliderFloat("##variance_size_f", &emitter->particle_variance.size, 0.0f, 20.f, "%.3f", 0);
      }

      igEndChild();

      // Particle speed

      igText("Speed:");
      igBeginChild_Str("particle_speed", v2imsubmenu, child_flags, 0);

      igText("Starting:");
      igSliderFloat("##particle_speed", &emitter->particle_defaults.speed, 0.f, 100.f, "%.3f", 0);

      igText("Variance:");
      igSliderFloat("##variance_speed", &emitter->particle_variance.speed, 0.f, 100.f, "%.3f", 0);

      igEndChild();

      // Particle dDuration

      igText("Duration:");
      igBeginChild_Str("particle_duration", v2imsubmenu, child_flags, 0);

      igText("Starting:");
      igSliderFloat("##particle_duration", &emitter->particle_defaults.duration, 0.01f, 20.f, "%.3f", 0);

      igText("Variance:");
      igSliderFloat("##variance_duration", &emitter->particle_variance.duration, 0.01f, 20.f, "%.3f", 0);

      igEndChild();
    }
  }
  igEnd();
}

void behavior_editor(Game game, entity_t* e, float dt) {
  UNUSED(dt);
  UNUSED(e);

  // Pressing F2 will toggle UI
  if (input_triggered(IN_TOGGLE_UI)) _show_ui ^= 1;

  if (!_show_ui) return;

  _editor_panel_info_scenes(game);
  _editor_panel_info_tools(game);
  _editor_panel_info_stats(game);
  _editor_panel_info_shortcuts(game);
  _editor_panel_info_object_mode_select(game);
  _editor_panel_info_objects(game);
  _editor_panel_info_ext_monument(game);

  entity_t* entity = entity_ref(_selected_entity);

  if (entity) {
    if (input_triggered(IN_CAM_CENTER)) {
      _entity_center(game, entity);
    }

    if (input_triggered(IN_DELETE_OBJECT)) {
      entity_remove(entity->id);
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
  }

  if (_selected_emitter.hash != 0) {
    ParticleEmitter emitter =
      ps_get_emitter(game->particle_system, _selected_emitter);

    _editor_panel_emitter_basic(game, emitter);
  }
  else if (_object_mode == OM_ENTITIES) {
    if (!entity) return;
    _editor_panel_entity_basic(game, entity);
    _editor_panel_entity_tools(game, entity);
    _editor_panel_entity_transform(game, entity);
    _editor_panel_entity_rendering(game, entity);
    _editor_panel_entity_model(game, entity);
    _editor_panel_entity_material(game, entity);
    _editor_panel_entity_attributes(game, entity);
  }
  else if (_object_mode == OM_EFFECTS) {
    _editor_panel_effect(game, _selected_effect);
  }
}

#endif

////////////////////////////////////////////////////////////////////////////////
// General grid and level switching behavior
////////////////////////////////////////////////////////////////////////////////

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

  if (game->input.touch.count == 3 && game->input.touch.third->released) {
    vec2 pos = game->input.touch.third->pos;
    vec2 origin = game->input.touch.third->origin;
    float diff = pos.x - origin.x;
    if (diff > 0.5) game->next_scene = game->scene + 1;
    if (diff < -0.5) game->next_scene = game->scene - 1;

    if (game->scene == 2) {
      diff = pos.y - origin.y;
      if (diff > 0.5)  ++game->demo->monument_extent;
      if (diff < -0.5) --game->demo->monument_extent;
      if (diff > 0.5 || diff < -0.5) game->next_scene = game->scene;
    }
  }

  if (input_triggered(IN_TOGGLE_LOCK) && game->scene == 2) {
    input_pointer_unlock();
  }

  if (input_triggered(IN_TOGGLE_SHADER)
  || (game->input.touch.count == 4 && game->input.touch.first->released)
  ) {
    game->demo->active_shader ^= 1;
  }

#ifndef __WASM__
  behavior_editor(game, e, dt);
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Cube spinning behavior
////////////////////////////////////////////////////////////////////////////////

void _behavior_cubespin(Game game, entity_t* e, float dt) {
  UNUSED(dt);

  entity_set_rotation(e, q4mul(
    q4axang(v3f(1.f, 1.5f, -.7f), game->scene_time),
    q4axang(v3f(-4.f, 1.5f, 1.f), game->scene_time / 3.6f)
  ));
}

////////////////////////////////////////////////////////////////////////////////
// Clockwise spin around the Z axis
////////////////////////////////////////////////////////////////////////////////

void _behavior_gear_rotate_cw(Game game, entity_t* e, float dt) {
  UNUSED(game);
  entity_rotate_a(e, v3back, dt);
}

////////////////////////////////////////////////////////////////////////////////
// Counter-clockwise spin around the Z axis
////////////////////////////////////////////////////////////////////////////////

void _behavior_gear_rotate_ccw(Game game, entity_t* e, float dt) {
  UNUSED(game);
  entity_rotate_a(e, v3front, dt);
}

////////////////////////////////////////////////////////////////////////////////
// Orients the entity to face the camera along its local +Z axis
////////////////////////////////////////////////////////////////////////////////

void _behavior_stare(Game game, entity_t* e, float dt) {
  UNUSED(dt);
  vec3 forward = v3norm(v3sub(game->camera.pos, e->pos));
  quat q = v3look(forward, v3up);
  entity_set_rotation(e, q);
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
  if (!shader_bind(shader)) return;

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

////////////////////////////////////////////////////////////////////////////////
// Render function for physically-based lighting
////////////////////////////////////////////////////////////////////////////////

void render_pbr(Game game, entity_t* e) {
  Shader shader = game->demo->shaders.light;
  if (!shader_bind(shader)) return;

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

////////////////////////////////////////////////////////////////////////////////
// Loading function to initialize the scene
////////////////////////////////////////////////////////////////////////////////

#include "light.h"

slotkey_t editor_light_bright;
slotkey_t editor_light_gizmo;
scene_unload_fn_t scene_load_gears(Game game) {

  demo_t* demo = game->demo;

  game->camera.pos = v3f(3, 2, 45);
  game->camera.front = v3front;
  game->demo->target = v3origin;
  game->demo->light_pos = v3f(40, 60, 0);

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
    .behavior = _behavior_camera_test,
  });

  //* Spinny Cube
  entity_add(&(entity_desc_t) {
    .name = S("Spinny-cube"),
    .model = demo->models.color_cube,
    .pos = v3f(-2, 0, 0),
    .onrender = render_basic,
    .behavior = _behavior_cubespin,
  }); //*/

  //* Staring Cube
  entity_add(&(entity_desc_t) {
    .name = S("Staring Cube"),
    .model = demo->models.color_cube,
    .pos = v3f(0, 0, 2),
    .onrender = render_basic,
    .behavior = _behavior_stare,
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
    .renderer = renderer_pbr,
    .behavior = _behavior_gear_rotate_cw,
  }); //*/

  //* Gear 2
  entity_add(&(entity_desc_t) {
    .name = S("Gear 2"),
    .model = demo->models.gear,
    .material = demo->materials.sands,
    .pos = v3f(20.5f, -1.5f, -12),
    .renderer = renderer_pbr,
    .behavior = _behavior_gear_rotate_ccw,
  }); //*/

  //* Gear 3
  entity_add(&(entity_desc_t) {
    .name = S("Gear 3"),
    .model = demo->models.gear,
    .material = demo->materials.mudds,
    .pos = v3f(43.f, -1.5f, -12),
    .renderer = renderer_pbr,
    .behavior = _behavior_gear_rotate_cw,
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
  slotkey_t crate_id =
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
    .tint = v4cv(v4f(0.8f, 0.3f, 0.6f, 1.0f)),
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

      int mindex = i + j;
      if (mindex < 0) mindex *= -1;
      mindex %= material->layers;
      entity_add(&(entity_desc_t) {
        .name = S("Ground Cube"),
        .model = demo->models.box,
        .material = material,
        .tint = b4white,
        .material_index = mindex,
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

  //* Particle test
  ParticleEffect effect = ps_add_effect(
    game->particle_system, S("test"), PF_DEFAULT, EF_DEFAULT
  );

  effect->emitter_defaults.duration = 0;
  //effect->emitter_defaults.offset = PI / 8.f;
  effect->emitter_defaults.rate = 40;
  effect->emitter_defaults.particle_defaults.speed = 15;
  effect->emitter_defaults.particle_defaults.duration = 2.f;
  effect->emitter_defaults.particle_variance.size = 0.3f;
  effect->emitter_defaults.particle_variance.speed = 4.f;
  effect->emitter_defaults.particle_variance.duration = 1.f;
  effect->on_particle_update = pb_gravity;
  effect->emitter_defaults.dir = q4axang(v3x, PI / 2.f);

  ParticleEmitter emitter = ps_add_emitter(effect);
  emitter->entity_id = crate_id;

  //*/

  return NULL;
}

