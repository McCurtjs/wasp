#include "SDL3/SDL.h"
#include "types.h"
#include "wasm.h"
#include <stdlib.h>
#include <ctype.h>

void export(wasm_push_window_event) (uint event_type, int x, int y) {
  SDL_WindowEvent event = {
    .type = event_type,
    .timestamp = 0,
    .data1 = x,
    .data2 = y,
  };

  SDL_PushEvent((SDL_Event*)&event);
}

void export(wasm_push_mouse_button_event) (
  uint event_type, byte button, float x, float y
) {
  byte state =
    event_type == SDL_EVENT_MOUSE_BUTTON_DOWN ? SDL_PRESSED : SDL_RELEASED;

  SDL_MouseButtonEvent event = {
    .type = event_type,
    .button = button,
    .state = state,
    .x = x, .y = y
  };

  SDL_PushEvent((SDL_Event*)&event);
}

void export(wasm_push_mouse_motion_event) (
  float x, float y, float xrel, float yrel
) {
  SDL_MouseMotionEvent event = {
    .type = SDL_EVENT_MOUSE_MOTION,
    .state = 0, // will figure this out later
    .x = x,
    .y = y,
    .xrel = xrel,
    .yrel = yrel
  };

  SDL_PushEvent((SDL_Event*)&event);
}

void export(wasm_push_keyboard_event) (
  uint event_type, int key, uint mod, uint repeat
) {
  SDL_KeyboardEvent event = {
    .type = event_type,
    .state = SDL_EVENT_KEY_DOWN ? SDL_PRESSED : SDL_RELEASED,
    .repeat = repeat,
    .keysym = { .sym = tolower(key), .mod = mod }
  };

  SDL_PushEvent((SDL_Event*)&event);
}
