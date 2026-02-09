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

#include "texture.h"

#include "gl.h"
#include "str.h"

static texture_t tex_default_white = { 0 };
static texture_t tex_default_normal = { 0 };

typedef struct rt_format_t {
  int     size;
  GLenum  format;
  GLenum  internal;
  GLenum  type;
} rt_format_t;

static const rt_format_t _rt_format[TF_SUPPORTED_MAX] = {
  { 3,  GL_RGB,   GL_RGB8,      GL_UNSIGNED_BYTE },
  { 4,  GL_RGBA,  GL_RGBA8,     GL_UNSIGNED_BYTE },
  { 1,  GL_RED,   GL_R8,        GL_UNSIGNED_BYTE },
  { 8,  GL_RGBA,  GL_RGBA16F,   GL_FLOAT },
  { 4,  GL_RG,    GL_RG16F,     GL_FLOAT },
  { 12, GL_RGB,   GL_RGB32F,    GL_FLOAT },
  { 16, GL_RGBA,  GL_RGBA32F,   GL_FLOAT },
  { 4,  GL_RED,   GL_R32F,      GL_FLOAT },
  { 4,  GL_RGBA,  GL_RGB10_A2,  GL_UNSIGNED_INT_2_10_10_10_REV },
  { 4,  GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT32F,  GL_FLOAT }
};

texture_format_t _tex_format_from_image(Image image) {
  switch (image->channels) {
#ifdef __WASM__
    case 0: return TF_RGBA_8;
#endif
    case 1: return TF_R_8;
    case 3: return TF_RGB_8;
    case 4: return TF_RGBA_8;
    default: assert(false);
  }
  return TF_RGB_8;
}

////////////////////////////////////////////////////////////////////////////////
// Initialize a texture from loaded image data
////////////////////////////////////////////////////////////////////////////////

texture_t tex_from_image(Image image) {
  assert(image);
  assert(image->ready);

  texture_t ret = { 0 };

  if (!image || !image->ready) return ret;

  texture_format_t format = _tex_format_from_image(image);

  GLenum err = glGetError();

  glGenTextures(1, &ret.handle);
  glBindTexture(GL_TEXTURE_2D, ret.handle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D
  , 0 // Mipmap level
  , _rt_format[format].internal
  , image->width
  , image->height
  , 0 // border (always 0)
  , _rt_format[format].format
  , _rt_format[format].type
  , image->data
  );

  err = glGetError();
  if (err) {
    str_log("[Texture.from_image] Error after glTexImage2D: 0x{!x}", err);
  }

  // Don't make mipmaps for very small textures, and set them up to use
  //    "nearest" filtering, mostly to make sure the error texture is sharp.
  if (image->width < 5 || image->height < 5) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  }
  // Generate mipmaps for any texture that's a valid power of 2 and set up
  //    proper mipmap filtering.
  else if (isPow2(image->width) && isPow2(image->height)) {
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR
    );
  }
  // Generic filtering for all other textures
  else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_2D, 0);

  return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Initialize a texture from data
////////////////////////////////////////////////////////////////////////////////

#ifdef __WASM__
extern const void*  js_buffer_create(const byte* bytes, uint size);
extern void         js_buffer_delete(const void* data_id);
#endif

texture_t tex_from_data(
  texture_format_t format, vec2i size, const void* data
) {
  texture_t texture;
  assert(format >= 0 && format < TF_SUPPORTED_MAX);

  glGenTextures(1, &texture.handle);

#ifdef __WASM__
  const void* data_buffer = js_buffer_create(
    data, size.w * size.h * sizeof(float) * 3 * 3
  );
  data = data_buffer;
#endif

  glBindTexture(GL_TEXTURE_2D, texture.handle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D
  , 0 // Mipmap level
  , _rt_format[format].internal
  , size.w
  , size.h
  , 0 // border (must be 0)
  , _rt_format[format].format
  , _rt_format[format].type
  , data
  );

#ifdef __WASM__
  js_buffer_delete(data_buffer);
#endif

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glBindTexture(GL_TEXTURE_2D, 0);

  return texture;
}

////////////////////////////////////////////////////////////////////////////////
// Initialize a blank texture for manual writing or render targets
////////////////////////////////////////////////////////////////////////////////

texture_t tex_generate(texture_format_t format, vec2i size) {
  texture_t texture;
  assert(format >= 0 && format < TF_SUPPORTED_MAX);
  assert(size.x > 0 && size.y > 0);

  glGenTextures(1, &texture.handle);

  glBindTexture(GL_TEXTURE_2D, texture.handle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D
  , 0
  , _rt_format[format].internal
  , size.w
  , size.h
  , 0
  , _rt_format[format].format
  , _rt_format[format].type
  , NULL
  );

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glBindTexture(GL_TEXTURE_2D, 0);

  return texture;
}

////////////////////////////////////////////////////////////////////////////////
// Initialize/get a default plain white texture <1, 1, 1>
////////////////////////////////////////////////////////////////////////////////

texture_t tex_get_default_white(void) {
  if (tex_default_white.handle == 0) {
    tex_default_white = tex_from_image(img_load_default_white());
  }

  return tex_default_white;
}

////////////////////////////////////////////////////////////////////////////////
// Initialize/get a default normal map texture <0.5, 0.5, 1.0>
////////////////////////////////////////////////////////////////////////////////

texture_t tex_get_default_normal(void) {
  if (tex_default_normal.handle == 0) {
    tex_default_normal = tex_from_image(img_load_default_normal());
  }

  return tex_default_normal;
}

////////////////////////////////////////////////////////////////////////////////
// Assign the texture to a given slot and associate it with the sampler
////////////////////////////////////////////////////////////////////////////////

void tex_apply(texture_t texture, uint slot, int sampler) {
  glActiveTexture(GL_TEXTURE0 + slot);
  glUniform1i(sampler, slot);
  glBindTexture(GL_TEXTURE_2D, texture.handle);
}

////////////////////////////////////////////////////////////////////////////////
// Frees the texture
////////////////////////////////////////////////////////////////////////////////

void tex_free(texture_t* texture) {
  glDeleteTextures(1, &texture->handle);
  texture->handle = 0;
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Texture Array
////////////////////////////////////////////////////////////////////////////////

texture_array_t tex_arr_generate(
  texture_format_t format, vec2i size, int layers
) {
  assert(format >= 0 && format < TF_SUPPORTED_MAX);
  assert(size.x > 0 && size.y > 0);
  assert(layers > 0);

  texture_array_t ret = {
    .format = format,
    .size = size,
    .layers = layers,
  };

  glGenTextures(1, &ret.handle);
  glBindTexture(GL_TEXTURE_2D_ARRAY, ret.handle);

  GLsizei mip_levels = 1;
  glTexStorage3D(GL_TEXTURE_2D_ARRAY
  , mip_levels
  , _rt_format[format].internal
  , size.w
  , size.h
  , layers
  );

  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

  return ret;
}

////////////////////////////////////////////////////////////////////////////////

void tex_arr_set_layer(
  texture_array_t tex, int layer, Image image
) {
  assert(tex.handle);
  assert(layer >= 0 && layer < tex.layers);
  assert(image);
  assert(image->ready);
  assert(image->width == tex.size.w);
  assert(image->height == tex.size.h);

  glBindTexture(GL_TEXTURE_2D_ARRAY, tex.handle);

  glTexSubImage3D(GL_TEXTURE_2D_ARRAY
  , 0                         // Mipmap level
  , 0, 0, layer               // x, y, z offsets
  , tex.size.w, tex.size.h, 1 // width/height/depth of change
  , _rt_format[tex.format].format
  , _rt_format[tex.format].type
  , image->data
  );

  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

////////////////////////////////////////////////////////////////////////////////
// Generate a texture array from an image atlas/grid/spritesheet
////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>

static byte* _tex_arr_vertical_clone(
  texture_array_t t, const byte* image_data, vec2i dim
) {
  int cell_width = _rt_format[t.format].size * t.size.w;
  int full_width = cell_width * dim.w;
  int cell_size = cell_width * t.size.h;
  int source_size_bytes = cell_size * t.layers;

  byte* data = malloc(source_size_bytes);
  assert(data);

  byte* iter_target = data;
  for (int dy = 0; dy < dim.y; ++dy) {
    for (int dx = 0; dx < dim.x; ++dx) {
      const byte* source = image_data
        + dx * cell_width
        + dy * full_width * t.size.h;

      for (int y = 0; y < t.size.h; ++y) {
        memcpy(iter_target, source, cell_width);
        source += full_width;
        iter_target += cell_width;
      }
    }
  }

  return data;
}

////////////////////////////////////////////////////////////////////////////////

texture_array_t tex_arr_from_image(Image image, vec2i dim) {
  assert(image);
  assert(image->ready);
  assert(dim.x > 0 && dim.y > 0);

  texture_array_t ret = {
    .format = _tex_format_from_image(image),
    .size = v2i(image->width / dim.x, image->height / dim.h),
    .layers = dim.x * dim.y,
  };

  glGenTextures(1, &ret.handle);
  glBindTexture(GL_TEXTURE_2D_ARRAY, ret.handle);

  // If the image is already a vertical atlas, we can use it directly
  void* atlas_data = NULL;
  const void* vertical_atlas = NULL;
  if (dim.x == 1) {
    vertical_atlas = image->data;
  }
  // If it isn't already a vertical atlas, we need to make one
  else {
#ifndef __WASM__
    atlas_data = _tex_arr_vertical_clone(ret, image->data, dim);
    vertical_atlas = atlas_data;
#else
    size_t channels = (size_t)_rt_format[ret.format].size;
    size_t size_bytes = ret.size.x * ret.size.y * channels * ret.layers;

    void* image_data_copy = malloc(size_bytes);
    assert(image_data_copy);
    img_copy_data(image_data_copy, image, channels);
    atlas_data = _tex_arr_vertical_clone(ret, image_data_copy, dim);
    free(image_data_copy);

    vertical_atlas = js_buffer_create(
      atlas_data, ret.size.x * ret.size.y * ret.layers * _rt_format[ret.format].size
    );
#endif
  }

  GLsizei mip_levels = 1;
  glTexStorage3D(GL_TEXTURE_2D_ARRAY
  , mip_levels
  , _rt_format[ret.format].internal
  , ret.size.w
  , ret.size.h
  , ret.layers
  );

  glTexSubImage3D(GL_TEXTURE_2D_ARRAY
  , 0                                   // Mipmap level
  , 0, 0, 0                             // x, y, z offsets
  , ret.size.w, ret.size.h, ret.layers  // width/height/depth of change
  , _rt_format[ret.format].format
  , _rt_format[ret.format].type
  , vertical_atlas
  );

  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  if (atlas_data) {
#ifdef __WASM__
    js_buffer_delete(vertical_atlas);
#endif
    free(atlas_data);
  }

#ifndef __WASM__
  tex_arr_gen_mips(ret);
#endif

  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

  return ret;
}

////////////////////////////////////////////////////////////////////////////////

void tex_arr_apply(texture_array_t tarr, uint slot, int sampler) {
  glActiveTexture(GL_TEXTURE0 + slot);
  glUniform1i(sampler, slot);
  glBindTexture(GL_TEXTURE_2D_ARRAY, tarr.handle);
}

////////////////////////////////////////////////////////////////////////////////

void tex_arr_gen_mips(texture_array_t array) {
  glBindTexture(GL_TEXTURE_2D_ARRAY, array.handle);
  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(
    GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR
  );

  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

////////////////////////////////////////////////////////////////////////////////

void tex_arr_free(texture_array_t* tex_array) {
  glDeleteTextures(1, &tex_array->handle);
  tex_array->handle = 0;
}
