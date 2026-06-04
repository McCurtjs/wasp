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

#ifndef WASP_INSTANCE_ATTRIBUTES_H_
#define WASP_INSTANCE_ATTRIBUTES_H_

// Describes the layouts of different instance attribute formats

#include "vec.h"
#include "mat.h"

typedef struct _opaque_Shader_t* Shader;

////////////////////////////////////////////////////////////////////////////////
// Instance attribute descriptors
////////////////////////////////////////////////////////////////////////////////

typedef enum attribute_format_t {
  AF_NONE,            // no associated per-instance attributes
  AF_TRANSFORM_ONLY,  // attribute_base_t
  AF_TINT,            // attribute_tint_t
  AF_MATERIAL,        // attribute_material_t
  AF_MATERIAL_TINT,   // attribute_material_tint_t
  AF_PARTICLE_POINT,
  AF_SUPPORTED_MAX
} attribute_format_t;

typedef struct attribute_base_t {
  mat4    transform;
} attribute_base_t;

typedef struct attribute_tint_t {
  mat4    transform;
  color4b tint;
} attribute_tint_t;

typedef struct attribute_material_t {
  mat4    transform;
  int     material_index;
} attribute_material_t;

typedef struct attribute_material_tint_t {
  mat4    transform;
  int     material_index;
  color4b tint;
} attribute_material_tint_t;

typedef struct attribute_particle_point_t {
  vec3    pos;
  float   scale;
} attribute_particle_point_t;

typedef struct attribute_particle_basic_t {
  vec3    pos;
  vec2    scale;
  quat    rot;
  vec4b   color;
} attribute_particle_basic_t;

typedef struct attribute_proto_t {
  mat4    transform;
  color4b tint;
  int     material_index;
} attribute_proto_t;

typedef struct attributes_t {
  union {
    mat4 transform;
    attribute_tint_t tint;
    attribute_material_t material;
    attribute_material_tint_t material_tint;
    attribute_particle_point_t particle;
    attribute_particle_basic_t particle_ext;
  };
} attributes_t;

void      attribute_bind(attribute_format_t, Shader);
void      attribute_bind_as(attribute_format_t actual, attribute_format_t as);

index_t   attribute_size(attribute_format_t);

bool      attribute_has_tint(attribute_format_t);
bool      attribute_has_material_index(attribute_format_t);

color4b*  attribute_ref_tint(attribute_format_t format, void* att);
int*      attribute_ref_material_index(attribute_format_t format, void* att);

#endif
