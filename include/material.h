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

#include "texture.h"
#include "vec.h"
#include "span_slice.h"

typedef struct mat_params_t {
  bool  use_diffuse_map;
  bool  use_normal_map;
  bool  use_roughness_map;
  bool  use_metalness_map;
  vec2i atlas_dimensions;
} mat_params_t;

#define MAT_MAP_COUNT 4

typedef struct _opaque_Material_t {
  slice_t     CONST name;
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
  mat_params_t params;
  bool        CONST ready;
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

// \brief Creates a new and empty material with the given params
Material  mat_new(slice_t name, mat_params_t);

// \brief Creates an empty material with the given params and atlas dimensions
Material  mat_new_atlas(slice_t name, mat_params_t, vec2i dim);

// \brief Begins loading material using the material name as the filename
void      mat_load_async(Material);

// \brief Loads material with filename override (may differ from material name)
//void    mat_load_file_async(Material, slice_t filename);

// \brief Loads atlas where each texture is from a separate image
void      mat_load_multi_async(Material, view_slice_t filenames);

// \brief Same as calling `mat_new` then `mat_load_async`.
Material  mat_new_load(slice_t name, mat_params_t);

// \brief Same as calling `mat_new_atlas` then `mat_load_async`.
Material  mat_new_atlas_load(slice_t name, mat_params_t, vec2i dim);

// \brief Begins loading all materials that aren't already loading
void      mat_load_all_async(void);

Material  mat_get(slice_t name);
void      mat_set_ext(Material, slice_t file_extension);
void      mat_build(Material);
void      mat_build_all(void);
void      mat_delete(Material* material);

void      mat_bind(Material);

#endif
