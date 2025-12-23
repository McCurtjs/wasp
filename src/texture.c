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

#include "texture.h"

#include "gl.h"
#include "str.h"

static texture_t tex_default_white = { 0 };
static texture_t tex_default_normal = { 0 };

static const GLenum _rt_formats_internal[] = {
  GL_RGB8, GL_RGBA8,
  GL_RGBA16F, GL_RGB32F, GL_RGBA32F, GL_R32F, GL_RG16F,
  GL_RGB10_A2,
  GL_DEPTH_COMPONENT32F
};

static const GLenum _rt_formats[] = {
  GL_RGB, GL_RGBA,
  GL_RGBA, GL_RGB, GL_RGBA, GL_RED, GL_RG,
  GL_RGBA,
  GL_DEPTH_COMPONENT
};

static const GLenum _rt_format_type[] = {
  GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE,
  GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT,
  GL_UNSIGNED_INT_2_10_10_10_REV,
  GL_FLOAT
};

////////////////////////////////////////////////////////////////////////////////
// Initialize a texture from loaded image data
////////////////////////////////////////////////////////////////////////////////

texture_t tex_from_image(Image image) {
  texture_t ret = { 0 };

  if (!image || !image->ready) return ret;

  GLenum fmt_internal = GL_RGBA8;
  GLenum fmt = GL_RGBA;

  if (image->channels == 3) {
    fmt = GL_RGB;
    fmt_internal = GL_RGB8;
  }
  else if (image->channels == 1) {
    fmt = GL_RED;
    fmt_internal = GL_R8;
  }
  else if
  (   image->channels != 4
#ifdef __WASM__
  &&  image->channels != 0
#endif
  ) {
    assert(false);
  }

#ifdef __WASM__
  glPixelStorei(GL_UNPACK_FLIP_Y_WEBGL, GL_TRUE);
#endif

  GLenum err = glGetError();

  glGenTextures(1, &ret.handle);
  glBindTexture(GL_TEXTURE_2D, ret.handle);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D
  ( GL_TEXTURE_2D
  , 0
  , fmt_internal
  , image->width
  , image->height
  , 0
  , fmt
  , GL_UNSIGNED_BYTE
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

  return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Initialize a texture from data
////////////////////////////////////////////////////////////////////////////////

#ifdef __WASM__
extern void* js_buffer_create(const byte* bytes, uint size);
extern void  js_buffer_delete(void* data_id);
#endif

texture_t tex_from_data(
  texture_format_t format, vec2i size, const void* data
) {
  texture_t texture;
  assert(format >= 0 && format < TF_SUPPORTED_MAX);

  glGenTextures(1, &texture.handle);

#ifdef __WASM__
  void* data_buffer = js_buffer_create(
    data, size.w * size.h * sizeof(float) * 3 * 3
  );
  data = data_buffer;
#endif

  glBindTexture(GL_TEXTURE_2D, texture.handle);
  glTexImage2D
  ( GL_TEXTURE_2D
  , 0
  , _rt_formats_internal[format]
  , size.w
  , size.h
  , 0
  , _rt_formats[format]
  , _rt_format_type[format]
  , data
  );

#ifdef __WASM__
  js_buffer_delete(data_buffer);
#endif

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  return texture;
}

////////////////////////////////////////////////////////////////////////////////
// Initialize a blank texture for manual writing or render targets
////////////////////////////////////////////////////////////////////////////////

texture_t tex_generate(texture_format_t format, vec2i size) {
  texture_t texture;
  assert(format >= 0 && format < TF_SUPPORTED_MAX);

  glGenTextures(1, &texture.handle);

  glBindTexture(GL_TEXTURE_2D, texture.handle);
  glTexImage2D
  ( GL_TEXTURE_2D
  , 0
  , _rt_formats_internal[format]
  , size.w
  , size.h
  , 0
  , _rt_formats[format]
  , _rt_format_type[format]
  , NULL
  );

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

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
