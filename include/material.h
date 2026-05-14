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

#ifndef WASP_MATERIAL_H_
#define WASP_MATERIAL_H_

#include "types.h"
#include "status.h"

#include "texture.h"
#include "vec.h"
#include "array_slice.h"

#define MAT_MAP_COUNT 4

typedef struct mat_params_t {
  union {
    bool  use_map[MAT_MAP_COUNT];
    struct {
      bool  use_diffuse_map;
      bool  use_normal_map;
      bool  use_roughness_map;
      bool  use_metalness_map;
    };
  };
  vec2i atlas_dimensions;
} mat_params_t;

typedef struct _opaque_Material_t {
  slice_t     CONST name;
  status_t    CONST status;
  index_t     CONST layers;
  union {
    Texture   CONST maps[MAT_MAP_COUNT];
    struct {
      Texture CONST map_diffuse;
      Texture CONST map_normals;
      Texture CONST map_roughness;
      Texture CONST map_metalness;
    };
  };
  union {
    vec3            weights;
    struct {
      float         weight_normal;
      float         weight_roughness;
      float         weight_metalness;
    };
  };
  mat_params_t      params;
}* Material;

// Material create params to denote which maps are active
#define   mat_params_none        ((mat_params_t){ 0, 0, 0, 0, si2ones })
#define   mat_params_diffuse     ((mat_params_t){ 1, 0, 0, 0, si2ones })
#define   mat_params_norm        ((mat_params_t){ 1, 1, 0, 0, si2ones })
#define   mat_params_roughness   ((mat_params_t){ 1, 0, 1, 0, si2ones })
#define   mat_params_metallic    ((mat_params_t){ 1, 0, 0, 1, si2ones })
#define   mat_params_rough_metal ((mat_params_t){ 1, 0, 1, 1, si2ones })
#define   mat_params_norm_rough  ((mat_params_t){ 1, 1, 1, 0, si2ones })
#define   mat_params_norm_metal  ((mat_params_t){ 1, 1, 0, 1, si2ones })
#define   mat_params_pbr         ((mat_params_t){ 1, 1, 1, 1, si2ones })

// \brief Creates a new material and queues its images for loading.
//
// \brief A texture given as "test.jpg" with all associated maps enabled will
//    load images from:
//    - "test.jpg"    - diffuse
//    - "test_n.jpg"  - normal map
//    - "test_r.jpg"  - roughness map
//    - "test_m.jpg"  - metallic map
//
// \returns a new single layer material.
Material    mat_new(slice_t name, mat_params_t);

// \brief Creates a new material and queues its image for loading.
//
// \brief see `mat_new` for usage details. This version differs in that the
//    single loaded image will be used to create NxM texture atlas based on the
//    `dim` argument. Layers in the atlas are read from the top left of the
//    image in left to right order.
//
// \returns a new multi-layer material with `dim.w x dim.h` layers
Material    mat_new_atlas(slice_t name, mat_params_t, vec2i dim);

// \brief Creates a new material and queues its images for loading.
//
// \brief see `mat_new` for usage details. This version differs in that each
//    filename in the given view will be loaded individually as a layer in the
//    fianl material. Currently, layers can't be atlased.
//
// \returns a new multi-layer material with filenames->count layers
Material    mat_new_multi(slice_t name, mat_params_t, view_slice_t filenames);

// \brief Gets a material by name. This name does _not_ include the
//    file extension.
Material    mat_get(slice_t name);
Array_slice mat_get_names(void);

void        mat_delete(Material* material);

void        mat_loading_manager(void);
index_t     mat_loading_count(void);

void        mat_bind(Material);

#endif
