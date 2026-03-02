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

#include "instance_attributes.h"
#include "shader.h"

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

static const vert_format_desc_t _attrib_format[AF_SUPPORTED_MAX] = {
  { 0 }, // AF_NONE
  { sizeof(attribute_base_t) },
  { sizeof(attribute_tint_t) },
  { sizeof(attribute_material_t) },
  { sizeof(attribute_material_tint_t) },
};

static inline void _attribute_bind_base(attribute_format_t format, Shader s) {
  const attribute_base_t* base = NULL;
  const GLsizei stride = _attrib_format[format].size;
  GLuint i = shader_attribute_loc(s, "model_matrix");

  glEnableVertexAttribArray(i);
  glVertexAttribPointer(i, 4, GL_FLOAT, false, stride, &base->transform.col[0]);
  glVertexAttribDivisor(i, 1);

  glEnableVertexAttribArray(++i);
  glVertexAttribPointer(i, 4, GL_FLOAT, false, stride, &base->transform.col[1]);
  glVertexAttribDivisor(i, 1);

  glEnableVertexAttribArray(++i);
  glVertexAttribPointer(i, 4, GL_FLOAT, false, stride, &base->transform.col[2]);
  glVertexAttribDivisor(i, 1);

  glEnableVertexAttribArray(++i);
  glVertexAttribPointer(i, 4, GL_FLOAT, false, stride, &base->transform.col[3]);
  glVertexAttribDivisor(i, 1);
}

static inline void _attribute_bind_tint(Shader s) {
  const attribute_tint_t* base = NULL;
  const GLsizei stride = sizeof(*base);
  GLint i = shader_attribute_loc(s, "model_tint");
  if (i < 0) return;

  glEnableVertexAttribArray(i);
  glVertexAttribPointer(i, 4, GL_UNSIGNED_BYTE, true, stride, &base->tint);
  glVertexAttribDivisor(i, 1);
}

static inline void _attribute_bind_material(Shader s) {
  const attribute_material_t* base = NULL;
  const GLsizei stride = sizeof(*base);
  GLint i = shader_attribute_loc(s, "model_material_index");
  if (i < 0) return;

  glEnableVertexAttribArray(i);
  glVertexAttribPointer(i, 1, GL_INT, false, stride, &base->material_index);
  glVertexAttribDivisor(i, 1);
}

static inline void _attribute_bind_material_tint(Shader s) {
  const attribute_material_tint_t* base = NULL;
  const GLsizei stride = sizeof(*base);

  GLint mat = shader_attribute_loc(s, "model_material_index");
  if (mat >= 0) {
    glEnableVertexAttribArray(mat);
    glVertexAttribIPointer(mat, 1, GL_INT, stride, &base->material_index);
    glVertexAttribDivisor(mat, 1);
  }

  GLint tint = shader_attribute_loc(s, "model_tint");
  if (tint >= 0) {
    glEnableVertexAttribArray(tint);
    glVertexAttribPointer(tint, 4, GL_UNSIGNED_BYTE, true, stride, &base->tint);
    glVertexAttribDivisor(tint, 1);
  }
}

void attribute_bind(attribute_format_t format, Shader s) {
  assert(format >= 0 && format < AF_SUPPORTED_MAX);
  if (format == AF_NONE) return;

  _attribute_bind_base(format, s);

  switch (format) {
    case AF_TRANSFORM_ONLY:                                   break;
    case AF_TINT:           _attribute_bind_tint(s);          break;
    case AF_MATERIAL:       _attribute_bind_material(s);      break;
    case AF_MATERIAL_TINT:  _attribute_bind_material_tint(s); break;
    default:                assert(false);                    break;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Genericized accessors
////////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
// Disable warning for the return line:
//    C33010: "Unchecked lower bound for enum format used as index"
// Obviously the bound is being checked in the assert...
# pragma warning ( disable : 33010)
#endif

index_t attribute_size(attribute_format_t format) {
  assert(format >= 0 && format < AF_SUPPORTED_MAX);
  return _attrib_format[format].size;
}

bool attribute_has_tint(attribute_format_t format) {
  assert(format >= 0 && format < AF_SUPPORTED_MAX);
  return format == AF_TINT || format == AF_MATERIAL_TINT;
}

bool attribute_has_material_index(attribute_format_t format) {
  assert(format >= 0 && format < AF_SUPPORTED_MAX);
  return format == AF_MATERIAL || format == AF_MATERIAL_TINT;
}

////////////////////////////////////////////////////////////////////////////////

color4b* attribute_ref_tint(attribute_format_t format, void* att) {
  assert(format >= 0 && format < AF_SUPPORTED_MAX);
  assert(att);

  switch (format) {

    case AF_TINT:
      return &((attribute_tint_t*)att)->tint;

    case AF_MATERIAL_TINT:
      return &((attribute_material_tint_t*)att)->tint;

    default:
      return NULL;
  }
}

int* attribute_ref_material_index(attribute_format_t format, void* att) {
  assert(format >= 0 && format < AF_SUPPORTED_MAX);
  assert(att);

  switch (format) {

    case AF_MATERIAL:
      return &((attribute_material_t*)att)->material_index;

    case AF_MATERIAL_TINT:
      return &((attribute_material_tint_t*)att)->material_index;

    default:
      return NULL;
  }
}
