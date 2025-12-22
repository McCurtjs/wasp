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

static uint _process_window_resize(Game game, SDL_WindowEvent* window) {
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

  game->camera.perspective.aspect = aspect;
  camera_build(&game->camera);

  if (game->on_window_resize) game->on_window_resize(game);
  return SDL_APP_CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////

static uint _process_mouse_btn_down(Game game, SDL_MouseButtonEvent* button) {
  keybind_t* span_foreach(keybind, game->input.keymap) {
    if (keybind->mouse && keybind->key == button->button && !keybind->pressed) {
      keybind->triggered = true;
      keybind->pressed = true;
    }
  }

  return SDL_APP_CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////

static uint _process_mouse_btn_up(Game game, SDL_MouseButtonEvent* button) {
  keybind_t* span_foreach(keybind, game->input.keymap) {
    if (keybind->mouse && keybind->key == button->button) {
      keybind->pressed = false;
      keybind->released = true;
    }
  }

  return SDL_APP_CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////

static uint _process_mouse_motion(Game game, SDL_MouseMotionEvent* motion) {
  game->input.mouse.pos = v2f(motion->x, motion->y);

  // you can get many mouse move inputs per frame, so accumulate motion
  game->input.mouse.move.x += motion->xrel;
  game->input.mouse.move.y += motion->yrel;

  return SDL_APP_CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////

static uint _process_key_down(Game game, SDL_KeyboardEvent* key) {
  if (key->repeat) return SDL_APP_CONTINUE;

  keybind_t* span_foreach(keybind, game->input.keymap) {
    if (key->key == (uint)keybind->key && !keybind->mouse) {

      // sometimes when getting a key-up event, it'll also send a
      // "helpful" reminder that other keys are still down... avoid double
      // triggering when that happens (makes the repeat check redundant,
      // but at least that check intuitively makes sense)
      if (keybind->pressed) continue;

      keybind->triggered = true;
      keybind->pressed = true;
    }
  }

  return SDL_APP_CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////

static uint _process_key_up(Game game, SDL_KeyboardEvent* key) {
  keybind_t* span_foreach(keybind, game->input.keymap) {
    if (key->key == (uint)keybind->key && !keybind->mouse) {
      keybind->pressed = false;
      keybind->released = true;
    }
  }

  return SDL_APP_CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////

uint event_process_system(Game game, void* system_event) {
  SDL_Event* event = system_event;
  
  switch(event->type) {

#ifndef __WASM__
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      return SDL_APP_SUCCESS;
#endif

    case SDL_EVENT_WINDOW_RESIZED:
      return _process_window_resize(game, &event->window);

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      return _process_mouse_btn_down(game, &event->button);

    case SDL_EVENT_MOUSE_BUTTON_UP:
      return _process_mouse_btn_up(game, &event->button);

    case SDL_EVENT_MOUSE_MOTION:
      return _process_mouse_motion(game, &event->motion);

    case SDL_EVENT_KEY_DOWN:
      return _process_key_down(game, &event->key);

    case SDL_EVENT_KEY_UP:
      return _process_key_up(game, &event->key);

    default:
      return SDL_APP_CONTINUE;
  }
}
