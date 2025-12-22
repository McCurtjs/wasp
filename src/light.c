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

#include "light.h"

#define con_type light_t
#define con_prefix light
#include "span.h"
#include "array.h"
#undef con_type
#undef con_prefix

array_light_t _all_lights = {
  .element_size = sizeof(light_t)
};
index_t _light_index = 0;

index_t light_add(light_t light) {
  light.id = (uint)++_light_index;
  arr_light_push_back(&_all_lights, light);
  return light.id;
}

light_t* light_ref(index_t id) {
  light_t* arr_foreach(light, &_all_lights) {
    if (light->id == (uint)id) {
      return light;
    }
  }
  return NULL;
}

void light_remove(index_t id) {
  light_t* arr_foreach_index(light, index, &_all_lights) {
    if (light->id == (uint)id) {
      arr_light_remove_unstable(&_all_lights, index);
    }
  }
}

void light_clear(void) {
  arr_light_clear(&_all_lights);
}

index_t light_count(void) {
  return _all_lights.size;
}

light_t* light_buffer(void) {
  return _all_lights.begin;
}
