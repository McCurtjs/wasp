import { Game } from "./game.js";

var game = null;


function renderTimer(now) {
  const { await_count, gl } = game;
  const { wasm_load, wasm_update, wasm_render } = game.wasm.exports;
  const ms = now - game.frame_time; // milliseconds per frame
  const dt = ms / 1000; // seconds per frame
  const fps = 1000 / ms; // frames per second
  game.frame_time = now;

  document.getElementById('console').textContent = `
    WASM GL Test - FPS: ${Math.floor(fps)}
  `;

  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  if (!game.ready) {
    if (wasm_load(await_count, dt)) {
      game.ready = true;
    }
  } else {
    wasm_update(dt);
    wasm_render();
  }
  requestAnimationFrame(renderTimer);
}

window.onload = async() => {
  game = new Game();

  await game.initialize('test.wasm');

  if (game.tests_only) {
    game.wasm.exports.wasm_tests();
  } else {
    game.wasm.exports.wasm_preload(game.gl.canvas.width, game.gl.canvas.height);

    requestAnimationFrame(renderTimer);
  }

  console.log('done');
}
