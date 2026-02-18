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

#ifndef WASP_RENDER_TARGET_H_
#define WASP_RENDER_TARGET_H_

#include "types.h"
#include "vec.h"
#include "texture.h"

typedef enum depth_format_t {
  F_DEPTH_NONE,
  F_DEPTH_32, // default
  F_DEPTH_24_STENCIL_8,
  F_DEPTH_FORMAT_MAX
} depth_format_t;

typedef struct _opaque_RenderTarget_t {
  CONST index_t               slot_count;
  CONST Texture       * CONST textures;
  CONST tex_format_t  * CONST formats;
  CONST depth_format_t        depth_format;
        color3                clear_color;
  CONST bool                  ready;
}* RenderTarget;

RenderTarget _rt_new(index_t size, tex_format_t formats[]);
#define rt_new(...) \
  _rt_new(_va_count(__VA_ARGS__), (tex_format_t[]) { __VA_ARGS__ })

bool rt_build(RenderTarget rt, vec2i screen);
void rt_clear(RenderTarget rt);
void rt_bind(RenderTarget rt);
void rt_bind_default(void);
bool rt_is_bound(RenderTarget rt);
void rt_resize(RenderTarget rt, vec2i screen);
void rt_delete(RenderTarget* rt);

#endif
