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

#ifndef WASP_LIGHT_H_
#define WASP_LIGHT_H_

#include "types.h"
#include "vec.h"
#include "slotkey.h"

#pragma pack(1)
typedef struct light_t {
  uint  id;
  uint  _unused;
  float intensity;
  vec3  pos;
  vec3  color;
} light_t;
#pragma pack()

#define con_type light_t
#define con_prefix light
#include "view.h"
#undef con_prefix
#undef con_type

slotkey_t     light_add(light_t light);
light_t*      light_ref(slotkey_t id);
void          light_remove(slotkey_t id);
void          light_clear(void);

view_light_t  light_buffer(void);

#ifdef WASP_TEXTURE_H_
#include "texture.h"
#endif

#endif
