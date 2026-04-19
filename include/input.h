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

#ifndef WASP_INPUT_H_
#define WASP_INPUT_H_

#include "types.h"
#include "vec.h"

// The physical type of button being mapped
typedef enum button_type_t {
  BT_KEYBOARD,
  BT_MOUSE,
  BT_TOUCH
} button_type_t;

// Description of virtual (touch-screen) button and its mapping to a game action
typedef struct touch_button_t {
  vec2 pos;
  float radius;
} touch_button_t;

// Individual mapping for a game action to button input
typedef struct keybind_t {
  button_type_t type;
  int name;
  union {
    int key;
    int button;
    touch_button_t* touch;
  };
  bool pressed;
  bool triggered;
  bool released;
} keybind_t;

#define con_type keybind_t
#define con_prefix keymap
#include "span.h"
#undef con_type
#undef con_prefix

// Type tracking touch-screen finger info
typedef struct touch_t {
  uint64_t id;
  vec2 origin;
  vec2 pos;
  vec2 move;
  float pressure;
  bool triggered;
  bool released;
} touch_t;

#define con_type touch_t
#define con_prefix fingers
#include "span.h"
#undef con_type
#undef con_prefix

typedef struct input_touch_t {
  span_fingers_t fingers;
  index_t        count;
  const touch_t* first;
  const touch_t* second;
  const touch_t* third;
} input_touch_t;

typedef struct input_stick_t {
  vec2 left;
  vec2 right;
} input_stick_t;

typedef struct input_mouse_t {
  vec2 raw;
  vec2 raw_move;
  vec2 pos;
  vec2 move;
  float scroll;
} input_mouse_t;

typedef bool (*input_fn_t)(int);

typedef struct input_t {
  span_keymap_t keymap;
  input_mouse_t mouse;
  input_touch_t touch;
  input_stick_t stick;

  // aliases for the global input_... functions
  input_fn_t    triggered;
  input_fn_t    pressed;
  input_fn_t    released;
} input_t;

void            input_update(input_t*, vec2i window);
void            input_reset(input_t*);

void            input_pointer_lock(void);
bool            input_pointer_locked(void);
void            input_pointer_unlock(void);

bool            input_triggered(int input_name);
bool            input_pressed(int input_name);
bool            input_released(int input_name);

index_t         input_touch_count(input_t*);
const touch_t*  input_touch_get(input_t*, index_t);
const touch_t*  input_touch_from_id(input_t*, uint64_t);

#endif
