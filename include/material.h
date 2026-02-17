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

typedef struct material_params_t {
  bool  use_diffuse_map;
  bool  use_normal_map;
  bool  use_roughness_map;
  bool  use_metalness_map;
  vec2i atlas_dimensions;
} material_params_t;

#ifdef WASP_MATERIAL_INTERNAL
#undef CONST
#define CONST
#endif

typedef struct _opaque_Material_t {
  slice_t     CONST name;
  int         CONST layers;
  Texture     CONST map_diffuse;
  Texture     CONST map_normals;
  Texture     CONST map_roughness;
  Texture     CONST map_metalness;
  union {
    vec3            weights;
    struct {
      float         weight_normal;
      float         weight_roughness;
      float         weight_metalness;
    };
  };
  material_params_t params;
  bool        CONST ready;
}* Material;

#ifdef WASP_MATERIAL_INTERNAL
#undef CONST
#define CONST const
#endif

// Material create params to denote which maps are active
#define matparams_none        ((material_params_t){ 0, 0, 0, 0, si2ones })
#define matparams_diffuse     ((material_params_t){ 1, 0, 0, 0, si2ones })
#define matparams_norm        ((material_params_t){ 1, 1, 0, 0, si2ones })
#define matparams_roughness   ((material_params_t){ 1, 0, 1, 0, si2ones })
#define matparams_metallic    ((material_params_t){ 1, 0, 0, 1, si2ones })
#define matparams_rough_metal ((material_params_t){ 1, 0, 1, 1, si2ones })
#define matparams_norm_rough  ((material_params_t){ 1, 1, 1, 0, si2ones })
#define matparams_norm_metal  ((material_params_t){ 1, 1, 0, 1, si2ones })
#define matparams_pbr         ((material_params_t){ 1, 1, 1, 1, si2ones })

Material  material_new(slice_t name, material_params_t);
Material  material_new_load(slice_t name, material_params_t);
Material  material_new_atlas(slice_t name, material_params_t, vec2i dim);
Material  material_new_atlas_load(slice_t name, material_params_t, vec2i dim);
Material  material_get(slice_t name);
void      material_load_async(Material);
void      material_build(Material);
void      material_load_all_async(void);
void      material_build_all(void);
void      material_delete(Material* material);

void      mateiral_bind(Material);

#endif
