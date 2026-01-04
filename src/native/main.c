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

#include "types.h"
#include "game.h"
#include "wasp.h"
#include "system_events.h"

#include "str.h"

#define SDL_MAIN_USE_CALLBACKS
#include "SDL3/SDL_main.h"

#include "SDL3/SDL.h"
#include "gl.h"
#include "stdio.h" // fflush

static struct {
  SDL_Window* window;
  SDL_GLContext gl_context;
  SDL_Thread* loading_thread;
  bool loading_done;
  uint64_t previous_time;
  Game game;
} app = { 0 };

////////////////////////////////////////////////////////////////////////////////

static int SDLCALL _loading_thread_fn(void* data) {
  game_set_local(data);
  return wasp_preload(data);
}

////////////////////////////////////////////////////////////////////////////////

SDL_AppResult SDL_AppInit(void** app_state, int argc, char* argv[]) {
  UNUSED(app_state);
  UNUSED(argc);
  UNUSED(argv);

  str_write("[App.Init] Here we go!");

  app.game = game_init(640, 480);

  str_log("[App.Init] Initializing game: {}", app.game->title);

  SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
  app.window = SDL_CreateWindow(
    app.game->title->begin,
    app.game->window.x,
    app.game->window.y,
    flags
  );

  if (!app.window) return SDL_APP_FAILURE;

  app.gl_context = SDL_GL_CreateContext(app.window);
  SDL_GL_SetSwapInterval(0);

  if (!app.gl_context) return SDL_APP_FAILURE;

  glClearColor(0.2f, 0.2f, 0.2f, 1);
  glClearDepth(1);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDepthFunc(GL_LEQUAL);

  SDL_SetWindowSize(app.window, app.game->window.w, app.game->window.h);
  SDL_SetWindowTitle(app.window, app.game->title->begin);

  glViewport(0, 0, app.game->window.x, app.game->window.y);

  app.previous_time = SDL_GetTicks();

  app.loading_thread = SDL_CreateThread(_loading_thread_fn, "load", &app.game);

  if (!app.loading_thread) {
    str_log("[App.Init] Failed to create loading thread: {}", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  return SDL_APP_CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////

SDL_AppResult SDL_AppIterate(void* app_state) {
  UNUSED(app_state);

  uint64_t current_time = SDL_GetTicks();
  float dt = (float)(current_time - app.previous_time) / 1000.f;
  app.previous_time = current_time;

  if (!app.loading_done) {
    float load_time = (float)current_time / 1000.f;

    if (app.loading_thread) {
      SDL_ThreadState state = SDL_GetThreadState(app.loading_thread);

      if (state == SDL_THREAD_COMPLETE) {
        int result;
        SDL_WaitThread(app.loading_thread, &result);
        if (!result) {
          str_write("[App.Preload] Failed preload step");
          return SDL_APP_FAILURE;
        }
        str_log("[App.Preload] Loaded - time elapsed: {}", load_time);
        app.loading_thread = NULL;
      }
    }

    app.loading_done = wasp_load(app.game, !!app.loading_thread, dt);

    if (!app.loading_thread && app.loading_done) {
      str_log("[App.Load] Loaded - time elapsed: {}", load_time);
    }
    else {
      SDL_GL_SwapWindow(app.window);
      return SDL_APP_CONTINUE;
    }
  }

  shader_check_updates();

  wasp_update(app.game, dt);

  if (app.game->should_exit) {
    return SDL_APP_SUCCESS;
  }

  wasp_render(app.game);

  SDL_GL_SwapWindow(app.window);

  if (app.game->should_exit)
    return SDL_APP_SUCCESS;
  return SDL_APP_CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////

SDL_AppResult SDL_AppEvent(void* app_state, SDL_Event* event) {
  UNUSED(app_state);
  return event_process_system(app.game, event);
}

////////////////////////////////////////////////////////////////////////////////

void SDL_AppQuit(void* app_state, SDL_AppResult result) {
  UNUSED(app_state);
  UNUSED(result);

  str_log("[App.Quit] Closing with code: {}", (int)result);

  fflush(stdout);

  game_delete(&app.game);

  if (app.gl_context) {
    SDL_GL_DestroyContext(app.gl_context);
  }

  if (app.window) {
    SDL_DestroyWindow(app.window);
  }
}
