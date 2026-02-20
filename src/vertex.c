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

#ifdef _MSC_VER
// Disable the warning about not checking for null - the last argument for
//    glVertexAttribPointer is an offset into the object, but is given as a
//    pointer which lets us use the actual name of the offset when the source
//    value is null. We're not actually dereferencing the object though, but
//    MSVC doesn't know that.
# pragma warning ( disable : 6011 )
#endif

typedef struct vert_format_desc_t {
  int size;
} vert_format_desc_t;

static const vert_format_desc_t _vert_format[VF_SUPPORTED_MAX] = {
  { sizeof(vert_base_t) },
  { sizeof(vert_color_t) },
  { sizeof(vert_uv_t) },
  { sizeof(vert_uv_norm_t) },
  { sizeof(vert_uv_norm_color_t) },
  { sizeof(vert_sprites_t) }
};

#define SLOT_POS  0 // v3f Vertex position
#define SLOT_UV   1 // v2f UV coordinate
#define SLOT_NORM 2 // v3f Vertex normal
#define SLOT_TANG 3 // v4f Vertex tangent
#define SLOT_TINT 4 // v3f tint color (should be v3b or v4b)

static inline void _vert_bind_base(vert_format_t format) {
  const vert_base_t* base = NULL;
  const GLsizei stride = _vert_format[format].size;
  GLint channels = format == VF_SPRITES ? v2floats : v3floats;

  glEnableVertexAttribArray(SLOT_POS);
  glVertexAttribPointer(
    SLOT_POS, channels, GL_FLOAT, false, stride, &base->pos
  );
}

static inline void _vert_bind_Color(void) {
  const vert_color_t* base = NULL;
  const GLsizei stride = sizeof(*base);

  glEnableVertexAttribArray(SLOT_TINT);
  glVertexAttribPointer(
    SLOT_TINT, v3floats, GL_FLOAT, false, stride, &base->color
  );
}

static inline void _vert_bind_uv(void) {
  const vert_uv_t* base = NULL;
  const GLsizei stride = sizeof(*base);

  glEnableVertexAttribArray(SLOT_UV);
  glVertexAttribPointer(
    SLOT_UV, v2floats, GL_FLOAT, false, stride, &base->uv
  );
}

static inline void _vert_bind_uv_norm(void) {
  const vert_uv_norm_t* base = NULL;
  const GLsizei stride = sizeof(*base);

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
  const vert_uv_norm_color_t* base = NULL;
  const GLsizei stride = sizeof(*base);

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

static inline void _vert_bind_sprites(void) {
  const vert_sprites_t* base = NULL;
  const GLsizei stride = sizeof(*base);

  glEnableVertexAttribArray(SLOT_POS);
  glVertexAttribPointer(
    SLOT_POS, v2floats, GL_FLOAT, false, stride, &base->pos
  );

  glEnableVertexAttribArray(SLOT_UV);
  glVertexAttribPointer(
    SLOT_UV, v2floats, GL_FLOAT, false, stride, &base->uv
  );

  glEnableVertexAttribArray(SLOT_NORM);
  glVertexAttribPointer(
    SLOT_NORM, v3floats, GL_FLOAT, false, stride, &base->norm
  );

  glEnableVertexAttribArray(SLOT_TINT);
  glVertexAttribPointer(
    SLOT_TINT, b3bytes, GL_UNSIGNED_BYTE, false, stride, &base->color
  );
}

void vert_bind(vert_format_t format) {
  assert(format >= 0 && format < VF_SUPPORTED_MAX);

  // Every vertex format binds the position in slot 0
  _vert_bind_base(format);

  switch (format) {
    case VF_POS_ONLY:                                   return;
    case VF_COLOR:          _vert_bind_Color();         return;
    case VF_UV:             _vert_bind_uv();            return;
    case VF_UV_NORM:        _vert_bind_uv_norm();       return;
    case VF_UV_NORM_COLOR:  _vert_bind_uv_norm_color(); return;
    case VF_SPRITES:        _vert_bind_sprites();       return;
    default:                assert(false);              return;
  }
}
