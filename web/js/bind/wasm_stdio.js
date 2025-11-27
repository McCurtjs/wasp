
import { seek, types } from "./wasm_const.js";

function wasm_import_stdio(imports, game) {

  imports['js_fopen'] =  (path, path_len) => {
    return game.store({
      type: types.file,
      ready: false,
      path: game.str(path, path_len),
      pos: 0,
      buffer: null,
    });
  }

  imports['js_fopen_async'] = async (data_id) => {
    let data = game.data[data_id];
    if (!data || data.type != types.file) return;
    ++game.await_count;

    let res = await fetch(data.path);

    data.buffer = await res.arrayBuffer();
    console.log("  Loaded (" + data_id + "): " + data.buffer.byteLength + " bytes");
    data.ready = true;
    --game.await_count;
  }

  imports['js_fseek'] = (data_id, offset, origin) => {
    let data = game.data[data_id];
    if (!data || data.type != types.file || !data.ready) return 1;
    if (origin == seek.end) data.pos = data.buffer.byteLength;
    else if (origin == seek.set) data.pos = offset;
    else if (origin == seek.cur) data.pos += offset;
    return 0;
  }

  imports['js_ftell'] = (data_id) => {
    let data = game.data[data_id];
    if (!data || data.type != types.file || !data.ready) return -1;
    return data.pos;
  }

  imports['js_fread'] = (data_id, ptr, size, max_count) => {
    let data = game.data[data_id];
    if (!data || data.type != types.file || !data.ready) return -1;
    let src = new Uint8Array(data.buffer);
    let dst = game.memory(ptr + data.pos, size * max_count);
    let max_objs = Math.floor(data.buffer.byteLength / size);
    let bytes_count = size * Math.min(max_count, max_objs);
    for (let i = 0; i < bytes_count; ++i) {
      dst[i] = src[i];
    }
    return bytes_count;
  }

  imports['js_fclose'] = (data_id) => {
    let data = game.data[data_id];
    if (!data || data.type != types.file) return;
    game.free(data_id);
  }

}

export { wasm_import_stdio };
