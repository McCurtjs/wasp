/*******************************************************************************
* MIT License
*
* Copyright (c) 2026 Curtis McCoy
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

#define MCLIB_INTERNAL_IMPL
#include "image.h"
#include "str.h"

////////////////////////////////////////////////////////////////////////////////
// Data section for setup and loading of static hardcoded image data
////////////////////////////////////////////////////////////////////////////////

#ifdef __WASM__
# define IMG_SET_HANDLE(IMG, DATA_SIZE) \
    IMG->handle = js_buffer_create(IMG->data, DATA_SIZE)
#else
# define IMG_SET_HANDLE(IMG, DATA_SIZE) IMG->handle = IMG->data
#endif

static Image _img_load_default_helper(Image image) {
  if (!image->ready) {
    image->ready = true;

#ifdef __WASM__
    image->handle = js_buffer_create(image->data, image->channels);
#else
    image->handle = image->data;
#endif
  }
  return image;
}

////////////////////////////////////////////////////////////////////////////////
// Default "error" texture checkerboard pattern
////////////////////////////////////////////////////////////////////////////////

#define F 255
struct image_t _img_default_error = {
  .type = IMG_ERROR,
  .filename = (String)&slice_empty,
  .handle = NULL,
  .data = (byte[]) {
    0,0,0, F,0,F, 0,0,0, F,0,F,
    F,0,F, 0,0,0, F,0,F, 0,0,0,
    0,0,0, F,0,F, 0,0,0, F,0,F,
    F,0,F, 0,0,0, F,0,F, 0,0,0
  },
  .width = 4,
  .height = 4,
  .channels = 3,
};

////////////////////////////////////////////////////////////////////////////////

static Image _img_load_default_error(void) {
  return _img_load_default_helper(&_img_default_error);
}

////////////////////////////////////////////////////////////////////////////////
// Default one-pixel white image
////////////////////////////////////////////////////////////////////////////////

struct image_t _img_default_white = {
  .type = IMG_DEFAULT,
  .filename = (String)&slice_empty,
  .handle = NULL,
  .data = (byte[]) { F, F, F },
  .width = 1,
  .height = 1,
  .channels = 3,
};

////////////////////////////////////////////////////////////////////////////////

Image img_load_default_white(void) {
  return _img_load_default_helper(&_img_default_white);
}

////////////////////////////////////////////////////////////////////////////////
// Default one-pixel image representing a flat normal map
////////////////////////////////////////////////////////////////////////////////

struct image_t _img_default_normal = {
  .type = IMG_DEFAULT,
  .filename = (String)&slice_empty,
  .handle = NULL,
  .data = (byte[]) { 127, 127, 255 },
  .width = 1,
  .height = 1,
  .channels = 3,
};

////////////////////////////////////////////////////////////////////////////////

Image img_load_default_normal(void) {
  return _img_load_default_helper(&_img_default_normal);
}

////////////////////////////////////////////////////////////////////////////////

#ifndef __WASM__
# define STB_IMAGE_IMPLEMENTATION
# include "stb_image.h"
#else
# include <string.h>
# include <stdlib.h> // malloc, free
# include "wasm.h"

extern void* js_image_open(Image img, const char* src, uint src_len);
extern void  js_image_delete(void* data_id);
extern void  js_image_extract(void* dst, void* src, int dst_channels);

extern void* js_buffer_create(const byte* bytes, uint size);
extern void  js_buffer_delete(void* data_id);

////////////////////////////////////////////////////////////////////////////////
// Callback to finalize asynchronous loading in WASM
////////////////////////////////////////////////////////////////////////////////

void export(img_open_async_done) (
  Image img, int width, int height, bool success
) {
  assert(img);
  assert(image->type == IMG_HANDLE);

  if (success) {
    str_log("  Loaded ({}): {} x {}", (size_t)img->handle, width, height);
    img->width = width;
    img->height = height;
    img->channels = 4;
    img->ready = true;
  }
  else {
    js_image_delete(img->handle);
    *img = _img_load_default_error();
    str_log("  Failed ({}): using default", (size_t)img->handle);
  }
}

#endif

////////////////////////////////////////////////////////////////////////////////
// Loads an image from a filename, asynchronously in WASM mode
////////////////////////////////////////////////////////////////////////////////

Image img_load_async_str(String filename) {
  str_log("[Image.load] Loading: {}", filename);
  Image ret;

#ifdef __WASM__
  ret = malloc(sizeof(*ret));
  assert(ret);

  *ret = (struct image_t) {
    .filename = filename,
    .type = IMG_HANDLE,
    .ready = false,
    .blend = true,
  };

  ret->handle = js_image_open(ret, ret->filename->begin, ret->filename->length);
  str_log("  Async ID: {}", (size_t)ret->handle);
  return ret;
#else
  struct image_t img = {
    .filename = filename,
    .type = IMG_HANDLE,
    .ready = true,
    .blend = true,
  };

  img.handle = stbi_load(
    img.filename->begin, &img.width, &img.height, &img.channels, 0
  );

  if (!img.handle) {
    str_delete(&img.filename);
    str_log("  Failed: {} - using default", stbi_failure_reason());
    return _img_load_default_error();
  }

  ret = malloc(sizeof(*ret));
  assert(ret);
  *ret = img;
  str_log("  Loaded: {} x {} ({})", ret->width, ret->height, ret->channels);
  return ret;
#endif
}

////////////////////////////////////////////////////////////////////////////////

Image img_load_async(slice_t filename) {
  return img_load_async_str(istr_copy(filename));
}

////////////////////////////////////////////////////////////////////////////////
// Initialize an image from an array of bytes
////////////////////////////////////////////////////////////////////////////////

Image img_from_bytes(const byte* source, vec2i size, int channels) {
  assert(source);
  assert(channels > 0 && channels <= 4);
  assert(size.w > 0 && size.h > 0);

  index_t count = size.w * size.h;
  index_t data_size = count * channels;

  Image ret = malloc(sizeof(*ret));
  assert(ret);

  void* data = malloc(data_size);
  assert(data);

  *ret = (struct image_t) {
    .filename = str_empty,
    .data = data,
    .size = size,
    .channels = channels,
    .type = IMG_DATA,
    .blend = true,
  };

  memcpy(ret->data, source, data_size);

  IMG_SET_HANDLE(ret, data_size);

  ret->ready = true;
  return ret;
}

////////////////////////////////////////////////////////////////////////////////

Image img_from_color(color4 colorf, vec2i size, int channels) {
  assert(channels > 0 && channels <= 4);
  assert(size.w > 0 && size.h > 0);

  vec4b color = v4cv(colorf);

  index_t count = size.w * size.h;
  index_t data_size = count * channels;

  Image ret = malloc(sizeof(*ret));
  assert(ret);

  void* data = malloc(data_size);
  assert(data);

  *ret = (struct image_t){
    .filename = str_empty,
    .handle = data,
    .data = data,
    .size = size,
    .channels = channels,
    .type = IMG_DATA,
    .blend = true,
  };

  switch (channels) {

    case 4: {
      vec4b* mem = data;
      for (index_t i = 0; i < count; ++i, ++mem) {
        *mem = color;
      }
    } break;

    case 3: {
      vec3b* mem = data;
      for (index_t i = 0; i < count; ++i, ++mem) {
        *mem = color.rgb;
      }
    } break;

    case 1: {
      // use the color's luminosity for
      byte lum = (byte)c4lum(colorf);
      memset(data, lum, data_size);
    } break;

    default: {
      assert(false);
    } break;

  }

  IMG_SET_HANDLE(ret, data_size);

  ret->ready = true;
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Deletes an images memory
////////////////////////////////////////////////////////////////////////////////

void _img_free(Image img) {
#ifdef __WASM__
  switch (img->type) {

    case IMG_DEFAULT:
      break;

    case IMG_ERROR:
      break;

    case IMG_HANDLE:
      js_image_delete(img->handle);
      break;

    case IMG_DATA:
      js_buffer_delete(img->handle);
      free(img->data);
      break;
  }
#else
  switch (img->type) {

    case IMG_DEFAULT:
      break;

    case IMG_ERROR:
      break;

    case IMG_HANDLE:
      stbi_image_free(img->handle);
      break;

    case IMG_DATA:
      free(img->data);
      break;
  }
#endif

  img->handle = NULL;
  img->data = NULL;
}

////////////////////////////////////////////////////////////////////////////////

void img_delete(Image* pimg) {
  if (!pimg || !*pimg) return;
  Image img = *pimg;
  str_delete(&img->filename);

  _img_free(img);
  if (img->type != IMG_DEFAULT && img->type != IMG_ERROR) {
    free(img);
  }

  *pimg = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Image data manipulation functions to release handles and change data format
////////////////////////////////////////////////////////////////////////////////

void img_resolve(Image* pimg) {
  assert(pimg && *pimg);
  Image img = *pimg;
  assert(img->ready);
  assert(img->handle);

  switch (img->type) {

    case IMG_DATA:
      break;

    case IMG_ERROR:
    case IMG_DEFAULT:
      *pimg = img_from_bytes(img->data, img->size, img->channels);

    case IMG_HANDLE: {
#ifdef __WASM__
      assert(!img->data);
      size_t size_bytes = img->size.x * img->size.y * img->channels;
      img->data = malloc(size_bytes);
      js_image_extract(img->data, img->handle, img->channels);
      js_image_delete(img->handle);
      IMG_SET_HANDLE(img, size_bytes);
      img->type = IMG_DATA;
#else
      img->data = img->handle;
#endif
    } break;

  }
}

////////////////////////////////////////////////////////////////////////////////

void img_set_layout(Image* pimg, vec2i size, int channels) {
  UNUSED(pimg);
  UNUSED(size);
  UNUSED(channels);
  // TODO
}

////////////////////////////////////////////////////////////////////////////////

void img_set_size(Image* pimg, vec2i size) {
  UNUSED(pimg);
  UNUSED(size);
  // TODO
}

////////////////////////////////////////////////////////////////////////////////

void img_set_channels(Image* pimg, int channels) {
  assert(pimg && *pimg);
  Image img = *pimg;
  assert(img->ready);
  assert(channels > 0 && channels <= 4);
  if (img->channels == channels) return;
  img_resolve(&img);

  size_t size_bytes = img->width * img->height * channels;
  byte* dst_data = malloc(size_bytes);
  assert(dst_data);

  for (size_t s = 0, d = 0; d < size_bytes; s += img->channels, d += channels) {
    color4b pixel;
    byte* src = (byte*)img->data + s;
    switch (img->channels) {
      case 4: pixel = *(color4b*)src;             break;
      case 3: pixel = v34b(*(color3b*)src, 255);  break;
      case 1: pixel = v4b(*src, *src, *src, 255); break;
      default: assert(false); pixel = b4black;    break;
    }

    byte* dst = (byte*)dst_data + d;
    switch (channels) {
      case 4: *(color4b*)dst = pixel;     break;
      case 3: *(color3b*)dst = pixel.rgb; break;
      case 1: *dst = b4lum(pixel);        break;
      default: assert(false);             break;
    }
  }

  _img_free(img);
  img->type = IMG_DATA;
  img->data = dst_data;
  IMG_SET_HANDLE(img, data_size);
}

////////////////////////////////////////////////////////////////////////////////
// Rearranges an image from a 2d grid of tiles into a vertical stack
////////////////////////////////////////////////////////////////////////////////

void img_repack_vertical(Image* pimg, vec2i dim) {
  assert(pimg && *pimg);
  Image img = *pimg;
  assert(img->ready);
  assert(img->handle);
  assert(dim.x > 0 && dim.y > 0);
  if (dim.x == 1) return;
  img_resolve(&img);

  size_t size_bytes = img->width * img->height * img->channels;
  byte* data = malloc(size_bytes);
  assert(data);

  vec2i tile = i2div(img->size, dim);
  int cell_width = img->channels * tile.w;
  int cell_size = cell_width * tile.h;
  int full_width = cell_width * dim.w;

  byte* iter_target = data;
  for (int dy = 0; dy < dim.y; ++dy) {
    for (int dx = 0; dx < dim.x; ++dx) {
      const byte* source = (byte*)img->data
        + dx * cell_width
        + dy * full_width * tile.h;

      for (int y = 0; y < tile.h; ++y) {
        memcpy(iter_target, source, cell_width);
        source += full_width;
        iter_target += cell_width;
      }
    }
  }

  _img_free(img);
  img->type = IMG_DATA;
  img->data = data;
  IMG_SET_HANDLE(img, size_bytes);
}
