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

static const GLenum _rt_formats_internal[] = {
  GL_RGB8, GL_RGBA8,
  GL_RGBA16F, GL_RGB10_A2, GL_R32F,
  GL_DEPTH_COMPONENT32
};

static const GLenum _rt_formats[] = {
  GL_RGB, GL_RGBA,
  GL_RGBA, GL_RGBA, GL_RED, 
  GL_DEPTH_COMPONENT
};

static const GLenum _rt_format_type[] = {
  GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE,
  GL_FLOAT, GL_FLOAT, GL_FLOAT,
  GL_UNSIGNED_INT
};

texture_t tex_from_image(Image image) {
  texture_t ret = { 0 };

  if (!image || !image->ready) return ret;

  GLenum fmt_internal = GL_RGBA8;
  GLenum fmt = GL_RGBA;

  if (image->channels == 3) {
    fmt = GL_RGB;
    fmt_internal = GL_RGB8;
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

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &ret.handle);
  glBindTexture(GL_TEXTURE_2D, ret.handle);
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

  if (isPow2(image->width) && isPow2(image->height)) {
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  return ret;
}

texture_t tex_generate_blank(uint width, uint height) {
  texture_t texture;
  glGenTextures(1, &texture.handle);

  glBindTexture(GL_TEXTURE_2D, texture.handle);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  return texture;
}

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

void tex_apply(texture_t texture, uint slot, int sampler) {
  glActiveTexture(GL_TEXTURE0 + slot);
  glUniform1i(sampler, slot);
  glBindTexture(GL_TEXTURE_2D, texture.handle);
}

void tex_free(texture_t* texture) {
  glDeleteTextures(1, &texture->handle);
  texture->handle = 0;
}
