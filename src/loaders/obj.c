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

model_obj_t file_load_obj(File file) {
  model_obj_t model = { 0 };

  Array verts = arr_new(obj_vertex_part_t);
  Array norms = arr_new(vec3);
  Array uvs   = arr_new(vec2);
  Array faces = arr_new(obj_face_elem_t);
  
  str_log("Model_Obj - Loading: {}", file->name);

  index_t line_pos = 0;

  while (line_pos < file->str.length) {
    slice_t line = str_token_line(file->str, &line_pos).token;

    index_t i = 0;
    while (i < line.length) {
      slice_t token = str_token_space(line, &i).token;
      if (token.length <= 0) break;

      switch (token.begin[0]) {

        // Read vertex info
        case 'v': {

          if (token.length == 1) {
            obj_vertex_part_t* vert = arr_emplace_back(verts);

            // coordinate
            slice_to_float(slice_token_space(line, &i).token, &vert->pos.x);
            slice_to_float(slice_token_space(line, &i).token, &vert->pos.y);
            slice_to_float(slice_token_space(line, &i).token, &vert->pos.z);

            // optional vertex color
            if (i != line.length) {
              model.has_vertex_color = true;
              slice_to_float(slice_token_space(line, &i).token, &vert->color.r);
              slice_to_float(slice_token_space(line, &i).token, &vert->color.g);
              slice_to_float(slice_token_space(line, &i).token, &vert->color.b);
            }
            else {
              vert->color = c4white.rgb;
            }
          }

          // Either 'vt' or 'vn' sections, read a texcoord or normal
          else {

            switch (token.begin[1]) {

              // read a vertex normal
              case 'n': {
                vec3* norm = arr_emplace_back(norms);
                slice_to_float(slice_token_space(line, &i).token, &norm->x);
                slice_to_float(slice_token_space(line, &i).token, &norm->y);
                slice_to_float(slice_token_space(line, &i).token, &norm->z);
              } break;

              // read a texture coordinate
              case 't': {
                vec2* uv = arr_emplace_back(uvs);
                slice_to_float(slice_token_space(line, &i).token, &uv->u);
                slice_to_float(slice_token_space(line, &i).token, &uv->v);
              } break;

              // unsupported vertex object
              default: {
                assert(false);
              } break;

            }

          }

        } break;

        // Read a face
        case 'f': {
          obj_face_elem_t elem0;
          slice_to_int(str_token_char(line, "/", &i).token, &elem0.vert);
          slice_to_int(str_token_char(line, "/", &i).token, &elem0.uv);
          slice_to_int(slice_token_space(line, &i).token, &elem0.norm);
          arr_write_back(faces, &elem0);

          obj_face_elem_t elem1;
          slice_to_int(str_token_char(line, "/", &i).token, &elem1.vert);
          slice_to_int(str_token_char(line, "/", &i).token, &elem1.uv);
          slice_to_int(slice_token_space(line, &i).token, &elem1.norm);
          arr_write_back(faces, &elem1);

          obj_face_elem_t elem2;
          slice_to_int(str_token_char(line, "/", &i).token, &elem2.vert);
          slice_to_int(str_token_char(line, "/", &i).token, &elem2.uv);
          slice_to_int(slice_token_space(line, &i).token, &elem2.norm);
          arr_write_back(faces, &elem2);

          while (i < line.length) {
            slice_to_int(str_token_char(line, "/", &i).token, &elem0.vert);
            slice_to_int(str_token_char(line, "/", &i).token, &elem0.uv);
            slice_to_int(slice_token_space(line, &i).token, &elem0.norm);
            arr_write_back(faces, &elem1);
            arr_write_back(faces, &elem2);
            arr_write_back(faces, &elem0);
            elem1 = elem2;
            elem2 = elem0;
          }
        } break;

        // Skip past the line if it's a comment, object name, or whatever 's' is
        default: {
          i = line.length;
        } break;

      }

    }

  }

  model.indices = arr_new_reserve(uint, faces->size);
  model.verts = model.has_vertex_color
    ? arr_new(obj_vertex_color_t)
    : arr_new(obj_vertex_t);

  // Collect face index values into into shared vertex data and the index list.
  HMap vmap = map_new(obj_face_elem_t, uint, NULL, NULL);
  map_reserve(vmap, faces->size);

  obj_face_elem_t* arr_foreach(f, faces) {
    res_ensure_t e = map_ensure(vmap, f);
    if (e.is_new) {
      obj_vertex_part_t* vert = arr_ref(verts, f->vert - 1);
      *(uint*)e.value = (uint)model.verts->size;
      arr_write_back(model.indices, e.value);
      arr_write_back(model.verts, &(obj_vertex_color_t) {
        .pos = vert->pos,
        .norm = *((vec3*)arr_ref(norms, f->norm - 1)),
        .uv = *((vec2*)arr_ref(uvs, f->uv - 1)),
        .tangent = v4zero,
        .color = vert->color,
      });
    }
    else {
      arr_write_back(model.indices, e.value);
    }
  }

  map_delete(&vmap);

  // Cleanup...
  arr_delete(&faces);
  arr_delete(&verts);
  arr_delete(&uvs);
  arr_delete(&norms);

  arr_truncate(model.verts, model.verts->size);

  str_log("  Loaded: verts: {}, indices: {}",
    model.verts->size, model.indices->size
  );

  // Calculate tangents for each vertex...
  if (model.indices->size >= 3) {
    uint* indices = model.indices->begin;

    for (index_t i = 0; i < model.indices->size; i += 3) {
      uint i0 = indices[i];
      uint i1 = indices[i + 1];
      uint i2 = indices[i + 2];

      obj_vertex_t* vtx[3] = {
        arr_ref(model.verts, i0),
        arr_ref(model.verts, i1),
        arr_ref(model.verts, i2)
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

    for (index_t i = 0; i < model.verts->size; ++i) {
      obj_vertex_t* vtx = arr_ref(model.verts, i);
      vtx->tangent.xyz = v3norm(vtx->tangent.xyz);
    }
  }

  return model;
}
