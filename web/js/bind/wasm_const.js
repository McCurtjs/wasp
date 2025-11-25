
export const
  seek = {
    set: 0,
    cur: 1,
    end: 2,
  },

  types = {
    none:       0,
    bytes:      1,
    file:       2,
    image:      3,
    shader:     10,
    sprog:      11,
    vao:        12,
    buffer:     13,
    uniform:    14,
    texture:    15,
    framebuf:   16,
    renderbuf:  17,
  },

  sdl = {
    // events
    window_resize: 0x206,
    key_down: 0x300,
    key_up: 0x301,
    mouse_motion: 0x400,
    mouse_button_down: 0x401,
    mouse_button_up: 0x402,
    mouse_wheel: 0x403,

    // other
    keymod: {
      none:   0x0000,
      lshift: 0x0001,
      rshift: 0x0002,
      lctrl:  0x0040,
      rctrl:  0x0080,
      lalt:   0x0100,
      ralt:   0x0200,
    }
  }
;
