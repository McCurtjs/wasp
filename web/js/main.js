import { Game } from "./game.js";

var game = null;

function render_timer(now) {
  const { gl } = game;
  const { wasp_loading_manager, wasp_update, wasp_render } = game.wasm.exports;
  const ms = game.frame_time > 0 ? (now - game.frame_time) : 16.6; // milliseconds per frame
  const dt = ms / 1000; // seconds per frame
  const fps = 1000 / ms; // frames per second
  game.frame_time = now;

  document.getElementById('console').textContent = `
    WASM GL Test - FPS: ${Math.floor(fps)}
  `;

  wasp_loading_manager();

  wasp_update(game.handle, dt);
  wasp_render(game.handle);

  let err = gl.getError();
  if (err != gl.NO_ERROR) {
    console.log("[Main.WebGL] Error: 0x" + err.toString(16));
  }

  requestAnimationFrame(render_timer);
}

window.onload = async() => {
  game = new Game();

  await game.initialize('game.wasm');

  if (game.tests_only) {
    game.wasm.exports.wasm_tests();
  } else {
    let canvas = game.gl.canvas;
    game.handle = game.wasm.exports.game_init(canvas.width, canvas.height);

    if (!game.wasm.exports.wasp_load(game.handle)) {
      console.log("[Main.onload] Error: Failed on initial loading");
      return;
    }

    game.initialize_window_events();
    requestAnimationFrame(render_timer);
  }
}
