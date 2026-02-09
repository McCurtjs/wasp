
import { types } from "./wasm_const.js";

function wasm_import_image(imports, game) {

  imports['js_image_open'] = (image_ptr, path, path_len) => {
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

  imports['js_image_extract'] = (dst_ptr, src_id, channels) => {
    let data = game.data[src_id];
    if (!data || data.type != types.image) return;

    const width = data.image.width;
    const height = data.image.height;
    const canvas = document.createElement('canvas');
    const context = canvas.getContext('2d');
    canvas.width = width;
    canvas.height = height;
    context.drawImage(data.image, 0, 0);
    const source = context.getImageData(0, 0, width, height);
    let dst = game.memory(dst_ptr, width * height * channels);

    // Doesn't yet support "rgba-float16" values (HDR?) for Float16Array
    if (source.pixelFormat !== "rgba-unorm8") {
      console.log("[Image.js.extract] Unsupported pixel format in canvas");
      return;
    }

    // If the target is RGBA, just do a full copy (prefer this for faster copy)
    if (channels == 4) {
      dst.set(source.data);
      return;
    }

    // If the target format has fewer channels, manual copy to extract the data
    for (let s = 0, d = 0; s < source_data.length; s += 4, d += channels) {
      for (let i = 0; i < channels; ++i) {
        dst[d + i] = source_data[s + i];
      }
    }
  }

  imports['js_image_delete'] = (data_id) => {
    let data = game.data[data_id];
    if (!data || data.type != types.image) return;
    game.free(data_id);
  }
}

export { wasm_import_image };
