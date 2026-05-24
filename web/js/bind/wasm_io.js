
import { seek, types } from "./wasm_const.js";

function wasm_import_stdio(imports, game) {

  imports['js_file_create'] =  (file_ptr, path, path_len) => {
    return game.store({
      type: types.file,
      ready: false,
      path: game.str(path, path_len),
      buffer: null,
      wasm_ptr: file_ptr
    });
  }

  imports['js_file_open_async'] = async (data_id) => {
    let data = game.data[data_id];
    if (!data || data.type != types.file) return;
    ++game.await_count;

    let res = await fetch(data.path);

    data.buffer = await res.arrayBuffer();

    console.log(`  Loaded File (${data_id}): ${data.buffer.byteLength} bytes`);

    data.ready = true;
    --game.await_count;

    game.wasm.exports.file_open_async_done(data.wasm_ptr, data.buffer.byteLength);
  }

  imports['js_file_read'] = (data_id, ptr, size) => {
    let data = game.data[data_id];
    if (!data || data.type != types.file || !data.ready) return false;
    let src = new Uint8Array(data.buffer);
    let dst = game.memory(ptr, size);
    for (let i = 0; i < size; ++i) {
      dst[i] = src[i];
    }
    return true;
  }

  imports['js_file_close'] = (data_id) => {
    let data = game.data[data_id];
    if (!data || data.type != types.file) return;
    if (data.ready == true) delete data.buffer;
    game.free(data_id);
  }

}

export { wasm_import_stdio };
