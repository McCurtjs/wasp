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

#ifndef __WASM__
# define STB_IMAGE_IMPLEMENTATION
# include "stb_image.h"
# include "SDL3/SDL.h"

static SDL_AtomicInt _img_loading_count = { 0 };
#else
# include <string.h>
# include <stdlib.h> // malloc, free
# include "wasm.h"

static index_t _img_loading_count = 0;

extern void* js_image_open(Image img, const char* src, uint src_len);
extern void  js_image_delete(void* data_id);
extern void  js_image_extract(void* dst, void* src, int dst_channels);

extern void* js_buffer_create(const byte* bytes, uint size);
extern void  js_buffer_delete(void* data_id);
#endif

typedef struct Image_Internal {
  struct _opaque_Image_t pub;

  String filename_internal;
} Image_Internal;

////////////////////////////////////////////////////////////////////////////////

#ifdef __WASM__
# define IMG_SET_HANDLE(IMG, DATA_SIZE) \
    IMG->pub.handle = js_buffer_create((IMG)->pub.data, DATA_SIZE)
#else
# define IMG_SET_HANDLE(IMG, DATA_SIZE) \
    (IMG)->pub.handle = (IMG)->pub.data
#endif

////////////////////////////////////////////////////////////////////////////////
// Data section for setup and loading of static hardcoded image data
////////////////////////////////////////////////////////////////////////////////

static Image_Internal* _img_load_default_helper(Image_Internal* image) {
  if (image->pub.status == S_READY) {
    image->pub.status = S_READY;
    image->pub.filename = image->filename_internal->slice;
    IMG_SET_HANDLE(image, image->channels);
  }
  return image;
}

////////////////////////////////////////////////////////////////////////////////
// Default "error" texture checkerboard pattern
////////////////////////////////////////////////////////////////////////////////

#define F 255
static Image_Internal _img_default_error = {
  .pub = {
    .type = IMG_ERROR,
    .status = S_READY,
    .size = { 4, 4 },
    .channels = 4,
    .blend = false,
    .data = (byte[]) {
      0,0,0,F, F,0,F,F, 0,0,0,F, F,0,F,F,
      F,0,F,F, 0,0,0,F, F,0,F,F, 0,0,0,F,
      0,0,0,F, F,0,F,F, 0,0,0,F, F,0,F,F,
      F,0,F,F, 0,0,0,F, F,0,F,F, 0,0,0,F
    },
  },
  .filename_internal = (String)&slice_empty,
};

////////////////////////////////////////////////////////////////////////////////

static Image_Internal* _img_load_default_error(void) {
  return _img_load_default_helper(&_img_default_error);
}

////////////////////////////////////////////////////////////////////////////////
// Default one-pixel white image
////////////////////////////////////////////////////////////////////////////////

static Image_Internal _img_default_white = {
  .pub = {
    .type = IMG_DEFAULT,
    .status = S_READY,
    .size = { 1, 1 },
    .channels = 4,
    .blend = false,
    .data = (byte[]) { F, F, F, F },
  },
  .filename_internal = (String)&slice_empty,
};

////////////////////////////////////////////////////////////////////////////////

Image img_load_default_white(void) {
  return (Image)_img_load_default_helper(&_img_default_white);
}

////////////////////////////////////////////////////////////////////////////////
// Default one-pixel image representing a flat normal map
////////////////////////////////////////////////////////////////////////////////

static Image_Internal _img_default_normal = {
  .pub = {
    .type = IMG_DEFAULT,
    .status = S_READY,
    .size = { 1, 1 },
    .channels = 4,
    .blend = false,
    .data = (byte[]) { 127, 127, 255, 255 },
  },
  .filename_internal = (String)&slice_empty,
};

////////////////////////////////////////////////////////////////////////////////

Image img_load_default_normal(void) {
  return (Image)_img_load_default_helper(&_img_default_normal);
}

#ifdef __WASM__

////////////////////////////////////////////////////////////////////////////////
// Asynchronously load an image from a file (WASM)
////////////////////////////////////////////////////////////////////////////////

void export(img_open_async_done)(
  Image img, int width, int height, bool success
) {
  assert(img);
  assert(img->type == IMG_HANDLE);
  --_img_loading_count;

  if (success) {
    str_log("[Image.new]   Loaded ({}): {} x {}"
    , (size_t)img->handle, width, height
    );

    img->width = width;
    img->height = height;
    img->channels = 4;
    img->status = S_READY;
  }
  else {
    js_image_delete(img->handle);
    *img = *_img_load_default_error();
    str_log("[Image.new]   Failed ({}): using default", img->handle);
  }
}

////////////////////////////////////////////////////////////////////////////////

Image img_new(String filename) {
  assert(!str_is_null_or_empty(filename));
  Image_Internal* ret = malloc(sizeof(*ret));
  assert(ret);

  *ret = (Image_Internal) {
    .pub = {
      .filename = filename->slice,
      .type = IMG_HANDLE,
      .status = S_LOADING,
      .blend = true,
    },
    .filename_internal = filename,
  };

  ++_img_loading_count;
  ret->handle = js_image_open(ret, filename->begin, filename->length);
  str_log("[Image.new] Loading: {}, Async ID: {}", filename, ret->handle);
  return (Image)ret;
}

#else

////////////////////////////////////////////////////////////////////////////////
// Asynchronously load an image from a file (Native/SDL)
////////////////////////////////////////////////////////////////////////////////

static int SDLCALL _img_load_async(void* data) {
  assert(data);
  Image_Internal* img = data;

  img->pub.handle = stbi_load(img->pub.filename.begin, 
    &img->pub.width, &img->pub.height, &img->pub.channels, 0
  );

  if (img->pub.handle) {
    img->pub.status = S_READY;
    str_log("[Image.load] Loaded: ({} x {}) {}"
    , img->pub.width, img->pub.height, img->pub.filename
    );
  }
  else {
    str_log("[Image.load] Failed to load ({}): {}"
    , img->pub.handle, img->pub.filename
    );
    *img = *_img_load_default_error();
  }

  SDL_AtomicDecRef(&_img_loading_count);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

Image img_new_str(String filename) {
  assert(!str_is_null_or_empty(filename));
  Image_Internal* ret = malloc(sizeof(*ret));
  assert(ret);

  *ret = (Image_Internal) {
    .pub = {
      .filename = filename->slice,
      .type = IMG_HANDLE,
      .status = S_LOADING,
      .blend = true,
    },
    .filename_internal = filename,
  };

  str_log("[Image.new] Loading: {}", filename);
  SDL_Thread* thread = SDL_CreateThread(_img_load_async, filename->begin, ret);

  if (thread) {
    SDL_AtomicIncRef(&_img_loading_count);
    SDL_DetachThread(thread);
  }
  else {
    str_log("[Image.new] Failed to create loading thraed: {}", filename);
    ret->pub.status = S_FAILED;
  }

  return (Image)ret;
}

#endif

////////////////////////////////////////////////////////////////////////////////

Image img_new(slice_t filename) {
  return img_new_str(str_copy(filename));
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

  Image_Internal* ret = malloc(sizeof(*ret));
  assert(ret);

  void* data = malloc(data_size);
  assert(data);

  *ret = (Image_Internal) {
    .pub = {
      .filename = slice_empty,
      .type = IMG_DATA,
      .status = S_READY,
      .channels = channels,
      .size = size,
      .data = data,
      .blend = true,
    },
    .filename_internal = str_empty,
  };

  memcpy(ret->pub.data, source, data_size);

  IMG_SET_HANDLE(ret, data_size);
  return (Image)ret;
}

////////////////////////////////////////////////////////////////////////////////

Image img_from_color(color4 colorf, vec2i size, int channels) {
  assert(channels > 0 && channels <= 4);
  assert(size.w > 0 && size.h > 0);

  vec4b color = v4cv(colorf);

  index_t count = size.w * size.h;
  index_t data_size = count * channels;

  Image_Internal* ret = malloc(sizeof(*ret));
  assert(ret);

  void* data = malloc(data_size);
  assert(data);

  *ret = (Image_Internal) {
    .pub = {
      .filename = slice_empty,
      .type = IMG_DATA,
      .status = S_READY,
      .channels = channels,
      .size = size,
      .data = data,
      .blend = true,
    },
    .filename_internal = str_empty,
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
      // use the color's luminosity for a grayscale image
      byte lum = (byte)c4lum(colorf);
      memset(data, lum, data_size);
    } break;

    default: {
      assert(false);
    } break;

  }

  IMG_SET_HANDLE(ret, data_size);
  return (Image)ret;
}

////////////////////////////////////////////////////////////////////////////////

Image img_pack_channels(Image r, Image g, Image b) {
  assert(r || g || b);
  vec2i size = i2ones;

  if (r) {
    size = v2i(MAX(size.w, r->width), MAX(size.h, r->height));
    img_resolve(r);
  }
  if (g) {
    size = v2i(MAX(size.w, g->width), MAX(size.h, g->height));
    img_resolve(g);
  }
  if (b) {
    size = v2i(MAX(size.w, b->width), MAX(size.h, b->height));
    img_resolve(b);
  }

  Image ret = img_from_color(c4white, size, 3);
  assert(ret && ret->status == S_READY);

  Image src[3] = { r, g, b };

  for (int y = 0; y < size.y; ++y) {
    for (int x = 0; x < size.y; ++x) {
      color3b pixel = b4white.rgb;

      for (int i = 0; i < 3; ++i) {
        if (src[i] && x < src[i]->width && y < src[i]->height) {
          byte* data = src[i]->data;
          data += (y * ret->width + x) * src[i]->channels;

          switch (src[i]->channels) {
            case 1: pixel.i[i] = *data;                 break;
            case 3: pixel.i[i] = b4lum(*(vec4b*)data);  break;
            case 4: pixel.i[i] = b4lum(*(vec4b*)data);  break;
            default: assert(false);                     break;
          }
        }
      }

      *((color3b*)ret->data + y * size.h + x) = pixel;
    }
  }

  return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Gets the number of actively loading images
////////////////////////////////////////////////////////////////////////////////

index_t img_loading_count(void) {
#ifdef __WASM__
  return _img_loading_count;
#else
  return (index_t)SDL_GetAtomicInt(&_img_loading_count);
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Deletes an images memory
////////////////////////////////////////////////////////////////////////////////

void _img_free(Image_Internal* img) {

  switch (img->pub.type) {
#ifdef __WASM__

    case IMG_DEFAULT:
      break;

    case IMG_ERROR:
      break;

    case IMG_HANDLE:
      js_image_delete(img->pub.handle);
      break;

    case IMG_DATA:
      js_buffer_delete(img->pub.handle);
      free(img->pub.data);
      break;
#else
    case IMG_DEFAULT:
      break;

    case IMG_ERROR:
      break;

    case IMG_HANDLE:
      stbi_image_free(img->pub.handle);
      break;

    case IMG_DATA:
      free(img->pub.data);
      break;

#endif
  }

  img->pub.handle = NULL;
  img->pub.data = NULL;
}

////////////////////////////////////////////////////////////////////////////////

void img_delete(Image* pimg) {
  if (!pimg || !*pimg) return;
  Image_Internal* img = (Image_Internal*)*pimg;

  if (img->pub.type != IMG_DEFAULT && img->pub.type != IMG_ERROR) {
    _img_free(img);
    str_delete(&img->filename_internal);
    free(img);
  }

  *pimg = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Image data manipulation functions to release handles and change data format
////////////////////////////////////////////////////////////////////////////////

void img_resolve(Image _img) {
  assert(_img);
  Image_Internal* img = (Image_Internal*)_img;
  assert(img->pub.status == S_READY);
  assert(img->pub.handle);

  if (img->pub.data) return;

  assert(img->pub.type == IMG_HANDLE);

#ifdef __WASM__
  assert(!img->data);
  size_t size_bytes = img->pub.width * img->pub.height * img->pub.channels;
  img->pub.data = malloc(size_bytes);
  js_image_extract(img->pub.data, img->pub.handle, img->pub.channels);
  js_image_delete(img->pub.handle);
  IMG_SET_HANDLE(img, size_bytes);
  img->pub.type = IMG_DATA;
#else
  img->pub.data = img->pub.handle;
#endif
}

////////////////////////////////////////////////////////////////////////////////

void img_set_layout(Image pimg, vec2i size, int channels) {
  UNUSED(pimg);
  UNUSED(size);
  UNUSED(channels);
  // TODO
}

////////////////////////////////////////////////////////////////////////////////

void img_set_size(Image pimg, vec2i size) {
  UNUSED(pimg);
  UNUSED(size);
  // TODO
}

////////////////////////////////////////////////////////////////////////////////

void img_set_channels(Image _img, int channels) {
  assert(_img);
  Image_Internal* img = (Image_Internal*)_img;
  assert(img->pub.status == S_READY);
  assert(channels > 0 && channels <= 4);
  if (img->pub.channels == channels) return;
  img_resolve(_img);

  size_t size_bytes = img->pub.width * img->pub.height * channels;
  byte* dst_data = malloc(size_bytes);
  assert(dst_data);

  for (
    size_t s = 0, d = 0;
    d < size_bytes;
    s += img->pub.channels, d += channels
  ) {
    color4b pixel;

    byte* src = (byte*)img->pub.data + s;
    switch (img->pub.channels) {
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
  img->pub.type = IMG_DATA;
  img->pub.data = dst_data;
  img->pub.channels = channels;
  IMG_SET_HANDLE(img, size_bytes);
}

////////////////////////////////////////////////////////////////////////////////
// Rearranges an image from a 2d grid of tiles into a vertical stack
////////////////////////////////////////////////////////////////////////////////

void img_repack_vertical(Image _img, vec2i dim) {
  assert(_img);
  Image_Internal* img = (Image_Internal*)_img;
  assert(img->pub.status == S_READY);
  assert(img->pub.handle);
  assert(dim.x > 0 && dim.y > 0);
  if (dim.x == 1) return;
  img_resolve(_img);

  size_t size_bytes = img->pub.width * img->pub.height * img->pub.channels;
  byte* data = malloc(size_bytes);
  assert(data);

  vec2i tile = i2div(img->pub.size, dim);
  int cell_width = img->pub.channels * tile.w;
  int full_width = cell_width * dim.w;

  byte* iter_target = data;
  for (int dy = 0; dy < dim.y; ++dy) {
    for (int dx = 0; dx < dim.x; ++dx) {
      const byte* source = (byte*)img->pub.data
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
  img->pub.type = IMG_DATA;
  img->pub.data = data;
  IMG_SET_HANDLE(img, size_bytes);
}

////////////////////////////////////////////////////////////////////////////////
// File saving
////////////////////////////////////////////////////////////////////////////////

void img_save(Image img) {
  assert(img);
  assert(img->filename.begin);
  img_save_as(img, img->filename);
}

////////////////////////////////////////////////////////////////////////////////

#ifdef __WASM__

void img_save_as(Image img, slice_t filename) {
  UNUSED(img);
  UNUSED(filename);
  assert(false);
}

#else

# define STB_IMAGE_WRITE_IMPLEMENTATION
# include "stb_image_write.h"

void img_save_as(Image img, slice_t filename) {
  assert(img);
  assert(img->status == S_READY);
  assert(filename.begin);

  // copy string to make sure the slice is null-terminated
  String file_str = str_copy(filename);
  bool success = false;
  int w = img->width;
  int h = img->height;
  int c = img->channels;

  if (str_ends_with(filename, ".jpg")) {
    success = stbi_write_jpg(file_str->begin, w, h, c, img->handle, 80);
  }
  else if (str_ends_with(filename, ".png")) {
    success = stbi_write_png(file_str->begin, w, h, c, img->handle, w * c);
  }
  else {
    assert(false);
  }

  if (!success) {
    str_log("[Image.save] Failed to save file: {}\n  {}"
    , filename, stbi_failure_reason()
    );
  }

  str_delete(&file_str);
}

#endif
