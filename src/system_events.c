#include "system_events.h"

#include <SDL3/SDL.h>
#include "gl.h"

#include "wasm.h"
#include "render_target.h"

extern bool game_continue;

static uint _process_window_resize(Game* game, SDL_WindowEvent* window) {
  game->window.w = window->data1;
  game->window.h = window->data2;
  str_log("Event: resizing window - {}", game->window);
  rt_resize(game->textures.render_target, game->window);
  float aspect = i2aspect(game->window);
  game->camera.persp.aspect = aspect;
  camera_build_perspective(&game->camera);
  //game->camera.ortho.left = -game->camera.ortho.top * aspect;
  //game->camera.ortho.right = -game->camera.ortho.bottom * aspect;
  //camera_build_orthographic(&game->camera);
  return SDL_APP_CONTINUE;
}

static uint _process_mouse_button_down(Game* game, SDL_MouseButtonEvent* button) {
  keybind_t* span_foreach(keybind, game->input.keymap) {
    if (keybind->mouse && keybind->key == button->button && !keybind->pressed) {
      keybind->triggered = true;
      keybind->pressed = true;
    }
  }

  return SDL_APP_CONTINUE;
}

static uint _process_mouse_button_up(Game* game, SDL_MouseButtonEvent* button) {
  keybind_t* span_foreach(keybind, game->input.keymap) {
    if (keybind->mouse && keybind->key == button->button) {
      keybind->pressed = false;
      keybind->released = true;
    }
  }

  return SDL_APP_CONTINUE;
}

static uint _process_mouse_motion(Game* game, SDL_MouseMotionEvent* motion) {
  game->input.mouse.pos = v2f(motion->x, motion->y);

  // you can get many mouse move inputs per frame, so accumulate motion
  game->input.mouse.move.x += motion->xrel;
  game->input.mouse.move.y += motion->yrel;

  return SDL_APP_CONTINUE;
}

static uint _process_key_down(Game* game, SDL_KeyboardEvent* key) {
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

static uint _process_key_up(Game* game, SDL_KeyboardEvent* key) {
  keybind_t* span_foreach(keybind, game->input.keymap) {
    if (key->key == (uint)keybind->key && !keybind->mouse) {
      keybind->pressed = false;
      keybind->released = true;
    }
  }

  return SDL_APP_CONTINUE;
}

uint process_system_event(Game* game, void* system_event) {
  SDL_Event* event = system_event;
  
  switch(event->type) {

#ifndef __WASM__
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      return SDL_APP_SUCCESS;
#endif

    case SDL_EVENT_WINDOW_RESIZED:
      return _process_window_resize(game, &event->window);

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      return _process_mouse_button_down(game, &event->button);

    case SDL_EVENT_MOUSE_BUTTON_UP:
      return _process_mouse_button_up(game, &event->button);

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
