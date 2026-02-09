/*******************************************************************************
* MIT License
*
* Copyright (c) 2025 Curtis McCoy
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include "image.h"
#include "str.h"

typedef enum img_load_t {
  IMG_DEFAULT,
  IMG_FROM_FILE,
  IMG_DATA
} img_load_t;

typedef struct Image_Internal {
  // public (read-only)
  String filename;
  void* data;
  int width;
  int height;
  int channels;
  bool ready;
  bool blend;

  // private
  img_load_t type : 8;
} Image_Internal;

#define IMAGE_INTERNAL \
  Image_Internal* img = (Image_Internal*)(img_in); \
  assert(img)

////////////////////////////////////////////////////////////////////////////////
// Data section for default built-in image values
////////////////////////////////////////////////////////////////////////////////

#define F 255
Image_Internal img_default = {
  .filename = (String)&slice_empty,
  .data = (byte[]) {
    0,0,0, F,0,F, 0,0,0, F,0,F,
    F,0,F, 0,0,0, F,0,F, 0,0,0,
    0,0,0, F,0,F, 0,0,0, F,0,F,
    F,0,F, 0,0,0, F,0,F, 0,0,0
  },
  .width = 4,
  .height = 4,
  .channels = 3,
  .ready = true,
  .blend = false,
  .type = IMG_DEFAULT,
};

////////////////////////////////////////////////////////////////////////////////

Image_Internal img_default_white = {
  .filename = (String)&slice_empty,
  .data = (byte[]) { F, F, F },
  .width = 1,
  .height = 1,
  .channels = 3,
#ifdef __WASM__
  .ready = false,
#else
  .ready = true,
#endif
  .blend = false,
  .type = IMG_DEFAULT
};

////////////////////////////////////////////////////////////////////////////////

Image_Internal img_default_normal = {
  .filename = (String)&slice_empty,
  .data = (byte[]) { 127, 127, 255 },
  .width = 1,
  .height = 1,
  .channels = 3,
#ifdef __WASM__
  .ready = false,
#else
  .ready = true,
#endif
  .blend = false,
  .type = IMG_DEFAULT
};

#ifdef __WASM__
# include <string.h>
# include <stdlib.h> // malloc, free
# include "wasm.h"

extern void* js_image_open(Image_Internal* img, const char* src, uint src_len);
extern void  js_image_delete(void* data_id);
extern void  js_image_extract(void* dst, void* src, int dst_channels);

extern void* js_buffer_create(const byte* bytes, uint size);
extern void  js_buffer_delete(void* data_id);

////////////////////////////////////////////////////////////////////////////////
// Callback to finalize asynchronous loading in WASM
////////////////////////////////////////////////////////////////////////////////

void export(img_open_async_done) (
  Image img_in, int width, int height, bool success
) {
  IMAGE_INTERNAL;
  if (success) {
    img->width = width;
    img->height = height;
    str_log("  Loaded ({}): {} x {}", (size_t)img->data, width, height);
  }
  else {
    *img = img_default;
    int buffer_size = img->width * img->height * img->channels;
    img->data = js_buffer_create(img_default.data, buffer_size);
    img->type = IMG_DATA;
    str_log("  Failed ({}): using default", (size_t)img->data);
  }

  img->channels = 4;
  img->ready = true;
}

#else
# define STB_IMAGE_IMPLEMENTATION
# include "stb_image.h"
#endif

////////////////////////////////////////////////////////////////////////////////
// Loads an image from a filename, asynchronously in WASM mode
////////////////////////////////////////////////////////////////////////////////

Image img_load_async_str(String filename) {
  str_log("[Image.load] Loading: {}", filename);
  Image_Internal* ret;

#ifdef __WASM__
  ret = malloc(sizeof(Image_Internal));
  assert(ret);

  *ret = (Image_Internal) {
    .filename = filename,
    .type = IMG_FROM_FILE,
    .blend = true,
  };

  ret->data = js_image_open(ret, ret->filename->begin, ret->filename->length);
  str_log("  Async ID: {}", (size_t)ret->data);
#else
  Image_Internal img = {
    .filename = filename,
    .type = IMG_FROM_FILE,
    .ready = true,
    .blend = true,
  };

  img.data = stbi_load(
    img.filename->begin, &img.width, &img.height, &img.channels, 0
  );

  if (!img.data) {
    str_delete(&img.filename);
    ret = &img_default;
    str_log("  Failed: {} - using default", stbi_failure_reason());
  }
  else {
    ret = malloc(sizeof(Image_Internal));
    assert(ret);
    *ret = img;
    str_log("  Loaded: {} x {} ({})", ret->width, ret->height, ret->channels);
  }
#endif

  return (Image)ret;
}

////////////////////////////////////////////////////////////////////////////////

Image img_load_async(slice_t filename) {
  return img_load_async_str(istr_copy(filename));
}

////////////////////////////////////////////////////////////////////////////////
// Gets a default one-pixel white image
////////////////////////////////////////////////////////////////////////////////

Image img_load_default_white(void) {
#ifdef __WASM__
  if (!img_default_white.ready) {
    img_default_white.ready = true;
    img_default_white.data = js_buffer_create(
      img_default_white.data, img_default_white.channels
    );
  }
#endif
  return (Image)&img_default_white;
}

////////////////////////////////////////////////////////////////////////////////
// Gets a default one-pixel image representing a flat normal map
////////////////////////////////////////////////////////////////////////////////

Image img_load_default_normal(void) {
#ifdef __WASM__
  if (!img_default_normal.ready) {
    img_default_normal.ready = true;
    img_default_normal.data = js_buffer_create(
      img_default_normal.data, img_default_normal.channels
    );
  }
#endif
  return (Image)&img_default_normal;
}

////////////////////////////////////////////////////////////////////////////////
// Initialize an image from an array of bytes
////////////////////////////////////////////////////////////////////////////////

Image img_from_bytes(const byte* data, int width, int height, int channels) {
  assert(data);

  index_t data_size = width * height * channels;

  Image_Internal* ret = malloc(sizeof(Image_Internal) + data_size);
  assert(ret);

  *ret = (Image_Internal) {
    .filename = str_empty,
    .data = (void*)(ret + 1),
    .width = width,
    .height = height,
    .type = IMG_DATA,
    .blend = true,
  };

  memcpy(ret->data, data, data_size);

  #ifdef __WASM__
  ret->data = js_buffer_create(ret->data, data_size);
  #endif

  ret->ready = true;
  return (Image)ret;
}

////////////////////////////////////////////////////////////////////////////////
// Deletes an images memory
////////////////////////////////////////////////////////////////////////////////

void img_delete(Image* pimg) {
  if (!pimg || !*pimg) return;
  Image_Internal* img = (Image_Internal*)*pimg;
  str_delete(&img->filename);

#ifdef __WASM__
  switch (img->type) {
    case IMG_FROM_FILE:
      js_image_delete(img->data);
      break;

    case IMG_DATA:
      js_buffer_delete(img->data);
      break;

    case IMG_DEFAULT:
      assert(false);
      break;
  }
  free(img);
#else
  switch (img->type) {
    case IMG_FROM_FILE:
      stbi_image_free(img->data);
      free(img);
      break;

    case IMG_DATA:
      free(img);
      break;

    case IMG_DEFAULT:
      break;
  }
#endif

  *pimg = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Copies the image data to an array with the designated number of channels
////////////////////////////////////////////////////////////////////////////////

void img_copy_data(void* dst, Image img_in, int channels) {
  IMAGE_INTERNAL;
  assert(img);
  assert(dst);
  assert(img->ready);
  assert(img->data);
  assert(channels > 0 && channels <= 4);
  assert(channels <= img->channels);

#ifdef __WASM__
  js_image_extract(dst, img->data, channels);
#else
  size_t size_bytes = img->width * img->height * img->channels;
  byte* dst_bytes = dst;
  const byte* src_bytes = img->data;

  if (channels == img->channels) {
    memcpy(dst, img->data, size_bytes);
    return;
  }

  size_t img_channels = img->channels;
  for (size_t s = 0, d = 0; s < size_bytes; s += img_channels, d += channels) {
    for (size_t i = 0; i < channels; ++i) {
      dst_bytes[d + i] = src_bytes[s + i];
    }
  }
#endif
}
