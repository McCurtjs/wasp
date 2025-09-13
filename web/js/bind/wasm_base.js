
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
}

export { wasm_import_base };
