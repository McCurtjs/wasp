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

#include "input.h"
#include "game.h"

// imports for mouse-locking
#ifdef __WASM__
extern void js_pointer_lock(void);
extern void js_pointer_unlock(void);
#else
#include "SDL3/SDL.h"
#endif

////////////////////////////////////////////////////////////////////////////////
// Update active input by clearing per-frame values
////////////////////////////////////////////////////////////////////////////////

void input_update(input_t* input) {
  keybind_t* span_foreach(keybind, input->keymap) {
    keybind->triggered = false;
    keybind->released = false;
  }
  input->mouse.move = v2zero;
}

////////////////////////////////////////////////////////////////////////////////
// Locks the cursor position and hides it
////////////////////////////////////////////////////////////////////////////////

static bool lazy_lock_check = false;
void input_pointer_lock(void) {
#ifdef __WASM__
  js_pointer_lock();
#else
  SDL_Window* sdl_window = SDL_GL_GetCurrentWindow();
  bool success = SDL_SetWindowRelativeMouseMode(sdl_window, true);
  if (!success) {
    str_write(SDL_GetError());
  }
  lazy_lock_check = true;
#endif
}

bool input_pointer_locked(void) {
  return lazy_lock_check;
}

////////////////////////////////////////////////////////////////////////////////
// Unlocks and unhides the cursor position
////////////////////////////////////////////////////////////////////////////////

void input_pointer_unlock(void) {
#ifdef __WASM__
  js_pointer_unlock();
#else
  SDL_Window* sdl_window = SDL_GL_GetCurrentWindow();
  bool success = SDL_SetWindowRelativeMouseMode(sdl_window, false);
  if (!success) {
    str_log("[Input.pointer] Failed to unlock: {}", SDL_GetError());
  }
  else {
    vec2i window = game_get_active()->window;
    vec2 center = v2scale(v2vi(window), 0.5f);
    SDL_WarpMouseInWindow(sdl_window, center.x, center.y);
    lazy_lock_check = false;
  }
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Get whether the named input was triggered since the last frame
////////////////////////////////////////////////////////////////////////////////

bool input_triggered(int input_name) {
  span_keymap_t keymap = game_get_active()->input.keymap;
  keybind_t* span_foreach(keybind, keymap) {
    if (keybind->name == input_name && keybind->triggered) {
      return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// Get whether the named input is currently pressed
////////////////////////////////////////////////////////////////////////////////

bool input_pressed(int input_name) {
  span_keymap_t keymap = game_get_active()->input.keymap;
  keybind_t* span_foreach(keybind, keymap) {
    if (keybind->name == input_name && keybind->pressed) {
      return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// Get whether the named input was released since the last frame
////////////////////////////////////////////////////////////////////////////////

bool input_released(int input_name) {
  span_keymap_t keymap = game_get_active()->input.keymap;
  keybind_t* span_foreach(keybind, keymap) {
    if (keybind->name == input_name && keybind->released) {
      return true;
    }
  }
  return false;
}
