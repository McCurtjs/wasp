#include "wasm.h"

#include "str.h"

#ifdef __WASM__
extern void js_log(const char* str, unsigned len, int color);
extern void js_alert(const char* str, unsigned len);

void wasm_write(slice_t slice) {
  js_log(slice.begin, slice.length, -1);
}

void wasm_write_color(slice_t slice, ConsoleColor c) {
  js_log(slice.begin, slice.length, c);
}

void wasm_alert(slice_t slice) {
  js_alert(slice.begin, slice.length);
}

#else
#include <stdio.h> // printf, fprintf
#include "str.h"

void wasm_write_color(slice_t slice, ConsoleColor c) {
  Array_slice arr = slice_split(slice, S("%c"));
  if (arr->size != 2) {
    printf("%.*s\n", (int)slice.size, slice.begin);
  } else {
    char color[] = "\033[_;__m";
    int is_bold = 0;
    if (c >= 40) {
      c -= 10;
      is_bold = 1;
    }
    sprintf(color, "\033[%01i;%02im", is_bold, c);
    slice_t second = arr_slice_get_back(arr);
    arr_slice_pop_back(arr);
    arr_slice_push_back(arr, S(color));
    arr_slice_push_back(arr, second);
    arr_slice_push_back(arr, S("\033[0m"));
    String result = str_join(str_empty->slice, arr);
    printf("%.*s\n", (int)result->size, result->begin);
    str_delete(&result);
  }
  arr_slice_delete(&arr);
}

void wasm_alert(slice_t slice) {
  uint length = (uint)slice.length;
  fprintf(stderr, "%.*s", length, slice.begin);
}
#endif
