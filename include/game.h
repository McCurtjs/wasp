#ifndef _GAME_H_
#define _GAME_H_

#include "vec.h"
#include "camera.h"
#include "entity.h"
#include "shader.h"
#include "texture.h"
#include "render_target.h"

#define con_type int
#include "span.h"
#include "array.h"
#undef con_type

typedef struct Game_Shaders {
  Shader loading;
  Shader frame;
  Shader basic;
  Shader light;
  Shader warhol;
} Game_Shaders;

typedef struct Game_Models {
  Model color_cube;
  Model gizmo;
  Model grid;
  Model box;
  Model gear;
  Model player;
  Model level_test;
  Model level_1;
} Game_Models;

typedef struct Game_Textures {
  texture_t flat;
  texture_t flats;
  texture_t crate;
  texture_t brass;
  texture_t brasn;
  texture_t tiles;
  texture_t grass;
  texture_t grasn;
  texture_t grasr;
  texture_t level;
  texture_t player;
  RenderTarget render_target;
} Game_Textures;

typedef struct Game_Materials {
  Material grass;
  Material sands;
  Material tiles;
  Material crate;
  Material mudds;
  Material renderite;
} Game_Materials;

#define game_key_count 10
#define game_mouse_button_count 3
#define game_button_input_count (game_key_count + game_mouse_button_count)

typedef struct keybind_t {
  int name; // pair with an enum value for the key name
  int key;
  bool mouse;
  bool pressed;
  bool triggered;
  bool released;
} keybind_t;

#define con_type keybind_t
#define con_prefix keymap
#include "span.h"
#undef con_type
#undef con_prefix

typedef struct input_mouse_t {
  vec2 pos;
  vec2 move;
} input_mouse_t;

typedef struct input_t {
  span_keymap_t keymap;
  input_mouse_t mouse;
} input_t;

typedef struct app_defaults_t {
  vec2i window;
  const char* title;
} app_defaults_t;

typedef struct Game {
  vec2i window;
  const char* title;
  Camera camera;

  vec3 target;
  vec4 light_pos;

  input_t input;

  Game_Shaders shaders;
  Game_Models models;
  Game_Textures textures;
  Game_Materials materials;

  Array_entity entities;

  uint level;
  bool should_exit;

} Game;

Game* game_init(int window_width, int window_height);

void game_reset(Game* game);
void game_quit(Game* game);

void game_add_entity(Game* game, const Entity* entity);

void game_update(Game* game, float dt);
void game_render(Game* game);
void game_cleanup(Game* game);

bool input_triggered(Game* game, int input_name);
bool input_pressed(Game* game, int input_name);
bool input_released(Game* game, int input_name);

#endif
