#include "obj.h"
#include "mat.h"

#include "str.h"
#include "array.h"
#include "map.h"

typedef struct ObjVertex {
  vec3 pos;
  vec3 color;
  vec3 norm;
  vec2 uv;
} ObjVertex;

typedef struct ObjVertexPart {
  vec3 pos;
  vec3 color;
} ObjVertexPart;

typedef struct ObjFaceElem {
  int vert;
  int norm;
  int uv;
} ObjFaceElem;

void file_load_obj(Model_Mesh* model, File file) {

  index_t pos = 0;
  slice_t str = file->str;

  Array verts = array_new(ObjVertexPart);
  Array norms = array_new(vec3);
  Array uvs   = array_new(vec2);
  Array faces = array_new(ObjFaceElem);

  str_log("Model_Obj - Loading: {}", file->name);

  while (pos < file->str.length) {
    slice_t token = str_token(str, " ", &pos);

    if (token.length == 0) break;

    switch (token.begin[0]) {

      // Read vertex info
      case 'v': {

        // Just 'v', read a vertex coordinate:
        if (token.size == 1) {

          ObjVertexPart vert;

          // coordinate
          slice_to_float(str_token(str, " ", &pos), &vert.pos.x);
          slice_to_float(str_token(str, " ", &pos), &vert.pos.y);
          slice_to_float(str_token(str, " \n", &pos), &vert.pos.z);

          // optional vertex color
          if (str.begin[pos - 1] == ' ') {
            slice_to_float(str_token(str, " ", &pos), &vert.color.r);
            slice_to_float(str_token(str, " ", &pos), &vert.color.g);
            slice_to_float(str_token(str, "\n", &pos), &vert.color.b);
          }
          else {
            vert.color = c4white.rgb;
          }

          array_write_back(verts, &vert);
        }

        // Either 'vt' or 'vn' sections, read a texcoord or normal
        else {

          switch (token.begin[1]) {

            // Read a vertex normal
            case 'n': {
              vec3 norm;

              slice_to_float(str_token(str, " ", &pos), &norm.x);
              slice_to_float(str_token(str, " ", &pos), &norm.y);
              slice_to_float(str_token(str, "\n", &pos), &norm.z);

              array_write_back(norms, &norm);

            } break;

            // Read a UV coordinate
            case 't': {
              vec2 uv;

              slice_to_float(str_token(str, " ", &pos), &uv.u);
              slice_to_float(str_token(str, "\n", &pos), &uv.v);

              array_write_back(uvs, &uv);

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

        ObjFaceElem elem;

        slice_to_int(str_token(str, "/", &pos), &elem.vert);
        slice_to_int(str_token(str, "/", &pos), &elem.uv);
        slice_to_int(str_token(str, " ", &pos), &elem.norm);
        array_write_back(faces, &elem);

        slice_to_int(str_token(str, "/", &pos), &elem.vert);
        slice_to_int(str_token(str, "/", &pos), &elem.uv);
        slice_to_int(str_token(str, " ", &pos), &elem.norm);
        array_write_back(faces, &elem);

        slice_to_int(str_token(str, "/", &pos), &elem.vert);
        slice_to_int(str_token(str, "/", &pos), &elem.uv);
        slice_to_int(str_token(str, "\n", &pos), &elem.norm);
        array_write_back(faces, &elem);

      } break;

      // Skip to the next line if it's a comment, object name, or whatever s is
      default: {
        str_token(str, "\n", &pos);
      } break;

    }

  }

  model->verts = array_new(ObjVertex);
  model->indices = array_new_reserve(uint, faces->size);

  // Collect face index values into into shared vertex data and the index list.
  HMap vmap = map_new(ObjFaceElem, uint, NULL, NULL);

  ObjFaceElem* array_foreach(f, faces) {
    ensure_t e = map_ensure(vmap, f);
    if (e.is_new) {
      ObjVertexPart* vert = array_ref(verts, f->vert - 1);
      *(uint*)e.value = (uint)model->verts->size;
      array_write_back(model->indices, e.value);
      array_write_back(model->verts, &(ObjVertex) {
        .pos = vert->pos,
        .color = vert->color,
        .norm = *((vec3*)array_ref(norms, f->norm - 1)),
        .uv = *((vec2*)array_ref(uvs, f->uv - 1)),
      });
    }
    else {
      array_write_back(model->indices, e.value);
    }
  }

  map_delete(&vmap);

  array_delete(&faces);
  array_delete(&verts);
  array_delete(&uvs);
  array_delete(&norms);

  array_truncate(model->verts, model->verts->size);

  str_log("  Loaded: verts: {}, indices: {}",
    model->verts->size, model->indices->size
  );
}
