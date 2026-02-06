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
  texture_t map_diffuse;
  texture_t map_normals;
  texture_t map_roughness;
  texture_t map_metalness;
  union {
    vec3    weights;
    struct {
      float weight_normal;
      float weight_roughness;
      float weight_metalness;
    };
  };
  bool ready;
  bool use_diffuse_map;
  bool use_normal_map;
  bool use_roughness_map;
  bool use_metalness_map;

  // hidden values
  String  name_internal;
  slice_t ext;
  Image   img_diffuse;
  Image   img_normals;
  Image   img_roughness;
  Image   img_metalness;
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
    .weight_normal = 1.0f,
    .weight_roughness = 0.5f,
    .weight_metalness = 0.0f,
    .ready = false,
    .use_diffuse_map = true,
    .use_normal_map = false,
    .use_roughness_map = false,
    .use_metalness_map = false,
    .name_internal = name_str,
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
    String filename = str_format("./res/textures/{}.{}", m->name, m->ext);
    m->img_diffuse = img_load_async_str(filename);
  }

  if (m->use_normal_map) {
    String filename = str_format("./res/textures/{}_n.{}", m->name, m->ext);
    m->img_normals = img_load_async_str(filename);
  }

  if (m->use_roughness_map) {
    String filename = str_format("./res/textures/{}_r.{}", m->name, m->ext);
    m->img_roughness = img_load_async_str(filename);
  }

  if (m->use_metalness_map) {
    String filename = str_format("./res/textures/{}_m.{}", m->name, m->ext);
    m->img_metalness = img_load_async_str(filename);
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
    m->map_diffuse = tex_from_image(m->img_diffuse);
    img_delete(&m->img_diffuse);
  }
  else m->map_diffuse = tex_get_default_white();

  if (m->img_normals) {
    assert(m->img_normals->ready);
    m->map_normals = tex_from_image(m->img_normals);
    img_delete(&m->img_normals);
  }
  else m->map_normals = tex_get_default_normal();

  if (m->img_roughness) {
    assert(m->img_roughness->ready);
    m->map_roughness = tex_from_image(m->img_roughness);
    img_delete(&m->img_roughness);
  }
  else m->map_roughness = tex_get_default_white();

  if (m->img_metalness) {
    assert(m->img_metalness->ready);
    m->map_metalness = tex_from_image(m->img_metalness);
    img_delete(&m->img_metalness);
  }
  else m->map_metalness = tex_get_default_white();

  m->ready = true;
}

////////////////////////////////////////////////////////////////////////////////

void material_build_all(void) {
  Material* map_foreach(material, _all_materials) {
    material_build(*material);
  }
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Material sets
////////////////////////////////////////////////////////////////////////////////

typedef struct MaterialSet_Internal {
  struct _opaque_MaterialSet_t pub;
} MaterialSet_Internal;

#define MATSET_INTERNAL \
  MaterialSet_Internal* ms = (MaterialSet_Internal*)(m_in); \
  assert(ms)


