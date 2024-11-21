#include "obj.h"
#include "mat.h"

#include "str.h"
#include "array.h"

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

void file_load_obj(Model_Obj* model, File* file) {

  index_s pos = 0;
  StringRange str = file->str;

  Array verts = array_new(ObjVertexPart);
  Array norms = array_new(vec3);
  Array uvs   = array_new(vec2);
  Array faces = array_new(ObjFaceElem);

  while (pos < file->str.length) {
    StringRange token = str_token(str, " ", &pos);

    if (token.length == 0) break;

    switch (token.begin[0]) {

      // Read vertex info
      case 'v': {

        switch (token.begin[1]) {

          // Read a vertex normal
          case 'n': {
            ++pos;

            vec3 norm;

            istr_to_float(str_token(str, " ", &pos), &norm.x);
            istr_to_float(str_token(str, " ", &pos), &norm.y);
            istr_to_float(str_token(str, "\n", &pos), &norm.z);

            array_write_back(norms, &norm);

          } break;

          // Read a UV coordinate
          case 't': {
            ++pos;

            vec3 uv;

            istr_to_float(str_token(str, " ", &pos), &uv.u);
            istr_to_float(str_token(str, "\n", &pos), &uv.v);

            array_write_back(uvs, &uv);

          } break;

          // Read a vertex coordinate
          default: {

            ObjVertexPart vert;

            // coordinate
            istr_to_float(str_token(str, " ", &pos), &vert.pos.x);
            istr_to_float(str_token(str, " ", &pos), &vert.pos.y);
            istr_to_float(str_token(str, " \n", &pos), &vert.pos.z);

            // optional vertex color
            if (str.begin[pos - 1] == ' ') {

              istr_to_float(str_token(str, " ", &pos), &vert.color.r);
              istr_to_float(str_token(str, " ", &pos), &vert.color.g);
              istr_to_float(str_token(str, "\n", &pos), &vert.color.b);

            } else {
              vert.color = c4white.rgb;
            }

            array_write_back(verts, &vert);

          } break;

        }

      } break;

      // Read a face
      case 'f': {

        ObjFaceElem elem;

        istr_to_int(str_token(str, "/", &pos), &elem.vert);
        istr_to_int(str_token(str, "/", &pos), &elem.uv);
        istr_to_int(str_token(str, " ", &pos), &elem.norm);
        array_write_back(faces, &elem);

        istr_to_int(str_token(str, "/", &pos), &elem.vert);
        istr_to_int(str_token(str, "/", &pos), &elem.uv);
        istr_to_int(str_token(str, " ", &pos), &elem.norm);
        array_write_back(faces, &elem);

        istr_to_int(str_token(str, "/", &pos), &elem.vert);
        istr_to_int(str_token(str, "/", &pos), &elem.uv);
        istr_to_int(str_token(str, "\n", &pos), &elem.norm);
        array_write_back(faces, &elem);

      } break;

      // Skip to the next line if it's a comment, object name, or whatever s is
      default: {
        str_token(str, "\n", &pos);
      } break;

    }

  }

  model->verts = array_new_reserve(ObjVertex, faces->size);
  model->indices = array_new_reserve(uint, faces->size);

  for (index_s i = 0; i < faces->size; ++i) {
    // -1's to account for obj's 1-indexing
    ObjFaceElem* f = array_ref(faces, i);
    ObjVertexPart* partial = array_ref(verts, f->vert - 1);

    array_write_back(model->verts, &(ObjVertex) {
      .pos = partial->pos,
      .color = partial->color,
      .norm = *((vec3*)array_ref(norms, f->norm - 1)),
      .uv = *((vec2*)array_ref(uvs, f->uv - 1)),
    });

    // TODO: make this actually make sense, lol. This should put the faces
    // in an actually indexed setup, but right now it's basically just a regular
    // array instead, which defeats the purpose
    array_write_back(model->indices, &i);
  }

  array_delete(&faces);
  array_delete(&verts);
  array_delete(&uvs);
  array_delete(&norms);
}
