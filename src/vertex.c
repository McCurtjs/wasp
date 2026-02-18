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

#include "vertex.h"

#include "gl.h"

typedef struct vert_format_desc_t {
  int size;
} vert_format_desc_t;

static const vert_format_desc_t _vert_format[VF_SUPPORTED_MAX] = {
  { sizeof(vert_base_t) },
  { sizeof(vert_color_t) },
  { sizeof(vert_uv_t) },
  { sizeof(vert_uv_norm_t) },
  { sizeof(vert_uv_norm_color_t) }
};

#define SLOT_POS  0
#define SLOT_UV   1
#define SLOT_NORM 2
#define SLOT_TANG 3
#define SLOT_TINT 4

static inline void _vert_bind_base(vert_format_t format) {
  glEnableVertexAttribArray(SLOT_POS);
  glVertexAttribPointer(
    SLOT_POS, v3floats, GL_FLOAT, false, _vert_format[format].size, 0
  );
}

static inline void _vert_bind_Color(void) {
  const GLsizei stride = sizeof(vert_color_t);
  glEnableVertexAttribArray(SLOT_TINT);
  glVertexAttribPointer(
    SLOT_TINT, v3floats, GL_FLOAT, false, stride, &((vert_color_t*)0)->color
  );
}

static inline void _vert_bind_uv(void) {
  const GLsizei stride = sizeof(vert_uv_t);
  glEnableVertexAttribArray(SLOT_UV);
  glVertexAttribPointer(
    SLOT_UV, v2floats, GL_FLOAT, false, stride, &((vert_uv_t*)0)->uv
  );
}

static inline void _vert_bind_uv_norm(void) {
  const GLsizei stride = sizeof(vert_uv_norm_t);
  const vert_uv_norm_t* base = NULL;

  glEnableVertexAttribArray(SLOT_UV);
  glVertexAttribPointer(
    SLOT_UV, v2floats, GL_FLOAT, false, stride, &base->uv
  );

  glEnableVertexAttribArray(SLOT_NORM);
  glVertexAttribPointer(
    SLOT_NORM, v3floats, GL_FLOAT, false, stride, &base->norm
  );

  glEnableVertexAttribArray(SLOT_TANG);
  glVertexAttribPointer(
    SLOT_TANG, v4floats, GL_FLOAT, false, stride, &base->tangent
  );
}

static inline void _vert_bind_uv_norm_color(void) {
  const GLsizei stride = sizeof(vert_uv_norm_color_t);
  const vert_uv_norm_color_t* base = NULL;

  glEnableVertexAttribArray(SLOT_UV);
  glVertexAttribPointer(
    SLOT_UV, v2floats, GL_FLOAT, false, stride, &base->uv
  );

  glEnableVertexAttribArray(SLOT_NORM);
  glVertexAttribPointer(
    SLOT_NORM, v3floats, GL_FLOAT, false, stride, &base->norm
  );

  glEnableVertexAttribArray(SLOT_TANG);
  glVertexAttribPointer(
    SLOT_TANG, v4floats, GL_FLOAT, false, stride, &base->tangent
  );

  glEnableVertexAttribArray(SLOT_TINT);
  glVertexAttribPointer(
    SLOT_TINT, v3floats, GL_FLOAT, false, stride, &base->color
  );
}

void vert_bind(vert_format_t format) {
  assert(format >= 0 && format < VF_SUPPORTED_MAX);

  _vert_bind_base(format);

  switch (format) {
    case VF_POS_ONLY:                                   return;
    case VF_COLOR:          _vert_bind_Color();         return;
    case VF_UV:             _vert_bind_uv();            return;
    case VF_UV_NORM:        _vert_bind_uv_norm();       return;
    case VF_UV_NORM_COLOR:  _vert_bind_uv_norm_color(); return;
    default:                assert(false);              return;
  }
}
