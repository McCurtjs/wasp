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

#include "camera.h"

#include <math.h>

////////////////////////////////////////////////////////////////////////////////

static void _camera_basis(vec3 out[3], vec3 back, vec3 up) {
  out[2] = v3norm(back); // back
  out[0] = v3norm(v3cross(up, out[2])); // right
  out[1] = v3cross(out[2], out[0]); // up
}

////////////////////////////////////////////////////////////////////////////////

static mat4 _camera_look(vec3 pos, vec3 target, vec3 up) {
  vec3 c[3];
  _camera_basis(c, v3sub(pos, target), up);

  return m4basis(c[0], c[1], c[2], pos);
}

////////////////////////////////////////////////////////////////////////////////
// Update the camera's projection matrix
////////////////////////////////////////////////////////////////////////////////

void camera_build(camera_t* camera) {
  if (camera->type == CAMERA_ORTHOGRAPHIC) {
    Camera_Orthographic params = camera->orthographic;
    camera->projection = m4orthographic(
      params.left, params.right,
      params.top, params.bottom,
      params.near, params.far
    );
  }
  else {
    Camera_Perspective params = camera->perspective;
    camera->projection = m4perspective(
      params.fov, params.aspect, params.near, params.far
    );
  }
}

////////////////////////////////////////////////////////////////////////////////
// Rotate the camera using euler angles (around x and y axes)
////////////////////////////////////////////////////////////////////////////////

void camera_rotate(camera_t* camera, vec2 euler) {
  vec3 c[3];
  _camera_basis(c, v3neg(camera->front.xyz), camera->up.xyz);
  mat4 transform = m4rotation(c[0], euler.x);
  transform = m4mul(transform, m4rotation(camera->up.xyz, euler.y));

  camera->front = mv4mul(transform, camera->front);
}

////////////////////////////////////////////////////////////////////////////////
// Rotate the camera around three axes
////////////////////////////////////////////////////////////////////////////////

void camera_rotate_local(camera_t* camera, vec3 euler) {
  vec3 c[3];
  _camera_basis(c, v3neg(camera->front.xyz), camera->up.xyz);

  mat4 transform = m4rotation(c[0], euler.x);
  transform = m4mul(transform, m4rotation(c[1], euler.y));
  transform = m4mul(transform, m4rotation(c[2], euler.z));

  camera->front = mv4mul(transform, camera->front);
  camera->up = mv4mul(transform, camera->up);
}

////////////////////////////////////////////////////////////////////////////////
// Orbit the camera around a center point
////////////////////////////////////////////////////////////////////////////////

void camera_orbit(camera_t* camera, vec3 center, vec2 euler) {
  vec4 center2pos = v4zero;
  center2pos.xyz = v3sub(camera->pos.xyz, center);
  vec3 c[3];
  _camera_basis(c, v3neg(camera->front.xyz), camera->up.xyz);

  //float t = fabs(v3angle(camera->front.xyz, camera->up.xyz) - PI/2);
  //if (t > PI/2 - 0.05) {
  //  print_float(t);
  //}

  mat4 transform = m4identity;
  transform = m4mul(transform, m4rotation(c[0], euler.x));
  transform = m4mul(transform, m4rotation(camera->up.xyz, euler.y));

  center2pos = mv4mul(transform, center2pos);
  camera->pos.xyz = v3add(center, center2pos.xyz);
  camera->front = mv4mul(transform, camera->front);
}

////////////////////////////////////////////////////////////////////////////////
// Orbit the camera around a center point using all 3 axes
////////////////////////////////////////////////////////////////////////////////

void camera_orbit_local(camera_t* camera, vec3 center, vec3 euler) {
  vec4 center2pos = v4zero;
  center2pos.xyz = v3sub(camera->pos.xyz, center);
  vec3 c[3];
  _camera_basis(c, v3neg(camera->front.xyz), camera->up.xyz);

  mat4 transform = m4rotation(c[0], euler.x);
  transform = m4mul(transform, m4rotation(c[1], euler.y));
  transform = m4mul(transform, m4rotation(c[2], euler.z));

  center2pos = mv4mul(transform, center2pos);
  camera->pos.xyz = v3add(center, center2pos.xyz);
  camera->front = mv4mul(transform, camera->front);
  camera->up = mv4mul(transform, camera->up);
}

////////////////////////////////////////////////////////////////////////////////
// Turns the camera to look at a given target
////////////////////////////////////////////////////////////////////////////////

void camera_look_at(camera_t* camera, vec3 target) {
  camera->front.xyz = v3norm(v3sub(target, camera->pos.xyz));
}

////////////////////////////////////////////////////////////////////////////////
// Calculate the view matrix
////////////////////////////////////////////////////////////////////////////////

mat4 camera_view(const camera_t* camera) {
  vec3 target = v3add(camera->pos.xyz, camera->front.xyz);
  return _camera_look(camera->pos.xyz, target, camera->up.xyz);
}

////////////////////////////////////////////////////////////////////////////////
// Calculate the [projection x view] matrix
////////////////////////////////////////////////////////////////////////////////

mat4 camera_projection_view(const camera_t* camera) {
  return m4mul(camera->projection, camera_view(camera));
}

////////////////////////////////////////////////////////////////////////////////
// Gets a vector from the camera position in the direction of the cursor
////////////////////////////////////////////////////////////////////////////////

vec3 camera_ray(const camera_t* camera, vec2i scr_wh, vec2 ndc_pos) {
  // find a point 1 unit away on a plane defined by our field of view
  float half_field_of_view = camera->perspective.fov / 2;
  float screen_plane_halfwidth = tanf(half_field_of_view);
  vec3 vec = v3f(
    screen_plane_halfwidth * i2aspect(scr_wh) * ndc_pos.x,
    screen_plane_halfwidth * ndc_pos.y,
    -1
  );
  return mv3mul(m3transpose(m43(camera->view)), vec);
}

////////////////////////////////////////////////////////////////////////////////
// Convert a screen coordinate from [0, w] and [0, 1-h] to [-1, 1]
////////////////////////////////////////////////////////////////////////////////

vec2 camera_screen_to_ndc(vec2i scr_wh, vec2 screen_pos) {
  return (vec2) {
    .x =      (screen_pos.x / (float)scr_wh.w - 0.5f) * 2,
    .y = (1 - (screen_pos.y / (float)scr_wh.h) - 0.5f) * 2,
  };
}

////////////////////////////////////////////////////////////////////////////////
// Project a ray from the camera into the screen based on a pixel coordinate
////////////////////////////////////////////////////////////////////////////////

vec3 camera_screen_to_ray(const camera_t* camera, vec2i window, vec2 screen_pos) {
  vec2 ndc = camera_screen_to_ndc(window, screen_pos);
  return v3norm(camera_ray(camera, window, ndc));
}
