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

#include "material.h"

#include "str.h"
#include "map.h"

#include <stdlib.h>

typedef struct Material_Internal {
  slice_t name;
  texture_t diffuse;
  texture_t normals;
  texture_t specular;
  float roughness;
  float metalness;
  bool ready;

  // hidden values
  String name_internal;
} Material_Internal;

#define MATERIAL_INTERNAL \
  Material_Internal* m = (Material_Internal*)(m_in); \
  assert(m)

HMap _all_materials = NULL;

Material material_new(slice_t name) {
  if (!_all_materials) {
    _all_materials =
      map_new(slice_t, Material, slice_hash_vptr, slice_compare_vptr);
  }

  res_ensure_t in_map = map_ensure(_all_materials, &name);

  if (!in_map.is_new) {
    str_log("[Material.new] Name already in use: {}", name);
    return *(Material*)in_map.value;
  }

  Material_Internal* ret = malloc(sizeof(Material_Internal));
  assert(ret);

  String name_str = str_copy(name);

  *ret = (Material_Internal) {
    .name = name_str->slice,
    .ready = false,
    .roughness = 0.5f,
    .metalness = 0.0f,
    .name_internal = name_str
  };

  *(Material*)in_map.value = (Material)ret;

  return (Material)ret;
}
