
import { types } from "./wasm_const.js";

function wasm_import_image(imports, game) {

  imports['js_image_open'] =  (image_ptr, path, path_len) => {
    ++game.await_count;

    let data = {
      type: types.image,
      ready: false,
      path: game.str(path, path_len),
      image: new Image(),
      wasm_ptr: image_ptr
    };

    data.image.onload = () => {
      data.ready = true;
      --game.await_count;

      game.wasm.exports.img_open_async_done(
        data.wasm_ptr, data.image.width, data.image.height, true
      );

      //* Test output to display images as they load.
      let img = document.createElement("img");
      img.setAttribute("src", data.path);
      img.setAttribute("height", 50);
      img.classList.add("console");
      document.querySelector("body").appendChild(img);
      //*/
    };

    data.image.onerror = () => {
      --game.await_count;

      game.wasm.exports.img_open_async_done(
        data.wasm_ptr, 0, 0, false
      );

      console.log("Failed to load image");
    };

    data.image.src = data.path;

    return game.store(data);
  }

  imports['js_image_delete'] = (data_id) => {
    let data = game.data[data_id];
    if (!data || data.type != types.image) return;
    game.free(data_id);
  }
}

export { wasm_import_image };
