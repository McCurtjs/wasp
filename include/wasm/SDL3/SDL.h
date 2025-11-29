#ifndef _WASM_SDL_H_
#define _WASM_SDL_H_

typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;
typedef unsigned long Uint64;
typedef int Sint32;
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
#define SDL_BUTTON_MIDDLE   3 // <- SDL and javascript
#define SDL_BUTTON_RIGHT    2 // <- swap these values
#define SDL_BUTTON_X1       4
#define SDL_BUTTON_X2       5
#define SDL_BUTTON_LMASK    SDL_BUTTON(SDL_BUTTON_LEFT)
#define SDL_BUTTON_MMASK    SDL_BUTTON(SDL_BUTTON_MIDDLE)
#define SDL_BUTTON_RMASK    SDL_BUTTON(SDL_BUTTON_RIGHT)
#define SDL_BUTTON_X1MASK   SDL_BUTTON(SDL_BUTTON_X1)
#define SDL_BUTTON_X2MASK   SDL_BUTTON(SDL_BUTTON_X2)

// SDL has a different keymapping than web keycodes...
#define SDLK_LEFT   37
#define SDLK_UP     38
#define SDLK_RIGHT  39
#define SDLK_DOWN   40
#define SDLK_HOME   36 // 61
#define SDLK_EQUALS 187
#define SDLK_MINUS  189

/**
 * Enumeration of valid key mods (possibly OR'd together).
 */
typedef enum
{
  SDL_KMOD_NONE = 0x0000,
  SDL_KMOD_LSHIFT = 0x0001,
  SDL_KMOD_RSHIFT = 0x0002,
  SDL_KMOD_LCTRL = 0x0040,
  SDL_KMOD_RCTRL = 0x0080,
  SDL_KMOD_LALT = 0x0100,
  SDL_KMOD_RALT = 0x0200,
  SDL_KMOD_LGUI = 0x0400,
  SDL_KMOD_RGUI = 0x0800,
  SDL_KMOD_NUM = 0x1000,
  SDL_KMOD_CAPS = 0x2000,
  SDL_KMOD_MODE = 0x4000,
  SDL_KMOD_SCROLL = 0x8000,

  SDL_KMOD_CTRL = SDL_KMOD_LCTRL | SDL_KMOD_RCTRL,
  SDL_KMOD_SHIFT = SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT,
  SDL_KMOD_ALT = SDL_KMOD_LALT | SDL_KMOD_RALT,
  SDL_KMOD_GUI = SDL_KMOD_LGUI | SDL_KMOD_RGUI,

  SDL_KMOD_RESERVED = SDL_KMOD_SCROLL /* This is for source-level compatibility with SDL 2.0.0. */
} SDL_Keymod;

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
  Uint32 type;        /**< ::SDL_EVENT_KEY_DOWN or ::SDL_EVENT_KEY_UP */
  Uint64 timestamp;   /**< In nanoseconds, populated using SDL_GetTicksNS() */
  SDL_Keysym keysym;  /**< The key that was pressed or released */
  Uint16 raw;         /**< The platform dependent scancode for this event */
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
