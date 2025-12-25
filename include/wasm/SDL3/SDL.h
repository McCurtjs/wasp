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

/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef WASM_SDL_H_
#define WASM_SDL_H_

#include "SDL_stdinc.h"
#include "SDL_scancode.h"
#include "SDL_keycode.h"

typedef Uint32 SDL_WindowID;

typedef int SDL_bool;
#define SDL_TRUE 1
#define SDL_FALSE 0

typedef enum {
  SDL_EVENT_WINDOW_RESIZED  = 0x206,

  SDL_EVENT_KEY_DOWN        = 0x300,
  SDL_EVENT_KEY_UP,

  SDL_EVENT_MOUSE_MOTION    = 0x400,
  SDL_EVENT_MOUSE_BUTTON_DOWN,
  SDL_EVENT_MOUSE_BUTTON_UP,
  SDL_EVENT_MOUSE_WHEEL,
} SDL_EventType;

#define SDL_RELEASED 0
#define SDL_PRESSED 1

#define SDL_BUTTON(X)       (1 << ((X)-1))
#define SDL_BUTTON_LEFT     1
#define SDL_BUTTON_MIDDLE   2
#define SDL_BUTTON_RIGHT    3
#define SDL_BUTTON_X1       4
#define SDL_BUTTON_X2       5
#define SDL_BUTTON_LMASK    SDL_BUTTON(SDL_BUTTON_LEFT)
#define SDL_BUTTON_MMASK    SDL_BUTTON(SDL_BUTTON_MIDDLE)
#define SDL_BUTTON_RMASK    SDL_BUTTON(SDL_BUTTON_RIGHT)
#define SDL_BUTTON_X1MASK   SDL_BUTTON(SDL_BUTTON_X1)
#define SDL_BUTTON_X2MASK   SDL_BUTTON(SDL_BUTTON_X2)

typedef enum SDL_AppResult
{
    SDL_APP_CONTINUE,   /**< Value that requests that the app continue from the main callbacks. */
    SDL_APP_SUCCESS,    /**< Value that requests termination with success from the main callbacks. */
    SDL_APP_FAILURE     /**< Value that requests termination with error from the main callbacks. */
} SDL_AppResult;

typedef struct SDL_Keysym
{
  //SDL_Scancode scancode;      /**< SDL physical key code - see ::SDL_Scancode for details */
  //SDL_Keycode sym;            /**< SDL virtual key code - see ::SDL_Keycode for details */
  Sint32 sym; // the key, for now
  Uint16 mod;                 /**< current key modifiers */
} SDL_Keysym;

/**
 *  Window state change event data (event.window.*)
 */
typedef struct SDL_WindowEvent
{
  Uint32 type;
  Uint64 timestamp;
  SDL_WindowID windowID;
  Sint32 data1;
  Sint32 data2;
} SDL_WindowEvent;

/**
 *  Keyboard button event structure (event.key.*)
 */
typedef struct SDL_KeyboardEvent
{
  Uint32 type;            /**< ::SDL_EVENT_KEY_DOWN or ::SDL_EVENT_KEY_UP */
  Uint64 timestamp;       /**< In nanoseconds, populated using SDL_GetTicksNS() */
  SDL_Keysym keysym;      /**< The key that was pressed or released */
  SDL_Scancode scancode;  /**< SDL physical key code */
  SDL_Keycode key;        /**< SDL virtual key code */
  bool down;
  bool repeat;
} SDL_KeyboardEvent;

/**
 *  Mouse motion event structure (event.motion.*)
 */
typedef struct SDL_MouseMotionEvent
{
  Uint32 type;        /**< ::SDL_EVENT_MOUSE_MOTION */
  Uint64 timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
  Uint32 state;       /**< The current button state */
  float x;            /**< X coordinate, relative to window */
  float y;            /**< Y coordinate, relative to window */
  float xrel;         /**< The relative motion in the X direction */
  float yrel;         /**< The relative motion in the Y direction */
} SDL_MouseMotionEvent;

/**
 *  Mouse button event structure (event.button.*)
 */
typedef struct SDL_MouseButtonEvent
{
  Uint32 type;
  Uint64 timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
  Uint8 button;       /**< The mouse button index */
  Uint8 state;        /**< ::SDL_PRESSED or ::SDL_RELEASED */
  Uint8 clicks;       /**< 1 for single-click, 2 for double-click, etc. */
  float x;            /**< X coordinate, relative to window */
  float y;            /**< Y coordinate, relative to window */
} SDL_MouseButtonEvent;

typedef union SDL_Event {
  Uint32 type;
  SDL_WindowEvent window;
  SDL_KeyboardEvent key;
  SDL_MouseMotionEvent motion;
  SDL_MouseButtonEvent button;

  Uint8 padding[32];
} SDL_Event;

int SDL_PushEvent(SDL_Event* event);
SDL_bool SDL_PollEvent(SDL_Event* event);

#endif
