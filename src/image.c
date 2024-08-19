#include "image.h"

#ifdef __WASM__
#include <string.h>

extern void* js_image_open(const char* src, uint src_len);
extern void  js_image_open_async(void* data_id);
extern uint  js_image_width(void* data_id);
extern uint  js_image_height(void* data_id);
extern void  js_image_delete(void* data_id);
#endif

void image_open_async(Image* image, const char* filename) {
  #ifdef __WASM__
  image->handle = js_image_open(filename, strlen(filename));
  image->ready = 0;
  js_image_open_async(image->handle);
  #else
  PARAM_UNUSED(filename);
  image->ready = 1;
  #endif
}

uint image_width(const Image* image) {
  #ifdef __WASM__
  return js_image_width(image->handle);
  #else
  PARAM_UNUSED(image);
  return 0;
  #endif
}

uint image_height(const Image* image) {
  #ifdef __WASM__
  return js_image_height(image->handle);
  #else
  PARAM_UNUSED(image);
  return 0;
  #endif
}

void image_delete(Image* image) {
  #ifdef __WASM__
  js_image_delete(image->handle);
  #else
  PARAM_UNUSED(image);
  #endif
}
