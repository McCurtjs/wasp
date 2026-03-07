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

#ifndef WASP_IMAGE_H_
#define WASP_IMAGE_H_

#include "types.h"
#include "slice.h"
#include "span.h"
#include "vec.h"

typedef enum img_type_t {
  IMG_DEFAULT,
  IMG_ERROR,
  IMG_HANDLE,
  IMG_DATA,
} img_type_t;

typedef struct image_t {
  img_type_t    CONST type;
  String        CONST filename;
  CONST void*   CONST handle;
  CONST void*   CONST data;
  union {
    vec2i       CONST size;
    struct {
      int       CONST width;
      int       CONST height;
    };
  };
  int           CONST channels;
  bool          CONST ready;
  bool                blend;
}* Image;

//Image2 img_new(width/height);
Image img_load_async_str(String filename);
Image img_load_async(slice_t filename);
Image img_load_default_white(void);
Image img_load_default_normal(void);
Image img_from_bytes(const byte* data, vec2i size, int channels);
Image img_from_color(color4, vec2i size, int channels);
Image img_copy(Image);
//Image img_copy_resize(Image, vec2i size, int channels);
void  img_delete(Image* image);
void  img_resolve(Image*);
void  img_set_layout(Image*, vec2i size, int channels);
void  img_set_size(Image*, vec2i size);
void  img_set_channels(Image*, int channels);
void  img_repack_vertical(Image*, vec2i dim);
void  img_copy_data(void* dst, Image image, int dst_channels);

#endif
