
import { types } from "./wasm_const.js";

function wasm_import_image(imports, game) {

  imports['js_image_open'] =  (path, path_len) => {
    return game.store({
      type: types.image,
      ready: false,
      path: game.str(path, path_len),
      image: null,
    });
  }

  imports['js_image_open_async'] = (data_id) => {
    let data = game.data[data_id];
    if (!data || data.type != types.image || data.ready) return;
    ++game.await_count;

    data.image = new Image();
    data.image.onload = () => {
      data.ready = true;
      --game.await_count;
    };
    data.image.src = data.path;
  }

  imports['js_image_width'] = (data_id) => {
    let data = game.data[data_id];
    if (!data || data.type != types.image || !data.ready) return 0;
    return data.image.width;
  }

  imports['js_image_height'] = (data_id) => {
    let data = game.data[data_id];
    if (!data || data.type != types.image || !data.ready) return 0;
    return data.image.height;
  }

  imports['js_image_delete'] = (data_id) => {
    let data = game.data[data_id];
    if (!data || data.type != types.image) return;
    game.free(data_id);
  }
}

export { wasm_import_image };
