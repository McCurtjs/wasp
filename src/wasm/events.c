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

#include "SDL3/SDL.h"
#include "types.h"
#include "wasm.h"
#include "str.h"
#include <stdlib.h>
#include <ctype.h>

#include "system_events.h"

void export(wasm_push_window_event) (uint event_type, int x, int y) {
  SDL_WindowEvent event = {
    .type = event_type,
    .timestamp = 0,
    .data1 = x,
    .data2 = y,
  };

  event_process_system(game_get_active(), &event);
}

void export(wasm_push_mouse_button_event) (
  uint event_type, uint source, byte button, float x, float y
) {
  SDL_MouseID which = source < 0 ? SDL_TOUCH_MOUSEID : source;

  SDL_MouseButtonEvent event = {
    .type = event_type,
    .which = which,
    .button = button,
    .x = x, .y = y
  };

  event_process_system(game_get_active(), &event);
}

void export(wasm_push_mouse_motion_event) (
  int source, float x, float y, float xrel, float yrel
) {
  SDL_MouseID which = source < 0 ? SDL_TOUCH_MOUSEID : source;

  SDL_MouseMotionEvent event = {
    .type = SDL_EVENT_MOUSE_MOTION,
    .which = which,
    .x = x,
    .y = y,
    .xrel = xrel,
    .yrel = yrel
  };

  event_process_system(game_get_active(), &event);
}

void export(wasm_push_mouse_wheel_event) (
  int deltaX, int deltaY
) {
  SDL_MouseWheelEvent event = {
    .type = SDL_EVENT_MOUSE_WHEEL,
    .x = -(float)deltaX / 100.f,
    .y = -(float)deltaY / 100.f,
    .integer_x = deltaX,
    .integer_y = deltaY,
  };

  event_process_system(game_get_active(), &event);
}

void export(wasm_push_keyboard_event) (
  uint event_type, int key, uint mod, uint repeat
) {
  UNUSED(mod);
  SDL_KeyboardEvent event = {
    .type = event_type,
    .down = event_type == SDL_EVENT_KEY_DOWN,
    .repeat = repeat,
    .key = tolower(key),
  };

  event_process_system(game_get_active(), &event);
}

void export(wasm_push_touch_event) (
  uint event_type, uint finger_id, float pressure, float x, float y
) {

  str_log("Event translator - event: {}, id: {}, x: {:.3}, y: {:.3}",
    event_type, finger_id, x, y);

  SDL_TouchFingerEvent event = {
    .type = event_type,
    .fingerID = finger_id,
    .x = x,
    .y = y,
    .pressure = pressure,
  };

  event_process_system(game_get_active(), &event);
}
