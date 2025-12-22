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

static input_t _input_default = { 0 };
static input_t* _input = &_input_default;

////////////////////////////////////////////////////////////////////////////////
// Use provided input struct for global input management
////////////////////////////////////////////////////////////////////////////////

void input_set(input_t* input) {
  if (input == NULL) input = &_input_default;
  _input = input;
}

////////////////////////////////////////////////////////////////////////////////

void input_unset(const input_t* input) {
  if (_input == input) input_set(NULL);
}

////////////////////////////////////////////////////////////////////////////////
// Update active input by clearing per-frame values
////////////////////////////////////////////////////////////////////////////////

void input_update(input_t* input) {
  if (input == NULL) input = _input;
  keybind_t* span_foreach(keybind, input->keymap) {
    keybind->triggered = false;
    keybind->released = false;
  }
  input->mouse.move = v2zero;
}

////////////////////////////////////////////////////////////////////////////////
// Get whether the named input was triggered since the last frame
////////////////////////////////////////////////////////////////////////////////

bool input_triggered(int input_name) {
  keybind_t* span_foreach(keybind, _input->keymap) {
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
  keybind_t* span_foreach(keybind, _input->keymap) {
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
  keybind_t* span_foreach(keybind, _input->keymap) {
    if (keybind->name == input_name && keybind->released) {
      return true;
    }
  }
  return false;
}
