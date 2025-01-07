
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

  imports['js_log_int'] = (i) => {
    console.log(Number(i));
  }

  imports['js_log_num'] = (i) => {
    console.log(Number(i));
  }

  imports['js_log_num_array'] = (ptr, count) => {
    let arr = [...game.memory_f(ptr, count)];
    arr = arr.map((f) => {
      return parseFloat(f.toFixed(4));
    });
    console.log(arr);
    //console.log([...game.memory_f(ptr, count)]);
  }

  //imports['js_log_lines'] = (ptr, count) => {
  //  let arr = [];
  //}

//  imports['js_log_bytes'] = (ptr, count) => {
//    console.log([...game.memory(ptr, count)]);
//  }
}

export { wasm_import_base };
