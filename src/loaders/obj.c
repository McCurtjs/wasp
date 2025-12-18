#include "obj.h"
#include "mat.h"

#include "str.h"
#include "array.h"
#include "map.h"

#include <math.h>

typedef struct {
  vec3 pos;
  vec3 color;
} obj_vertex_part_t;

typedef struct {
  int vert;
  int norm;
  int uv;
} obj_face_elem_t;

void file_load_obj(Model_Mesh* model, File file) {

  index_t pos = 0;
  slice_t str = file->str;

  Array verts = arr_new(obj_vertex_part_t);
  Array norms = arr_new(vec3);
  Array uvs   = arr_new(vec2);
  Array faces = arr_new(obj_face_elem_t);
  
  str_log("Model_Obj - Loading: {}", file->name);

  while (pos < file->str.length) {
    slice_t token = str_token(str, S(" "), &pos).token;

    if (token.length == 0) break;

    switch (token.begin[0]) {

      // Read vertex info
      case 'v': {

        // Just 'v', read a vertex coordinate:
        if (token.size == 1) {

          obj_vertex_part_t* vert = arr_emplace_back(verts);

          // coordinate
          slice_to_float(str_token_char(str, " ", &pos).token, &vert->pos.x);
          slice_to_float(str_token_char(str, " ", &pos).token, &vert->pos.y);
          slice_to_float(str_token_char(str, " \n", &pos).token, &vert->pos.z);

          // optional vertex color
          if (str.begin[pos - 1] == ' ') {
            model->use_color = true;
            vec3* color = &vert->color;
            slice_to_float(str_token_char(str, " ", &pos).token, &color->r);
            slice_to_float(str_token_char(str, " ", &pos).token, &color->g);
            slice_to_float(str_token_char(str, "\n", &pos).token, &color->b);
          }
          else {
            vert->color = c4white.rgb;
          }
        }

        // Either 'vt' or 'vn' sections, read a texcoord or normal
        else {

          switch (token.begin[1]) {

            // Read a vertex normal
            case 'n': {
              vec3* norm = arr_emplace_back(norms);

              slice_to_float(str_token_char(str, " ", &pos).token, &norm->x);
              slice_to_float(str_token_char(str, " ", &pos).token, &norm->y);
              slice_to_float(str_token_char(str, "\n", &pos).token, &norm->z);

            } break;

            // Read a UV coordinate
            case 't': {
              vec2* uv = arr_emplace_back(uvs);

              slice_to_float(str_token_char(str, " ", &pos).token, &uv->u);
              slice_to_float(str_token_char(str, "\n", &pos).token, &uv->v);

            } break;

            // Read a vertex coordinate
            default: {
              assert(false);
            } break;

          }

        }

      } break;

      // Read a face
      case 'f': {

        obj_face_elem_t elem;

        slice_to_int(slice_token_char(str, S("/"), &pos).token, &elem.vert);
        slice_to_int(slice_token_char(str, S("/"), &pos).token, &elem.uv);
        slice_to_int(slice_token_char(str, S(" "), &pos).token, &elem.norm);
        arr_write_back(faces, &elem);

        slice_to_int(slice_token_char(str, S("/"), &pos).token, &elem.vert);
        slice_to_int(slice_token_char(str, S("/"), &pos).token, &elem.uv);
        slice_to_int(slice_token_char(str, S(" "), &pos).token, &elem.norm);
        arr_write_back(faces, &elem);

        slice_to_int(slice_token_char(str, S("/"), &pos).token, &elem.vert);
        slice_to_int(slice_token_char(str, S("/"), &pos).token, &elem.uv);
        slice_to_int(slice_token_char(str, S("\n"), &pos).token, &elem.norm);
        arr_write_back(faces, &elem);

      } break;

      // Skip to the next line if it's a comment, object name, or whatever s is
      default: {
        str_token(str, "\n", &pos);
      } break;

    }

  }

  model->indices = arr_new_reserve(uint, faces->size);
  model->verts = model->use_color
    ? arr_new(obj_vertex_color_t)
    : arr_new(obj_vertex_t);

  // Collect face index values into into shared vertex data and the index list.
  HMap vmap = map_new(obj_face_elem_t, uint, NULL, NULL);

  obj_face_elem_t* arr_foreach(f, faces) {
    res_ensure_t e = map_ensure(vmap, f);
    if (e.is_new) {
      obj_vertex_part_t* vert = arr_ref(verts, f->vert - 1);
      *(uint*)e.value = (uint)model->verts->size;
      arr_write_back(model->indices, e.value);
      arr_write_back(model->verts, &(obj_vertex_color_t) {
        .pos = vert->pos,
        .norm = *((vec3*)arr_ref(norms, f->norm - 1)),
        .uv = *((vec2*)arr_ref(uvs, f->uv - 1)),
        .tangent = v4zero,
        .color = vert->color,
      });
    }
    else {
      arr_write_back(model->indices, e.value);
    }
  }

  map_delete(&vmap);

  // Calculate tangents for each vertex...
  if (model->indices->size >= 3) {
    uint* indices = model->indices->begin;

    for (index_t i = 0; i < model->indices->size; i += 3) {
      uint i0 = indices[i];
      uint i1 = indices[i + 1];
      uint i2 = indices[i + 2];

      obj_vertex_t* vtx[3] = {
        arr_ref(model->verts, i0),
        arr_ref(model->verts, i1),
        arr_ref(model->verts, i2)
      };

      vec3 n0 = vtx[0]->norm;
      vec3 p0 = vtx[0]->pos;
      //vec2 uv0 = vtx[0]->uv;

      vec3 p1 = vtx[1]->pos;
      vec2 uv1 = vtx[1]->uv;

      vec3 p2 = vtx[2]->pos;
      vec2 uv2 = vtx[2]->uv;

      vec3 e1 = v3sub(p1, p0);
      vec3 e2 = v3sub(p2, p0);

      float uv_cross = v2cross(uv1, uv2);
      vec3 tangent;
      float handedness;

      if (fabs(uv_cross) < 0.000001f) {
        tangent = v3perp(n0);
        //tangent = v3zero;
        handedness = 1;
      }
      else {
        float r = 1.0f / uv_cross;

        tangent = v3sub(v3scale(e1, uv2.y), v3scale(e2, uv1.y));
        vec3 bitangent = v3sub(v3scale(e2, uv1.x), v3scale(e1, uv2.x));
        tangent = v3scale(tangent, r);
        bitangent = v3scale(bitangent, r);

        handedness = v3dot(v3cross(n0, tangent), bitangent) < 0 ? -1.0f : 1.0f;
      }

      for (index_t j = 0; j < 3; ++j) {
        vtx[j]->tangent.xyz = v3add(vtx[j]->tangent.xyz, tangent);
        if (vtx[j]->tangent.w == 0.0f) vtx[j]->tangent.w = handedness;
      }
    }

    for (index_t i = 0; i < model->verts->size; ++i) {
      obj_vertex_t* vtx = arr_ref(model->verts, i);
      vtx->tangent.xyz = v3norm(vtx->tangent.xyz);
    }
  }

  // Cleanup...
  arr_delete(&faces);
  arr_delete(&verts);
  arr_delete(&uvs);
  arr_delete(&norms);

  arr_truncate(model->verts, model->verts->size);
  model->index_count = (int)model->indices->size;

  str_log("  Loaded: verts: {}, indices: {}",
    model->verts->size, model->indices->size
  );
}

/*

vec3 e1 = p1 - p0;
vec3 e2 = p2 - p0;
Edges in UV space
c
Copy code
vec2 dUV1 = uv1 - uv0;
vec2 dUV2 = uv2 - uv0;
We want vectors T and B such that :

text
Copy code
e1 ≈ T* dUV1.x + B * dUV1.y
e2 ≈ T * dUV2.x + B * dUV2.y

*/
