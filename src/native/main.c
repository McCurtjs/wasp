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

bool game_continue = true;
bool game_loaded = false;

struct {
  SDL_Window* window;
  SDL_GLContext gl_context;
  uint64_t previous_time;
  Game* game;
} app = { 0 };

////////////////////////////////////////////////////////////////////////////////

SDL_AppResult SDL_AppInit(void** app_state, int argc, char* argv[]) {
  UNUSED(app_state);
  UNUSED(argc);
  UNUSED(argv);

  str_write("Here we go!");

  app_defaults_t defaults = (app_defaults_t){
    .window = v2i(640, 480),
    .title = "Game Title",
  };
  wasp_init(&defaults);

  app.game = game_init(defaults.window.w, defaults.window.h);
  app.game->title = defaults.title;

  SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
  app.window = SDL_CreateWindow(
    app.game->title,
    app.game->window.x,
    app.game->window.y,
    flags
  );

  if (!app.window) return SDL_APP_FAILURE;

  app.gl_context = SDL_GL_CreateContext(app.window);

  if (!app.gl_context) return SDL_APP_FAILURE;

  glClearColor(0.2f, 0.2f, 0.2f, 1);
  glClearDepth(1);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glDepthFunc(GL_LEQUAL);

  wasp_preload(app.game);

  SDL_SetWindowSize(app.window, app.game->window.w, app.game->window.h);
  SDL_SetWindowTitle(app.window, app.game->title);

  glViewport(0, 0, app.game->window.x, app.game->window.y);

  app.previous_time = SDL_GetTicks();

  return SDL_APP_CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////

SDL_AppResult SDL_AppIterate(void* app_state) {
  UNUSED(app_state);

  if (!game_continue) {
    return SDL_APP_SUCCESS;
  }

  uint64_t current_time = SDL_GetTicks();
  float dt = (float)(current_time - app.previous_time) / 1000.f;
  app.previous_time = current_time;

  if (!game_loaded) {
    if (wasp_load(app.game, 0, dt)) {
      str_write("Loaded");
      game_loaded = true;
    }

    return SDL_APP_CONTINUE;
  }

  wasp_update(app.game, dt);
  wasp_render(app.game);

  SDL_GL_SwapWindow(app.window);

  if (app.game->should_exit)
    return SDL_APP_SUCCESS;
  return SDL_APP_CONTINUE;
}

////////////////////////////////////////////////////////////////////////////////

SDL_AppResult SDL_AppEvent(void* app_state, SDL_Event* event) {
  UNUSED(app_state);
  return process_system_event(app.game, event);
}

////////////////////////////////////////////////////////////////////////////////

void SDL_AppQuit(void* app_state, SDL_AppResult result) {
  UNUSED(app_state);
  UNUSED(result);

  fflush(stdout);

  if (app.gl_context) {
    SDL_GL_DestroyContext(app.gl_context);
  }

  if (app.window) {
    SDL_DestroyWindow(app.window);
  }
}
