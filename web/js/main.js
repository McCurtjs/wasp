import { Game } from "./game.js";

var game = null;

function renderTimer(now) {
  const { await_count, gl } = game;
  const { wasp_load, wasp_update, wasp_render } = game.wasm.exports;
  const ms = now - game.frame_time; // milliseconds per frame
  const dt = ms / 1000; // seconds per frame
  const fps = 1000 / ms; // frames per second
  game.frame_time = now;

  document.getElementById('console').textContent = `
    WASM GL Test - FPS: ${Math.floor(fps)}
  `;

  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT); // shouldn't be needed
  if (!game.ready) {
    if (wasp_load(game.handle, await_count, dt)) {
      game.ready = true;
    }
  } else {
    wasp_update(game.handle, dt);
    wasp_render(game.handle);

    let err = game.gl.getError();
    if (err != game.gl.NO_ERROR) {
      console.log("OpenGL error: ", err);
    }
  }
  requestAnimationFrame(renderTimer);
}

window.onload = async() => {
  game = new Game();

  await game.initialize('game.wasm');

  if (game.tests_only) {
    game.wasm.exports.wasm_tests();
  } else {
    let canvas = game.gl.canvas;
    game.handle = game.wasm.exports.game_init(canvas.width, canvas.height);
    if (game.wasm.exports.wasp_preload(game.handle)) {
      game.initialize_window_events();
      requestAnimationFrame(renderTimer);
    }
  }
}
