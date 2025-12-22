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

#ifndef WASP_DEBUG_DRAW_H_
#define WASP_DEBUG_DRAW_H_

#include "types.h"
#include "vec.h"

typedef struct DebugDrawState {
  color4 color;
  color4 color_gradient_start;
  bool   use_gradient;
  vec3   vector_offset;
  float  scale;
} DebugDrawState;
extern DebugDrawState draw;

void draw_push();
void draw_pop();
void draw_default_state();

void draw_point(vec3 p);
void draw_circle(vec3 p, float radius);
void draw_line(vec3 a, vec3 b);
void draw_line_solid(vec3 a, vec3 b, color4 color);
void draw_vector(vec3 v);
void draw_dir(vec3 v);
void draw_rect(vec3 pos, vec3 a, vec3 b);

void draw_render();

void draw_cleanup();

#endif
