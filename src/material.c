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

typedef struct mat_images_t{
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
  mat_images_t* img;
//Image img_emissive; ?
} Material_Internal;

#define MATERIAL_INTERNAL                                                     \
  Material_Internal* m = (Material_Internal*)(m_in);                          \
  assert(m)                                                                   //

HMap _all_materials = NULL;

////////////////////////////////////////////////////////////////////////////////
// Initializes a new material
////////////////////////////////////////////////////////////////////////////////

Material mat_new(slice_t name, mat_params_t params) {
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

  *ret = (Material_Internal) {
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
    .img = NULL,
  };

  *(Material*)in_map.value = (Material)ret;

  return (Material)ret;
}

////////////////////////////////////////////////////////////////////////////////

Material mat_new_load(slice_t name, mat_params_t params) {
  Material ret = mat_new(name, params);
  mat_load_async(ret);
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Initializes a new material using texture atlases
////////////////////////////////////////////////////////////////////////////////

Material  mat_new_atlas(slice_t name, mat_params_t p, vec2i dim) {
  p.atlas_dimensions = dim;
  return mat_new(name, p);
}

////////////////////////////////////////////////////////////////////////////////

Material  mat_new_atlas_load(slice_t name, mat_params_t p, vec2i dim) {
  p.atlas_dimensions = dim;
  return mat_new_load(name, p);
}

////////////////////////////////////////////////////////////////////////////////
// Gets an existing material from the map
////////////////////////////////////////////////////////////////////////////////

Material mat_get(slice_t name) {
  if (!_all_materials) return NULL;
  Material* material = map_ref(_all_materials, &name);
  assert(material);
  return *material;
}

////////////////////////////////////////////////////////////////////////////////

void mat_set_ext(Material m_in, slice_t ext) {
  MATERIAL_INTERNAL;
  m->ext = ext;
}

////////////////////////////////////////////////////////////////////////////////
// Loads the material texture images, asynchronously in WASM mode
////////////////////////////////////////////////////////////////////////////////

void mat_load_async(Material mat) {
  span_slice_t name = span_slice(&mat->name, 1);
  mat_load_multi_async(mat, name.view);
}

////////////////////////////////////////////////////////////////////////////////

void mat_load_multi_async(Material m_in, view_slice_t filenames) {
  MATERIAL_INTERNAL;

  // Don't re-load if we've alrady loaded or are already loading
  if (m->pub.ready || m->img) return;

  m->pub.layers = view_slice_size(filenames);

  assert(m->pub.layers > 0);

  mat_images_t** img = &m->img;

  const slice_t* view_foreach(filename, filenames) {
    *img = malloc(sizeof(mat_images_t));
    assert(*img);
    **img = (mat_images_t){ 0 };

    if (m->pub.params.use_diffuse_map) {
      String file = str_format("./res/textures/{}.{}", filename, m->ext);
      (*img)->diffuse = img_load_async_str(file);
    }

    if (m->pub.params.use_normal_map) {
      String file = str_format("./res/textures/{}_n.{}", filename, m->ext);
      (*img)->normals = img_load_async_str(file);
    }

    if (m->pub.params.use_roughness_map) {
      String file = str_format("./res/textures/{}_r.{}", filename, m->ext);
      (*img)->roughness = img_load_async_str(file);
    }

    if (m->pub.params.use_metalness_map) {
      String file = str_format("./res/textures/{}_m.{}", filename, m->ext);
      (*img)->metalness = img_load_async_str(file);
    }

    (*img)->next = NULL;
    assert(*img); // MSVC doesn't understand pointers to pointer
    img = &(*img)->next;
  }
}

////////////////////////////////////////////////////////////////////////////////

void mat_load_all_async(void) {
  Material* map_foreach(material, _all_materials) {
    mat_load_async(*material);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Builds the material textures from the loaded images
////////////////////////////////////////////////////////////////////////////////

void mat_build(Material m_in) {
  MATERIAL_INTERNAL;
  if (m->pub.ready) return;
  if (!m->img) return;

  vec2i dim = m->pub.params.atlas_dimensions;

  // Doesn't currently support loading multiple files each with multiple images
  assert((dim.w == 1 && dim.h == 1) || !m->img->next);

  mat_images_t* img = m->img;

  if (!img->next) {
    assert(m->pub.layers == dim.w * dim.h);

    for (int i = 0; i < MAT_MAP_COUNT; ++i) {
      if (img->images[i]) {
        assert(img->images[i]->ready);
        m->pub.maps[i] = tex_from_image_atlas(img->images[i], dim);
        img_delete(&img->images[i]);
      }
    }
  }

  // if we're building from multiple images rather than an atlas
  else {
    // Initial setup of each map we're using
    for (int i = 0; i < MAT_MAP_COUNT; ++i) {
      if (img->images[i]) {
        assert(img->images[i]->ready);
        m->pub.maps[i] = tex_generate_atlas(
          img_format(img->images[i]), img->images[i]->size, m->pub.layers
        );
      }
    }

    // Go through the list and set each layer accordingly
    for (index_t layer = 0; layer < m->pub.layers; ++layer, img = img->next) {
      assert(img);

      for (int i = 0; i < MAT_MAP_COUNT; ++i) {
        if (img->images[i]) {
          assert(img->images[i]->ready);
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

    // Cleanup all those images we loaded
    img = m->img;
    while (img) {
      for (int i = 0; i < MAT_MAP_COUNT; ++i) {
        if (img->images[i]) img_delete(&img->images[i]);
      }

      mat_images_t* prev = img;
      img = img->next;
      free(prev);
    }
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

  m->pub.ready = true;
}

////////////////////////////////////////////////////////////////////////////////

void mat_build_all(void) {
  Material* map_foreach(material, _all_materials) {
    mat_build(*material);
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
