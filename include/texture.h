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

#ifndef WASP_TEXTURE_H_
#define WASP_TEXTURE_H_

#include "types.h"
#include "image.h"
#include "vec.h"

typedef enum texture_format_t {
  TF_RGB_8,
  TF_RGBA_8,
  TF_RGBA_16,
  TF_RGBA_32,
  TF_R_32,
  TF_RG_16,
  TF_RGB_10_A_2,
  TF_DEPTH_32,
  TF_SUPPORTED_MAX
} texture_format_t;

typedef struct texture_t {
  uint handle;
} texture_t;

//Texture texture_new(texture_format_t format, vec2i size);
//Texture texture_new_from_image(Image image);

texture_t tex_from_image(Image image);
texture_t tex_generate(texture_format_t format, vec2i size);
texture_t tex_get_default_white(void);
texture_t tex_get_default_normal(void);
void      tex_apply(texture_t texture, uint slot, int sampler);
void      tex_free(texture_t* handle);

#endif
