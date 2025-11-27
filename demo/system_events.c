#include "system_events.h"

#include <SDL3/SDL.h>
#include "gl.h"

#include "wasm.h"
#include "render_target.h"

extern bool game_continue;

void process_system_events(Game* game) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    switch(event.type) {

#ifndef __WASM__
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
        game_continue = false;
      } break;
#endif

      case SDL_EVENT_WINDOW_RESIZED: {
        game->window.w = event.window.data1;
        game->window.h = event.window.data2;
        str_log("Event: resizing window - {}", game->window);
        rt_resize(game->textures.render_target, game->window);
        float aspect = i2aspect(game->window);
        game->camera.persp.aspect = aspect;
        camera_build_perspective(&game->camera);
        //game->camera.ortho.left = -game->camera.ortho.top * aspect;
        //game->camera.ortho.right = -game->camera.ortho.bottom * aspect;
        //camera_build_orthographic(&game->camera);
      } break;

      case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        if (event.button.button & SDL_BUTTON_LMASK) {
          game->input.pressed.lmb = TRUE;
          game->input.triggered.lmb = TRUE;
        }
        if (event.button.button & SDL_BUTTON_MMASK) {
          game->input.pressed.mmb = TRUE;
          game->input.triggered.mmb = TRUE;
        }
        if (event.button.button & SDL_BUTTON_RMASK) {
          game->input.pressed.rmb = TRUE;
          game->input.triggered.rmb = TRUE;
        }
      } break;

      case SDL_EVENT_MOUSE_BUTTON_UP: {
        // web uses a button id instead of a mask for up events,
        // but it doesn't even match the mask index -_-
        if (event.button.button & SDL_BUTTON_LMASK) {
          game->input.pressed.lmb = FALSE;
          game->input.released.lmb = TRUE;
        }
        if (event.button.button & SDL_BUTTON_MMASK) {
          game->input.pressed.mmb = FALSE;
          game->input.released.mmb = TRUE;
        }
        if (event.button.button & SDL_BUTTON_RMASK) {
          game->input.pressed.rmb = FALSE;
          game->input.released.rmb = TRUE;
        }
      } break;

      case SDL_EVENT_MOUSE_MOTION: {
        game->input.mouse.pos = v2f(event.motion.x, event.motion.y);

        // you can get many mouse move inputs per frame, so accumulate motion
        game->input.mouse.move.x += event.motion.xrel;
        game->input.mouse.move.y += event.motion.yrel;
      } break;

      case SDL_EVENT_KEY_DOWN: {
        if (event.key.repeat) break;
        for (uint i = 0; i < game_key_count; ++i) {
          if (event.key.raw == game->input.mapping.keys[i]) {

            // sometimes when getting a key-up event, it'll also send a
            // "helpful" reminder that other keys are still down... avoid double
            // triggering when that happens (makes the repeat check redundant,
            // but at least that check intuitively makes sense)
            if (game->input.pressed.keys[i]) return;

            game->input.pressed.keys[i] = TRUE;
            game->input.triggered.keys[i] = TRUE;
          }
        }
      }; break;

      case SDL_EVENT_KEY_UP: {
        for (uint i = 0; i < game_key_count; ++i) {
          if (event.key.raw == game->input.mapping.keys[i]) {
            game->input.pressed.keys[i] = FALSE;
            game->input.released.keys[i] = TRUE;
          }
        }
      }; break;
    }
  }
}
