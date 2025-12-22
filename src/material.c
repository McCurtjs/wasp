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
  float normal_weight;
  bool ready;
  bool use_diffuse_map;
  bool use_normal_map;
  bool use_specular_map;

  // hidden values
  String name_internal;
  slice_t ext;
  Image img_diffuse;
  Image img_normals;
  Image img_specular;
} Material_Internal;

#define MATERIAL_INTERNAL \
  Material_Internal* m = (Material_Internal*)(m_in); \
  assert(m)

HMap _all_materials = NULL;

////////////////////////////////////////////////////////////////////////////////
// Initializes a new material
////////////////////////////////////////////////////////////////////////////////

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
    .normal_weight = 1.0f,
    .name_internal = name_str,
    .use_diffuse_map = true,
    .use_normal_map = false,
    .use_specular_map = false,
    .ext = S("jpg"),
  };

  *(Material*)in_map.value = (Material)ret;

  return (Material)ret;
}

////////////////////////////////////////////////////////////////////////////////

Material material_new_load(slice_t name) {
  Material ret = material_new(name);
  material_load_async(ret);
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Gets an existing material from the map
////////////////////////////////////////////////////////////////////////////////

Material material_get(slice_t name) {
  if (!_all_materials) return NULL;
  Material* material = map_ref(_all_materials, &name);
  assert(material);
  return *material;
}

////////////////////////////////////////////////////////////////////////////////
// Loads the material texture images, asynchronously in WASM mode
////////////////////////////////////////////////////////////////////////////////

void material_load_async(Material m_in) {
  MATERIAL_INTERNAL;

  if (m->use_diffuse_map) {
    String filename_d = str_format("./res/textures/{}.{}", m->name, m->ext);
    m->img_diffuse = img_load_async_str(filename_d);
  }

  if (m->use_normal_map) {
    String filename_n = str_format("./res/textures/{}_n.{}", m->name, m->ext);
    m->img_normals = img_load_async_str(filename_n);
  }

  if (m->use_specular_map) {
    String filename_s = str_format("./res/textures/{}_s.{}", m->name, m->ext);
    m->img_specular = img_load_async_str(filename_s);
  }
}

////////////////////////////////////////////////////////////////////////////////

void material_load_all_async(void) {
  Material* map_foreach(material, _all_materials) {
    material_load_async(*material);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Builds the material textures from the loaded images
////////////////////////////////////////////////////////////////////////////////

void material_build(Material m_in) {
  MATERIAL_INTERNAL;
  if (m->ready) return;

  if (m->img_diffuse) {
    assert(m->img_diffuse->ready);
    m->diffuse = tex_from_image(m->img_diffuse);
    img_delete(&m->img_diffuse);
  }
  else m->diffuse = tex_get_default_white();

  if (m->img_normals) {
    assert(m->img_normals->ready);
    m->normals = tex_from_image(m->img_normals);
    img_delete(&m->img_normals);
  }
  else m->normals = tex_get_default_normal();

  if (m->img_specular) {
    assert(m->img_specular->ready);
    m->specular = tex_from_image(m->img_specular);
    img_delete(&m->img_specular);
  }
  else m->specular = tex_get_default_white();

  m->ready = true;
}

////////////////////////////////////////////////////////////////////////////////

void material_build_all(void) {
  Material* map_foreach(material, _all_materials) {
    material_build(*material);
  }
}
