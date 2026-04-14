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

#include "system_events.h"

#include <SDL3/SDL.h>
#include "gl.h"

#include "wasm.h"
#include "render_target.h"

////////////////////////////////////////////////////////////////////////////////
// Window system events
////////////////////////////////////////////////////////////////////////////////

static void _process_window_resize(Game game, SDL_WindowEvent* window) {
  vec2i new_size = v2i(window->data1, window->data2);
  *(vec2i*)(&game->window) = new_size;

  str_log("Event: resizing window - {}", game->window);
  rt_resize(NULL, game->window);
  float aspect = i2aspect(game->window);

  if (game->camera.type == CAMERA_ORTHOGRAPHIC) {
    float top = game->camera.orthographic.top;
    float bottom = game->camera.orthographic.bottom;
    game->camera.orthographic.left = -top * aspect;
    game->camera.orthographic.right = -bottom * aspect;
  }
  else {
    game->camera.perspective.aspect = aspect;
  }

  camera_build(&game->camera);

  if (game->on_window_resize) game->on_window_resize(game);
}

////////////////////////////////////////////////////////////////////////////////
// Keyboard inputs
////////////////////////////////////////////////////////////////////////////////

static void _process_key_down(Game game, SDL_KeyboardEvent* key) {
  if (key->repeat) return;

  keybind_t* span_foreach(keybind, game->input.keymap) {
    if (keybind->type != BT_KEYBOARD) continue;

    if (key->key == (uint)keybind->key) {

      // sometimes when getting a key-up event, it'll also send a
      // "helpful" reminder that other keys are still down... avoid double
      // triggering when that happens (makes the repeat check redundant,
      // but at least that check intuitively makes sense)
      if (keybind->pressed) continue;

      keybind->triggered = true;
      keybind->pressed = true;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static void _process_key_up(Game game, SDL_KeyboardEvent* key) {
  keybind_t* span_foreach(keybind, game->input.keymap) {
    if (keybind->type != BT_KEYBOARD) continue;

    if (key->key == (uint)keybind->key) {
      keybind->pressed = false;
      keybind->released = true;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Mouse events
////////////////////////////////////////////////////////////////////////////////

static void _process_mouse_btn_down(Game game, SDL_MouseButtonEvent* button) {
  input_t* input = &game->input;
  if (button->which == SDL_TOUCH_MOUSEID && input->touch.fingers.begin) return;

  keybind_t* span_foreach(keybind, game->input.keymap) {
    if (keybind->type != BT_MOUSE) continue;

    if (keybind->key == button->button && !keybind->pressed) {
      keybind->triggered = true;
      keybind->pressed = true;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static void _process_mouse_btn_up(Game game, SDL_MouseButtonEvent* button) {
  input_t* input = &game->input;
  if (button->which == SDL_TOUCH_MOUSEID && input->touch.fingers.begin) return;

  keybind_t* span_foreach(keybind, game->input.keymap) {
    if (keybind->type != BT_MOUSE) continue;

    if (keybind->key == button->button) {
      keybind->pressed = false;
      keybind->released = true;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

static void _process_mouse_motion(Game game, SDL_MouseMotionEvent* motion) {
  input_t* input = &game->input;
  if (motion->which == SDL_TOUCH_MOUSEID && input->touch.fingers.begin) return;

  game->input.mouse.raw = v2f(motion->x, motion->y);

  // you can get many mouse move inputs per frame, so accumulate motion
  game->input.mouse.raw_move.x += motion->xrel;
  game->input.mouse.raw_move.y += motion->yrel;
}

////////////////////////////////////////////////////////////////////////////////

static void _process_mouse_wheel(Game game, SDL_MouseWheelEvent* wheel) {
  game->input.mouse.scroll = wheel->y;
}

////////////////////////////////////////////////////////////////////////////////
// Touch input events
////////////////////////////////////////////////////////////////////////////////
// Note: if multi-touch fingers aren't set, these will route to mouse inputs

// Gets the first matching touch point for a given id
static touch_t* _touch_get(input_touch_t* touch, uint64_t id) {
  touch_t* span_foreach(ret, touch->fingers) {
    if (ret->id == id) return ret;
  }
  return NULL;
}

// Transforms the touch input to NDC space, from -1 to 1 with center origin
static vec2 _touch_to_ndc(vec2 pos) {
  return v2scale(v2sub(pos, v2f(0.5f, 0.5f)), 2.0f);
}

////////////////////////////////////////////////////////////////////////////////

static void _process_finger_down(Game game, SDL_TouchFingerEvent* touch) {
  if (!game->input.touch.fingers.begin) return;

  // a touch id of 0 means "unused" - find the first unused slot for this event.
  // if there are no unused slots we've gone over our multi-touch limit (defined
  //    by the size of the touch_t span in game->input).
  touch_t* input = _touch_get(&game->input.touch, 0);
  if (!input) return;

  vec2 pos = _touch_to_ndc(v2f(touch->x, touch->y));

  *input = (touch_t) {
    .id = touch->fingerID,
    .origin = pos,
    .pos = pos,
    .move = v2zero,
    .pressure = touch->pressure,
    .triggered = true,
    .released = false,
  };
}

////////////////////////////////////////////////////////////////////////////////

static void _process_finger_motion(Game game, SDL_TouchFingerEvent* touch) {
  if (!game->input.touch.fingers.begin) return;

  touch_t* slot = _touch_get(&game->input.touch, touch->fingerID);
  if (!slot) return;

  slot->pos = _touch_to_ndc(v2f(touch->x, touch->y));
  slot->move = v2add(slot->move, v2f(touch->dx, -touch->dy));
  slot->pressure = touch->pressure;
}

////////////////////////////////////////////////////////////////////////////////

static void _process_finger_up(Game game, SDL_TouchFingerEvent* touch) {
  touch_t* input = _touch_get(&game->input.touch, touch->fingerID);
  if (!input) return;

  input->released = true;
  input->pressure = 0.0f;
  input->pos = _touch_to_ndc(v2f(touch->x, touch->y));
}

////////////////////////////////////////////////////////////////////////////////

static void _process_finger_cancelled(Game game, SDL_TouchFingerEvent* touch) {
  touch_t* input = _touch_get(&game->input.touch, touch->fingerID);
  if (!input) return;

  *input = (touch_t){ 0 };
}

////////////////////////////////////////////////////////////////////////////////
// Event dispatch
////////////////////////////////////////////////////////////////////////////////

uint event_process_system(Game game, void* system_event) {
  SDL_Event* event = system_event;
  
  switch(event->type) {

#ifndef __WASM__
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      return SDL_APP_SUCCESS;
#endif

    case SDL_EVENT_WINDOW_RESIZED:
      _process_window_resize(game, &event->window);
      break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      _process_mouse_btn_down(game, &event->button);
      break;

    case SDL_EVENT_MOUSE_BUTTON_UP:
      _process_mouse_btn_up(game, &event->button);
      break;

    case SDL_EVENT_MOUSE_WHEEL:
      _process_mouse_wheel(game, &event->wheel);
      break;

    case SDL_EVENT_MOUSE_MOTION:
      _process_mouse_motion(game, &event->motion);
      break;

    case SDL_EVENT_FINGER_DOWN:
      _process_finger_down(game, &event->tfinger);
      break;

    case SDL_EVENT_FINGER_MOTION:
      _process_finger_motion(game, &event->tfinger);
      break;

    case SDL_EVENT_FINGER_UP:
      _process_finger_up(game, &event->tfinger);
      break;

    case SDL_EVENT_FINGER_CANCELED:
      _process_finger_cancelled(game, &event->tfinger);
      break;

    case SDL_EVENT_KEY_DOWN:
      _process_key_down(game, &event->key);
      break;

    case SDL_EVENT_KEY_UP:
      _process_key_up(game, &event->key);
      break;

    default:
      return SDL_APP_CONTINUE;
  }

  return SDL_APP_CONTINUE;
}
