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

#define WASP_TEXTURE_INTERNAL
#include "texture.h"

#include "gl.h"
#include "str.h"

#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Defaults and format values
////////////////////////////////////////////////////////////////////////////////

static struct texture_t tex_default_white = { 0 };
static struct texture_t tex_default_normal = { 0 };

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

tex_format_t _tex_format_from_image(Image image) {
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

Texture tex_from_image(Image image) {
  assert(image);
  assert(image->ready);

  if (!image || !image->ready) return NULL;

  Texture ret = malloc(sizeof(*ret));
  assert(ret);

  *ret = (struct texture_t) {
    .name = str_copy(image->filename),
    .size = v2i(image->width, image->height),
    .format = _tex_format_from_image(image),
    .wrapping = TEX_WRAPPING_CLAMP,
    .layers = 0,
    .has_mips = false,
  };

  GLenum err = glGetError();

  glGenTextures(1, &ret->handle);
  glBindTexture(GL_TEXTURE_2D, ret->handle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D
  , 0 // Mipmap level
  , _rt_format[ret->format].internal
  , image->width
  , image->height
  , 0 // border (always 0)
  , _rt_format[ret->format].format
  , _rt_format[ret->format].type
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
    ret->filtering = TEX_FILTERING_CLAMP;
  }
  // Generate mipmaps for any texture that's a valid power of 2 and set up
  //    proper mipmap filtering.
  else if (isPow2(image->width) && isPow2(image->height)) {
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR
    );
    ret->filtering = TEX_FILTERING_LINEAR;
    ret->has_mips = true;
  }
  // Generic filtering for all other textures
  else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ret->filtering = TEX_FILTERING_LINEAR;
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_2D, 0);

  return (Texture)ret;
}

////////////////////////////////////////////////////////////////////////////////
// Initialize a texture from data
////////////////////////////////////////////////////////////////////////////////

#ifdef __WASM__
extern const void*  js_buffer_create(const byte* bytes, uint size);
extern void         js_buffer_delete(const void* data_id);
#endif

Texture tex_from_data(tex_format_t format, vec2i size, const void* data) {
  assert(format >= 0 && format < TF_SUPPORTED_MAX);

  Texture ret = malloc(sizeof(struct texture_t));
  assert(ret);

  *ret = (struct texture_t) {
    .name = str_copy("tex_data"),
    .size = size,
    .format = format,
    .filtering = TEX_FILTERING_LINEAR,
    .wrapping = TEX_WRAPPING_CLAMP,
    .layers = 0,
    .has_mips = false,
  };

  glGenTextures(1, &ret->handle);

#ifdef __WASM__
  const void* data_buffer = js_buffer_create(
    data, size.w * size.h * _rt_format[format].size
  );
  data = data_buffer;
#endif

  glBindTexture(GL_TEXTURE_2D, ret->handle);
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

  return (Texture)ret;
}

////////////////////////////////////////////////////////////////////////////////
// Initialize a blank texture for manual writing or render targets
////////////////////////////////////////////////////////////////////////////////

Texture tex_generate(tex_format_t format, vec2i size) {
  assert(format >= 0 && format < TF_SUPPORTED_MAX);
  assert(size.x > 0 && size.y > 0);

  Texture ret = malloc(sizeof(*ret));
  assert(ret);

  *ret = (struct texture_t) {
    .name = str_copy("tex_gen"),
    .size = size,
    .format = format,
    .filtering = TEX_FILTERING_LINEAR,
    .wrapping = TEX_WRAPPING_CLAMP,
    .layers = 0,
    .has_mips = false,
  };

  ret->name = str_empty;
  ret->size = size;
  ret->format = format;

  glGenTextures(1, &ret->handle);
  glBindTexture(GL_TEXTURE_2D, ret->handle);
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

  return (Texture)ret;
}

////////////////////////////////////////////////////////////////////////////////
// Initialize/get a default plain white texture <1, 1, 1>
////////////////////////////////////////////////////////////////////////////////

Texture tex_get_default_white(void) {
  if (tex_default_white.handle == 0) {
    tex_default_white = *tex_from_image(img_load_default_white());
  }

  return (Texture)&tex_default_white;
}

////////////////////////////////////////////////////////////////////////////////
// Initialize/get a default normal map texture <0.5, 0.5, 1.0>
////////////////////////////////////////////////////////////////////////////////

Texture tex_get_default_normal(void) {
  if (tex_default_normal.handle == 0) {
    tex_default_normal = *tex_from_image(img_load_default_normal());
  }

  return (Texture)&tex_default_normal;
}

////////////////////////////////////////////////////////////////////////////////

void tex_set_name(Texture tex, slice_t name) {
  assert(tex);
  assert(tex->name);
  str_delete(&tex->name);
  tex->name = str_copy(name);
}

////////////////////////////////////////////////////////////////////////////////

void tex_set_filtering(Texture tex, tex_filtering_t filtering) {
  assert(tex);
  assert(tex->handle);
  GLenum target = tex->layers > 0 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
  GLenum param = filtering ? GL_LINEAR : GL_NEAREST;
  glBindTexture(target, tex->handle);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, param);

  if (tex->has_mips) {
    if (tex->filtering == TEX_FILTERING_LINEAR)
      param = GL_LINEAR_MIPMAP_LINEAR;
    else
      param = GL_NEAREST;
  }

  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, param);
  glBindTexture(target, 0);
}

////////////////////////////////////////////////////////////////////////////////

void tex_set_wrapping(Texture tex, tex_wrapping_t wrapping) {
  assert(tex);
  assert(tex->handle);
  GLenum target = tex->layers > 0 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
  GLenum param;
  switch (wrapping) {
    case TEX_WRAPPING_CLAMP:  param = GL_CLAMP_TO_EDGE;   break;
    case TEX_WRAPPING_REPEAT: param = GL_REPEAT;          break;
    case TEX_WRAPPING_MIRROR: param = GL_MIRRORED_REPEAT; break;
    default: assert(false); return;
  }
  glBindTexture(target, tex->handle);
  glTexParameteri(target, GL_TEXTURE_WRAP_S, param);
  glTexParameteri(target, GL_TEXTURE_WRAP_T, param);
  glBindTexture(target, 0);
}

////////////////////////////////////////////////////////////////////////////////
// Assign the texture to a given slot and associate it with the sampler
////////////////////////////////////////////////////////////////////////////////

void tex_apply(Texture tex, uint slot, int sampler) {
  assert(tex->handle);
  assert(tex->layers >= 0);

  glActiveTexture(GL_TEXTURE0 + slot);
  glUniform1i(sampler, slot);

  GLenum target = tex->layers > 0 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
  glBindTexture(target, tex->handle);
}

////////////////////////////////////////////////////////////////////////////////
// Frees the texture
////////////////////////////////////////////////////////////////////////////////

void tex_delete(Texture* texture) {
  assert(texture && *texture);
  Texture tex = *texture;
  if (tex == &tex_default_white || tex == &tex_default_normal) return;
  glDeleteTextures(1, &tex->handle);
  str_delete(&tex->name);
  free(tex);
  *texture = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Texture array functions
////////////////////////////////////////////////////////////////////////////////

static byte* _tex_arr_vertical_clone(
  Texture tex, const byte* image_data, vec2i dim
) {
  int cell_width = _rt_format[tex->format].size * tex->size.w;
  int full_width = cell_width * dim.w;
  int cell_size = cell_width * tex->size.h;
  int source_size_bytes = cell_size * tex->layers;

  byte* data = malloc(source_size_bytes);
  assert(data);

  byte* iter_target = data;
  for (int dy = 0; dy < dim.y; ++dy) {
    for (int dx = 0; dx < dim.x; ++dx) {
      const byte* source = image_data
        + dx * cell_width
        + dy * full_width * tex->size.h;

      for (int y = 0; y < tex->size.h; ++y) {
        memcpy(iter_target, source, cell_width);
        source += full_width;
        iter_target += cell_width;
      }
    }
  }

  return data;
}

////////////////////////////////////////////////////////////////////////////////
// Creates a texture atlas from an image sectioned into a grid
////////////////////////////////////////////////////////////////////////////////

Texture tex_from_image_atlas(Image image, vec2i dim) {
  assert(image);
  assert(image->ready);
  assert(dim.x > 0 && dim.y > 0);

  Texture ret = malloc(sizeof(*ret));
  assert(ret);

  *ret = (struct texture_t) {
    .name = str_copy(image->filename),
    .size = v2i(image->width / dim.x, image->height / dim.h),
    .format = _tex_format_from_image(image),
    .layers = dim.x * dim.y,
  };

  glGenTextures(1, &ret->handle);
  glBindTexture(GL_TEXTURE_2D_ARRAY, ret->handle);

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
    size_t channels = (size_t)_rt_format[ret->format].size;
    size_t size_bytes = 
      ret->size.x * ret->size.y * ret->layers * _rt_format[ret->format].size;

    void* image_data_copy = malloc(size_bytes);
    assert(image_data_copy);
    img_copy_data(image_data_copy, image, channels);
    atlas_data = _tex_arr_vertical_clone(ret, image_data_copy, dim);
    free(image_data_copy);

    vertical_atlas = js_buffer_create(atlas_data, size_bytes);
#endif
  }

  GLsizei mip_levels = 1;
  glTexStorage3D(GL_TEXTURE_2D_ARRAY
  , mip_levels
  , _rt_format[ret->format].internal
  , ret->size.w
  , ret->size.h
  , ret->layers
  );

  glTexSubImage3D(GL_TEXTURE_2D_ARRAY
  , 0                                   // Mipmap level
  , 0, 0, 0                             // x, y, z offsets
  , ret->size.w, ret->size.h, ret->layers  // width/height/depth of change
  , _rt_format[ret->format].format
  , _rt_format[ret->format].type
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
  tex_gen_mips(ret);
#endif

  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

  return (Texture)ret;
}

////////////////////////////////////////////////////////////////////////////////

Texture tex_generate_atlas(tex_format_t format, vec2i size, int layers) {
  assert(format >= 0 && format < TF_SUPPORTED_MAX);
  assert(size.x > 0 && size.y > 0);
  assert(layers > 0);

  Texture ret = malloc(sizeof(*ret));
  assert(ret);

  *ret = (struct texture_t) {
    .name = str_copy("tex_atlas"),
    .size = size,
    .format = format,
    .layers = layers,
  };

  glGenTextures(1, &ret->handle);
  glBindTexture(GL_TEXTURE_2D_ARRAY, ret->handle);

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
// Sets a single image in an atlas - must match size and format
////////////////////////////////////////////////////////////////////////////////

void tex_set_atlas_layer(Texture tex, int layer, Image image) {
  assert(tex);
  assert(tex->handle);
  assert(tex->layers > 0);
  assert(layer >= 0);
  assert(layer < tex->layers);
  assert(image);
  assert(image->ready);
  assert(image->width == tex->size.w);
  assert(image->height == tex->size.h);

  glBindTexture(GL_TEXTURE_2D_ARRAY, tex->handle);

  glTexSubImage3D(GL_TEXTURE_2D_ARRAY
  , 0                         // Mipmap level
  , 0, 0, layer               // x, y, z offsets
  , tex->size.w, tex->size.h, 1 // width/height/depth of change
  , _rt_format[tex->format].format
  , _rt_format[tex->format].type
  , image->data
  );

  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

////////////////////////////////////////////////////////////////////////////////

void tex_gen_mips(Texture tex) {
  assert(tex);
  assert(tex->handle);
  assert(tex->layers > 0);

  glBindTexture(GL_TEXTURE_2D_ARRAY, tex->handle);
  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(
    GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR
  );

  glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}
