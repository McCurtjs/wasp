#include "image.h"

#ifdef __WASM__
#include <string.h>

extern void* js_image_open(const char* src, uint src_len);
extern void  js_image_open_async(void* data_id);
extern uint  js_image_width(void* data_id);
extern uint  js_image_height(void* data_id);
extern void  js_image_delete(void* data_id);
#else
#define STB_IMAGE_IMPLEMENTATION
#include "str.h"
#include "stb_image.h"
#endif

void image_open_async(Image* image, const char* filename) {
  #ifdef __WASM__
  image->handle = js_image_open(filename, strlen(filename));
  image->ready = 0;
  js_image_open_async(image->handle);
  #else

  int w, h, n;
  byte* data = stbi_load(filename, &w, &h, &n, 4);

  if (!data) {
    image->ready = 0;
    str_log("Error loading image: {}, file: {}", stbi_failure_reason(), filename);
    return;
  }

  image->handle = data;
  image->width = w;
  image->height = h;
  image->ready = 1;
  #endif
}

uint image_width(const Image* image) {
  #ifdef __WASM__
  return js_image_width(image->handle);
  #else
  return image->width;
  #endif
}

uint image_height(const Image* image) {
  #ifdef __WASM__
  return js_image_height(image->handle);
  #else
  return image->height;
  #endif
}

void image_delete(Image* image) {
  #ifdef __WASM__
  js_image_delete(image->handle);
  #else
  if (image->ready) {
    image->ready = 0;
    stbi_image_free(image->handle);
  }
  #endif
}
