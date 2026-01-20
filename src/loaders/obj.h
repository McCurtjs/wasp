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

#ifndef WASP_LOADER_OBJ_H_
#define WASP_LOADER_OBJ_H_

#include "file.h"
#include "array.h"
#include "model.h"

typedef struct {
  vec3 pos;
  vec3 norm;
  vec2 uv;
  vec4 tangent;
} obj_vertex_t;

typedef struct {
  vec3 pos;
  vec3 norm;
  vec2 uv;
  vec4 tangent;
  vec3 color;
} obj_vertex_color_t;

typedef struct model_obj_t {
  Array verts;
  Array indices;
  bool has_vertex_color;
} model_obj_t;

model_obj_t file_load_obj(File file);

#endif
