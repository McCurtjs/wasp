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

#define WASP_MATERIAL_INTERNAL
#include "material.h"

#include "str.h"
#include "map.h"

#include <stdlib.h>

typedef struct Material_Internal {
  struct _opaque_Material_t pub;

  // hidden values
  String  name_internal;
  slice_t ext;
  Image   img_diffuse;
  Image   img_normals;
  Image   img_roughness;
  Image   img_metalness;
//Image   img_emissive; ?
} Material_Internal;

#define MATERIAL_INTERNAL \
  Material_Internal* m = (Material_Internal*)(m_in); \
  assert(m)

HMap _all_materials = NULL;

////////////////////////////////////////////////////////////////////////////////
// Initializes a new material
////////////////////////////////////////////////////////////////////////////////

Material material_new(slice_t name, material_params_t params) {
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

  if (params.atlas_dimensions.x <= 0 || params.atlas_dimensions.y <= 0) {
    params.atlas_dimensions = i2ones;
  }

  *ret = (Material_Internal){
    .pub = {
      .name = name_str->slice,
      .layers = params.atlas_dimensions.x * params.atlas_dimensions.y,
      .weight_normal = 1.0f,
      .weight_roughness = 0.5f,
      .weight_metalness = 0.0f,
      .ready = false,
      .params = params,
    },
    .name_internal = name_str,
    .ext = S("jpg"),
  };

  *(Material*)in_map.value = (Material)ret;

  return (Material)ret;
}

////////////////////////////////////////////////////////////////////////////////

Material material_new_load(slice_t name, material_params_t params) {
  Material ret = material_new(name, params);
  material_load_async(ret);
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Initializes a new material using texture atlases
////////////////////////////////////////////////////////////////////////////////

Material  material_new_atlas(slice_t name, material_params_t p, vec2i dim) {
  p.atlas_dimensions = dim;
  return material_new(name, p);

}

////////////////////////////////////////////////////////////////////////////////

Material  material_new_atlas_load(slice_t name, material_params_t p, vec2i d) {
  p.atlas_dimensions = d;
  return material_new_load(name, p);
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

  if (m->pub.params.use_diffuse_map) {
    String filename = str_format("./res/textures/{}.{}", m->pub.name, m->ext);
    m->img_diffuse = img_load_async_str(filename);
  }

  if (m->pub.params.use_normal_map) {
    String filename = str_format("./res/textures/{}_n.{}", m->pub.name, m->ext);
    m->img_normals = img_load_async_str(filename);
  }

  if (m->pub.params.use_roughness_map) {
    String filename = str_format("./res/textures/{}_r.{}", m->pub.name, m->ext);
    m->img_roughness = img_load_async_str(filename);
  }

  if (m->pub.params.use_metalness_map) {
    String filename = str_format("./res/textures/{}_m.{}", m->pub.name, m->ext);
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
  if (m->pub.ready) return;

  vec2i dim = m->pub.params.atlas_dimensions;

  if (m->img_diffuse) {
    assert(m->img_diffuse->ready);
    m->pub.map_diffuse = tex_from_image_atlas(m->img_diffuse, dim);
    img_delete(&m->img_diffuse);
  }
  else m->pub.map_diffuse = tex_get_default_white_atlas();

  if (m->img_normals) {
    assert(m->img_normals->ready);
    m->pub.map_normals = tex_from_image_atlas(m->img_normals, dim);
    img_delete(&m->img_normals);
  }
  else m->pub.map_normals = tex_get_default_normal_atlas();

  if (m->img_roughness) {
    assert(m->img_roughness->ready);
    m->pub.map_roughness = tex_from_image_atlas(m->img_roughness, dim);
    img_delete(&m->img_roughness);
  }
  else m->pub.map_roughness = tex_get_default_white_atlas();

  if (m->img_metalness) {
    assert(m->img_metalness->ready);
    m->pub.map_metalness = tex_from_image_atlas(m->img_metalness, dim);
    img_delete(&m->img_metalness);
  }
  else m->pub.map_metalness = tex_get_default_white_atlas();

  m->pub.ready = true;
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

/*
typedef struct MaterialSet_Internal {
  struct _opaque_MaterialSet_t pub;
} MaterialSet_Internal;

#define MATSET_INTERNAL \
  MaterialSet_Internal* ms = (MaterialSet_Internal*)(m_in); \
  assert(ms)
//*/
