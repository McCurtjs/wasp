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

#ifndef WASP_TEXTURE_H_
#define WASP_TEXTURE_H_

#include "types.h"
#include "image.h"
#include "vec.h"

typedef enum tex_format_t {
  TF_RGB_8,
  TF_RGBA_8,
  TF_R_8,
  TF_RGBA_16,
  TF_RG_16,
  TF_RGB_32,
  TF_RGBA_32,
  TF_R_32,
  TF_RGB_10_A_2,
  TF_DEPTH_32,
  TF_SUPPORTED_MAX
} tex_format_t;

typedef enum tex_filtering_t {
  TEX_FILTERING_LINEAR,
  TEX_FILTERING_CLAMP
} tex_filtering_t;

typedef enum tex_wrapping_t {
  TEX_WRAPPING_CLAMP,
  TEX_WRAPPING_REPEAT,
  TEX_WRAPPING_MIRROR
} tex_wrapping_t;

#ifdef WASP_TEXTURE_INTERNAL
#undef CONST
#define CONST
#endif

typedef struct texture_t {
  String          CONST name;
  vec2i           CONST size;
  tex_format_t    CONST format;
  tex_filtering_t CONST filtering;
  tex_wrapping_t  CONST wrapping;
  int             CONST layers;
  uint            CONST handle;
  bool            CONST has_mips;
}* Texture;

Texture tex_from_image(Image);
Texture tex_from_image_atlas(Image, vec2i dim);
Texture tex_from_data(tex_format_t, vec2i size, const void* data);
Texture tex_generate(tex_format_t, vec2i size);
Texture tex_generate_atlas(tex_format_t, vec2i size, int layers);
Texture tex_get_default_white(void);
Texture tex_get_default_normal(void);
void    tex_set_name(Texture, slice_t name);
void    tex_set_filtering(Texture, tex_filtering_t filtering);
void    tex_set_wrapping(Texture, tex_wrapping_t wrapping);
void    tex_set_atlas_layer(Texture, int layer, Image);
void    tex_gen_mips(Texture);
void    tex_apply(Texture, uint slot, int sampler);
void    tex_delete(Texture*);

//Texture tex_atlas_from_data(tex_format_t, vec2i size, const void* dat);
//void    tex_atlas_set_layers(Texture, int layr, vec2i dim, Image);

#ifdef WASP_TEXTURE_INTERNAL
#undef CONST
#define CONST const
#endif

#endif

// Specialty function for when both texture and light headers are included
#ifdef WASP_LIGHT_H_
Texture tex_from_lights(void);
#endif
