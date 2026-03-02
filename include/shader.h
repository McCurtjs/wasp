/*******************************************************************************
* MIT License
*
* Copyright (c) 2025 Curtis McCoy
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

#ifndef WASP_SHADER_H_
#define WASP_SHADER_H_

#include "types.h"
#include "slice.h"
#include "instance_attributes.h"

typedef struct _opaque_Material_t* Material;

typedef struct _opaque_Shader_t {
  slice_t             CONST name;
  attribute_format_t        attrib_format;
  bool                CONST ready;
}* Shader;

Shader  shader_new(slice_t name);
Shader  shader_new_load_async(slice_t name);
void    shader_file_vert(Shader, slice_t override);
void    shader_file_frag(Shader, slice_t override);
void    shader_load_async(Shader);
void    shader_build(Shader);
void    shader_build_all(void);
void    shader_bind(Shader);
void    shader_bind_attributes(Shader);
void    shader_set_uniform(Shader);
void    shader_set_material(Shader, Material);
void    shader_delete(Shader* shader);

int     shader_uniform_loc(Shader, const char* name);
int     shader_attribute_loc(Shader, const char* name);

void    shader_check_updates(void);

#endif
