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

#define MCLIB_INTERNAL_IMPL
#include "material.h"

#include "str.h"
#include "map.h"

#include <stdlib.h>

typedef struct mat_images_t {
  struct mat_images_t* next;
  union {
    Image images[MAT_MAP_COUNT];
    struct {
      Image diffuse;
      Image normals;
      Image roughness;
      Image metalness;
    };
  };
} mat_images_t;

typedef struct Material_Internal {
  struct _opaque_Material_t pub;

  // hidden values
  String  name_internal;
  slice_t ext;
  mat_images_t* img; // TODO: replace with dynamic Array
//Image img_emissive; ?
} Material_Internal;

#define MATERIAL_INTERNAL                                                     \
  Material_Internal* m = (Material_Internal*)(m_in);                          \
  assert(m)                                                                   //

#define con_type Material_Internal*
#define con_prefix material
#include "map.h"
#undef con_prefix
#undef con_type

static HMap_material  _all_materials_map = NULL;
static index_t        _materials_built_count = 0;

////////////////////////////////////////////////////////////////////////////////
// Helpers for handling each stage of loading and building a material
////////////////////////////////////////////////////////////////////////////////

Material_Internal* _mat_new(slice_t filename, mat_params_t params) {
  assert(!slice_is_empty(filename));

  if (!_all_materials_map) _all_materials_map = map_material_new();

  slice_t name = slice_until(filename, S("."));

  Material_Internal* ret =
    map_material_get_or_default(_all_materials_map, name, NULL);

  if (ret) return ret;

  ret = malloc(sizeof(Material_Internal));
  assert(ret);

  String filename_copy = str_copy(filename);
  name = slice_until_last(filename_copy->slice, S("."));
  slice_t ext = slice_after_last(filename_copy->slice, S("."));

  if (ext.size == 0) {
    ext = S("jpg");
  }

  if (params.atlas_dimensions.x <= 0 || params.atlas_dimensions.y <= 0) {
    params.atlas_dimensions = i2ones;
  }

  *ret = (Material_Internal) {
    .pub = {
      .name = name,
      .status = S_NEW,
      .layers = params.atlas_dimensions.x * params.atlas_dimensions.y,
      .weight_normal = 1.0f,
      .weight_roughness = 0.5f,
      .weight_metalness = 0.0f,
      .params = params,
    },
    .name_internal = filename_copy,
    .ext = ext,
    .img = NULL,
  };

  map_material_insert(_all_materials_map, name, ret);

  return ret;
}

////////////////////////////////////////////////////////////////////////////////

void _mat_load_async(Material_Internal* m, view_slice_t filenames) {
  assert(m);
  assert(filenames.end > filenames.begin);

  if (m->pub.status != S_NEW) { assert(m->img); return; }
  assert(m->img == NULL);

  // Get the correct grid size for the map (if it's an atlas)
  m->pub.layers = view_slice_size(filenames);
  if (m->pub.layers == 1) {
    vec2i dim = m->pub.params.atlas_dimensions;
    m->pub.layers = dim.x * dim.y;
  }
  else {
    // Doesn't currently support multiple files each with multiple images
    assert(m->pub.params.atlas_dimensions.x = 1);
    assert(m->pub.params.atlas_dimensions.y = 1);
  }

  assert(m->pub.layers > 0);

  mat_images_t** img = &m->img;

  const slice_t* view_foreach(filename, filenames) {
    *img = malloc(sizeof(mat_images_t));
    assert(*img);
    **img = (mat_images_t){ 0 };

    if (m->pub.params.use_diffuse_map) {
      String file = str_format("./res/textures/{}.{}", filename, m->ext);
      (*img)->diffuse = img_new_str(file);
    }

    if (m->pub.params.use_normal_map) {
      String file = str_format("./res/textures/{}_n.{}", filename, m->ext);
      (*img)->normals = img_new_str(file);
    }

    if (m->pub.params.use_roughness_map) {
      String file = str_format("./res/textures/{}_r.{}", filename, m->ext);
      (*img)->roughness = img_new_str(file);
    }

    if (m->pub.params.use_metalness_map) {
      String file = str_format("./res/textures/{}_m.{}", filename, m->ext);
      (*img)->metalness = img_new_str(file);
    }

    (*img)->next = NULL;
    assert(*img); // MSVC doesn't understand pointers to pointer
    img = &(*img)->next;
  }

  assert(m->img);
  m->pub.status = S_LOADING;
}

////////////////////////////////////////////////////////////////////////////////

void _mat_load_check(Material_Internal* m) {
  assert(m);
  assert(m->pub.status == S_LOADING);
  assert(m->img);

  status_t ret = S_BUILDING;

  mat_images_t* img = m->img;
  while (img) {
    for (index_t i = 0; i < MAT_MAP_COUNT; ++i) {
      if (m->pub.params.use_map[i] && img->images[i]->status != S_READY) {
        return;
      }
    }
    img = img->next;
  }

  m->pub.status = S_BUILDING;
}

////////////////////////////////////////////////////////////////////////////////

void _mat_build_check(Material_Internal* m) {
  assert(m);
  assert(m->img);

  if (m->pub.status != S_BUILDING) return;

  vec2i dim = m->pub.params.atlas_dimensions;

  // Doesn't currently support loading multiple files each with multiple images
  assert((dim.w == 1 && dim.h == 1) || !m->img->next);

  mat_images_t* img = m->img;

  if (!img->next) {
    assert(m->pub.layers == dim.w * dim.h);

    for (int i = 0; i < MAT_MAP_COUNT; ++i) {
      if (img->images[i]) {
        assert(img->images[i]->status == S_READY);
        m->pub.maps[i] = tex_from_image_atlas(img->images[i], dim);
        img_delete(&img->images[i]);
      }
    }
  }

  // if we're building from multiple images rather than an atlas
  else {
    // Initial setup of each map we're using
    for (int i = 0; i < MAT_MAP_COUNT; ++i) {
      vec2i size = i2zero;

      mat_images_t* node = img;
      while (node) {
        assert(node->images[i]->status == S_READY);
        vec2i img_size = node->images[i]->size;
        if (img_size.x > size.x || img_size.y > size.y) {
          size = img_size;
        }
        node = node->next;
      }

      if (size.x != 0) {
        m->pub.maps[i] = tex_generate_atlas(
          img_format(img->images[i]), size, m->pub.layers
        );
      }
    }

    // Go through the list and set each layer accordingly
    for (index_t layer = 0; layer < m->pub.layers; ++layer, img = img->next) {
      assert(img);

      for (int i = 0; i < MAT_MAP_COUNT; ++i) {
        if (img->images[i]) {
          assert(img->images[i]->status == S_READY);
          if (img->images[i]->type != IMG_ERROR) {
            assert(img->images[i]->size.w == m->pub.maps[i]->size.w);
            assert(img->images[i]->size.h == m->pub.maps[i]->size.h);
            tex_set_atlas_layer(m->pub.maps[i], layer, img->images[i]);
          }
          else {
            color4 color = c4white;
            if (&m->pub.maps[i] == &m->pub.map_normals) color = c4normal;
            tex_set_atlas_layer_color(m->pub.maps[i], layer, color);
          }
        }
      }
    }
    assert(!img);

    //// Cleanup all those images we loaded
    //img = m->img;
    //while (img) {
    //  for (int i = 0; i < MAT_MAP_COUNT; ++i) {
    //    if (img->images[i]) img_delete(&img->images[i]);
    //  }
    //
    //  mat_images_t* prev = img;
    //  img = img->next;
    //  free(prev);
    //}
  }

  // Set up default maps for unused material attributes
  for (int i = 0; i < MAT_MAP_COUNT; ++i) {
    if (m->pub.maps[i]) {
      tex_generate_mips(m->pub.maps[i]);
    }
    else {
      if (&m->pub.maps[i] == &m->pub.map_normals)
        m->pub.maps[i] = tex_get_default_normal_atlas();
      else
        m->pub.maps[i] = tex_get_default_white_atlas();
    }
  }

  m->pub.status = S_READY;
  ++_materials_built_count;
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Initialization functions to create materials
////////////////////////////////////////////////////////////////////////////////

Material mat_new(slice_t filename, mat_params_t params) {
  Material_Internal* ret = _mat_new(filename, params);
  assert(ret);

  if (ret->pub.status != S_NEW) {
    str_log("[Material.new] Name already in use: {}", ret->pub.name);
    return (Material)ret;
  }

  view_slice_t name = view_slice(&ret->pub.name, 1);
  _mat_load_async(ret, name);
  return (Material)ret;
}

////////////////////////////////////////////////////////////////////////////////

Material mat_new_atlas(slice_t name, mat_params_t p, vec2i dim) {
  p.atlas_dimensions = dim;
  return mat_new(name, p);
}

////////////////////////////////////////////////////////////////////////////////

Material mat_new_multi(slice_t name, mat_params_t p, view_slice_t filenames) {
  Material_Internal* ret = _mat_new(name, p);
  assert(ret);

  if (ret->pub.status != S_NEW) {
    str_log("[Material.new_multi] Name already in use: {}", ret->pub.name);
    return (Material)ret;
  }

  _mat_load_async(ret, filenames);
  return (Material)ret;
}

////////////////////////////////////////////////////////////////////////////////
// Functions to manage the async loading
////////////////////////////////////////////////////////////////////////////////

void mat_loading_manager(void) {
  Material_Internal** map_foreach(pmat, _all_materials_map) {
    Material_Internal* m = *pmat;

    if (m->pub.status == S_LOADING) {
      _mat_load_check(m);
    }

    if (m->pub.status == S_BUILDING) {
      _mat_build_check(m);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

index_t mat_loading_count(void) {
  index_t ret = _all_materials_map->size - _materials_built_count;
  assert(ret >= 0);
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Gets an existing material from the map
////////////////////////////////////////////////////////////////////////////////

Material mat_get(slice_t name) {
  if (!_all_materials_map) return NULL;
  return (Material)map_material_get_or_default(_all_materials_map, name, NULL);
}

////////////////////////////////////////////////////////////////////////////////

Array_slice mat_get_names(void) {
  if (!_all_materials_map) return NULL;
  Array_slice ret = arr_slice_new_reserve(_all_materials_map->size);
  Material_Internal** map_foreach(pm, _all_materials_map) {
    arr_slice_push_back(ret, (*pm)->pub.name);
  }
  return ret;
}
