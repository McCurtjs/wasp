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

#ifndef WASP_CAMERA_H_
#define WASP_CAMERA_H_

#include "mat.h"

typedef struct Camera_PerspectiveParams {
  float fov, aspect, near, far;
} Camera_PerspectiveParams;

typedef struct Camera_OrthographicParams {
  float left, right, top, bottom, near, far;
} Camera_OrthographicParams;

typedef struct Camera {
  vec4 pos;
  vec4 front;
  vec4 up;

  union {
    float params[6];
    Camera_PerspectiveParams persp;
    Camera_OrthographicParams ortho;
  };

  mat4 projection;
  mat4 projview;
  mat4 view;

} Camera;

void camera_build_perspective(Camera* camera);
void camera_build_orthographic(Camera* camera);
void camera_rotate(Camera* camera, vec2 euler);
void camera_rotate_local(Camera* camera, vec3 euler);
void camera_orbit(Camera* camera, vec3 center, vec2 euler);
void camera_orbit_local(Camera* camera, vec3 center, vec3 euler);
void camera_look_at(Camera* camera, vec3 target);
mat4 camera_view(const Camera* camera);
mat4 camera_projection_view(const Camera* camera);
vec2 camera_screen_to_ndc(vec2i screen, vec2 screen_pos);
vec3 camera_screen_to_ray(const Camera* camera, vec2i scr, vec2 screen_pos);
vec3 camera_ray(const Camera* camera, vec2i scr_wh, vec2 ndc_pos);

#endif
