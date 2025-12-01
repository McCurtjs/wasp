
import { sdl } from "./bind/wasm_const.js";
import { wasm_import_base } from "./bind/wasm_base.js";
import { wasm_import_gl } from "./bind/wasm_gl.js";
import { wasm_import_stdio } from "./bind/wasm_stdio.js";
import { wasm_import_image } from "./bind/wasm_image.js";

class Game {

  constructor() {
    this.handle = 0;
    this.wasm = null; // wasm instance
    this.gl = null;
    this.canvas = null; // html canvas element (not gl.canvas!)
    this.data_id = 0;
    this.data = {};
    this.initialized = false;
    this.shader_vertex = null;
    this.shader_fragment = null;
    this.await_count = 0;
    this.ready = false;
    this.frame_time = 0;
    this.tests_only = false;

    this.utf8 = {
      decoder: new TextDecoder("utf-8"),
      encoder: new TextEncoder("utf-8"),
    };
  }

  memory(index, size) {
    if (!this.initialized) return null;
    let array = new Uint8Array(this.wasm.exports.memory.buffer);
    return array.subarray(index, index + size);
  }

  memory_f(index, count) {
    if (!this.initialized) return null;
    let array = new Float32Array(this.wasm.exports.memory.buffer);
    return array.subarray(index/4, index/4 + count);
  }

  memory_i(index, count) {
    if (!this.initialized) return;
    let array = new Uint32Array(this.wasm.exports.memory.buffer);
    return array.subarray(index/4, index/4 + count);
  }

  clone_memory(index, size) {
    if (!this.initialized) return null;
    return this.wasm.exports.memory.buffer.slice(index, size);
  }

  str(index, size) {
    let arr = this.memory(index, size);
    return this.utf8.decoder.decode(arr);
  }

  store(data) {
    this.data[++this.data_id] = data;
    return this.data_id;
  }

  free(data_id) {
    delete this.data[data_id];
  }

  async initialize(wasm_filename) {
    await this.initialize_wasm(wasm_filename);
    if (this.tests_only) return;
    this.initialize_webgl();
  }

  async initialize_wasm(wasm_filename) {
    const response = await fetch(wasm_filename);
    let imports = {};

    wasm_import_base(imports, this);
    wasm_import_stdio(imports, this);
    wasm_import_gl(imports, this);
    wasm_import_image(imports, this);

    await WebAssembly.instantiateStreaming(response, { env: imports })
    .then((obj) => {
      this.wasm = obj.instance;
      if (!this.wasm) return;
      this.initialized = true;
      this.tests_only = this.wasm.exports.canary(2) == 1 ? true : false;
    });
  }

  initialize_webgl() {
    const canvas = document.getElementById("game");
    canvas.width = canvas.parentElement.offsetWidth;
    canvas.height = canvas.parentElement.offsetHeight;
    let gl = canvas.getContext("webgl2");

    if (!gl) {
      alert("Unable to initialize WebGL :(");
    }

    gl.clearColor(0.05, 0.15, 0.25, 1);
    gl.clearDepth(1);
    gl.enable(gl.DEPTH_TEST);
    gl.enable(gl.CULL_FACE);
    gl.depthFunc(gl.LEQUAL);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    this.gl = gl;
    this.canvas = canvas;
  }

  initialize_window_events() {
    let game = this;

    let canvas_resize = () => {
      let bounds = game.gl.canvas.getBoundingClientRect();
      let pr = window.devicePixelRatio;
      let w = (bounds.width * pr) | 0;
      let h = (bounds.height * pr) | 0;
      game.gl.canvas.width = w;
      game.gl.canvas.height = h;
      game.wasm.exports.wasm_push_window_event(sdl.window_resize, w, h);
    }

    canvas_resize();
    window.addEventListener("ready", canvas_resize);
    window.addEventListener("resize", canvas_resize);

    game.gl.canvas.addEventListener('contextmenu', (e) => {
      e.preventDefault(); e.stopPropagation();
    });

    let keyboard_event = (e, event_type) => {
      let k = e.keyCode;
      let mod = 0;
      mod |= e.shiftKey ? sdl.keymod.lshift : 0;
      mod |= e.ctrlKey ? sdl.keymod.lctrl : 0;
      mod |= e.altKey ? sdl.keymod.lalt : 0;
      game.wasm.exports.wasm_push_keyboard_event(event_type, k, mod, e.repeat);
    }

    window.addEventListener('keydown', (e) => {
      keyboard_event(e, sdl.key_down);
    });

    window.addEventListener('keyup', (e) => {
      keyboard_event(e, sdl.key_up);
    });

    game.gl.canvas.addEventListener('mousedown', (e) => {
      game.wasm.exports.wasm_push_mouse_button_event(
        sdl.mouse_button_down, e.button + 1, e.offsetX, e.offsetY);
    });

    // attach to window so we still get "up" events when dragged out the window
    window.addEventListener('mouseup', (e) => {
      let b = game.gl.canvas.getBoundingClientRect();
      let pos = { x: e.clientX - b.x, y: e.clientY - b.y };
      game.wasm.exports.wasm_push_mouse_button_event(
        sdl.mouse_button_up, e.button + 1, pos.x, pos.y);
    });

    window.addEventListener('mousemove', (e) => {
      let b = game.gl.canvas.getBoundingClientRect();
      let pos = { x: e.clientX - b.x, y: e.clientY - b.y };
      game.wasm.exports.wasm_push_mouse_motion_event(
        pos.x, pos.y, e.movementX, e.movementY);
    });
  }
};

export { Game };
