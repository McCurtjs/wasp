
import { types } from "./wasm_const.js";

function wasm_import_image(imports, game) {

  imports['js_image_open'] =  (image_ptr, path, path_len) => {
    return game.store({
      type: types.image,
      ready: false,
      path: game.str(path, path_len),
      image: null,
      wasm_ptr: image_ptr
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

      game.wasm.exports.image_open_async_done(
        data.wasm_ptr, data.image.width, data.image.height
      );

      /* Test output to display images as they load.
      let img = document.createElement("img");
      img.setAttribute("src", data.path);
      img.setAttribute("height", 50);
      img.classList.add("console");
      document.querySelector("body").appendChild(img);
      //*/
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
