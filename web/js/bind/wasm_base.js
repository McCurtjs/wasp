
import { types } from "./wasm_const.js";

function wasm_import_base(imports, game) {

  imports['js_alert'] = (str, len) => {
    window.alert(game.str(str, len));
  }

  imports['js_log'] = (str, len, color) => {
    if (color < 0) {
      console.log(game.str(str, len));
    } else {
      let format = 'color:#' + (color & 0xffffff).toString(16).padStart(6, '0');
      if (color & 0x1000000) {
        format += ';font-weight:bold';
      }
      console.log(game.str(str, len), format);
    }
  }

  // Generic byte buffer handling
  imports['js_buffer_create'] = (ptr, size) => {
    return game.store({
      type: types.bytes,
      buffer: { begin: ptr, size: size },
      get_buffer: () => game.memory(ptr, size),
      size: size,
      ready: true,
    });
  }

  imports['js_buffer_delete'] = (data_id) => {
    let data = game.data[data_id];
    if (!data || data.type != types.bytes) return;
    game.free(data_id);
  }

  imports['js_pointer_lock'] = () => {
    game.canvas.requestPointerLock();
  }

  imports['js_pointer_unlock'] = () => {
    document.exitPointerLock();
  }
}

export { wasm_import_base };
