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

#ifndef WASP_VERTEX_H_
#define WASP_VERTEX_H_

// Describes different vertex structures and their descriptors
#include "vec.h"
#include "mat.h"

////////////////////////////////////////////////////////////////////////////////
// Vertex descriptors
////////////////////////////////////////////////////////////////////////////////

typedef enum vert_format_t {
  VF_POS_ONLY,      // vert_base_t
  VF_COLOR,         // vert_color_t
  VF_UV,            // vert_uv_t
  VF_UV_NORM,       // vert_uv_norm_t
  VF_UV_NORM_COLOR, // vert_uv_norm_color_t
  VF_SUPPORTED_MAX
} vert_format_t;

typedef struct vert_base_t {
  vec3 pos;
} vert_base_t;

typedef struct vert_color_t {
  vec3 pos;
  vec3 color;
} vert_color_t;

typedef struct vert_uv_t {
  vec3 pos;
  vec2 uv;
} vert_uv_t;

typedef struct vert_uv_norm_t {
  vec3 pos;
  vec2 uv;
  vec3 norm;
  vec4 tangent;
} vert_uv_norm_t;

typedef struct vert_uv_norm_color_t {
  vec3 pos;
  vec2 uv;
  vec3 norm;
  vec4 tangent;
  vec3 color;
} vert_uv_norm_color_t;

void vert_bind(vert_format_t);
void vert_bind_as(vert_format_t actual, vert_format_t as);

////////////////////////////////////////////////////////////////////////////////
// Instance attribute descriptors
////////////////////////////////////////////////////////////////////////////////

typedef enum attrib_format_t {
  AF_TRANSFORM, // vert_base_t
  AF_TINT,      // vert_tint_t
  AF_MAT,       // vert_cindex_t
  AF_MAT_TINT,  // vert_tint_cindex_t
  AF_SUPPORTED_MAX
} attrib_format_t;

typedef struct attrib_base_t {
  mat4    transform;
} attrib_base_t;

typedef struct attrib_tint_t {
  mat4    transform;
  color4  tint;
} attrib_tint_t;

typedef struct attrib_mat_index_t {
  mat4    transform;
  int     material_index;
} attrib_mat_index_t;

typedef struct attrib_tint_mat_index_t {
  mat4    transform;
  color4  tint;
  int     material_index;
} attrib_tint_mat_index_t;

void attrib_bind(attrib_format_t);
void attrib_bind_as(attrib_format_t actual, attrib_format_t as);

#endif
