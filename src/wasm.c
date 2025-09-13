#include "wasm.h"

#include "str.h"

#define FN_SIGNATURE_PRINT void print(const char* str)
#define FN_SIGNATURE_PRINT_STR void print_str(slice_t str)
#define FN_SIGNATURE_PRINT_STR_C void print_str_color(slice_t str, ConsoleColor c)
#define FN_SIGNATURE_PRINT_INT void print_int(long long int i)
#define FN_SIGNATURE_PRINT_PTR void print_ptr(const void* p)
#define FN_SIGNATURE_PRINT_FLOAT void print_float(float f)
#define FN_SIGNATURE_PRINT_FLOATS void print_floats(const float* f, uint count)
#define FN_SIGNATURE_ALERT void alert(const char* str)

#ifdef __WASM__
extern void js_log(const char* str, unsigned len, int color);
extern void js_log_int(long long int i);
extern void js_log_num(float f);
extern void js_log_num_array(const float* ptr, uint count);
extern void js_alert(const char* str, unsigned len);

#include <string.h> // strlen

// this is an annoying way to do it, but avoids having to keep signatures sync'd
// should all be replaced once proper string handling is in with structured logs
FN_SIGNATURE_PRINT {
  js_log(str, strlen(str), -1);
}

FN_SIGNATURE_PRINT_STR {
  js_log(str.begin, str.size, -1);
}

FN_SIGNATURE_PRINT_STR_C {
  js_log(str.begin, str.size, c);
}

FN_SIGNATURE_PRINT_INT {
  js_log_int(i);
}

FN_SIGNATURE_PRINT_PTR {
  js_log_int((size_t)p);
}

FN_SIGNATURE_PRINT_FLOAT {
  js_log_num(f);
}

FN_SIGNATURE_PRINT_FLOATS {
  js_log_num_array(f, count);
}

FN_SIGNATURE_ALERT {
  js_alert(str, strlen(str));
}

#else
#include <stdio.h> // printf, fprintf
#include "str.h"

FN_SIGNATURE_PRINT {
  printf("%s\n", str);
}

FN_SIGNATURE_PRINT_STR {
  printf("%.*s\n", (int)str.size, str.begin);
}

FN_SIGNATURE_PRINT_STR_C {
  Array_slice arr = str_split(str, "%c");
  if (arr->size != 2) {
    printf("%.*s\n", (int)str.size, str.begin);
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

FN_SIGNATURE_PRINT_INT {
  printf("%lld\n", i);
}

FN_SIGNATURE_PRINT_PTR {
  printf("%p\n", p);
}

FN_SIGNATURE_PRINT_FLOAT {
  printf("%f\n", f);
}

FN_SIGNATURE_PRINT_FLOATS {
  printf("[");
  for (uint i = 0; i < count; ++i) {
    printf("%f", f[i]);
    if (i != count -1) printf(",");
  }
  printf("]\n");
}

FN_SIGNATURE_ALERT {
  fprintf(stderr, "%s", str);
}
#endif
