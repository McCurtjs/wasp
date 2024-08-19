#ifndef _WASP_CAMERA_H_
#define _WASP_CAMERA_H_

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

} Camera;

void camera_build_perspective(Camera* camera);
void camera_build_orthographic(Camera* camera);
void camera_rotate(Camera* camera, vec2 euler);
void camera_rotate_local(Camera* camera, vec3 euler);
void camera_orbit(Camera* camera, vec3 center, vec2 euler);
void camera_orbit_local(Camera* camera, vec3 center, vec3 euler);
void camera_look_at(Camera* camera, vec3 target);
mat4 camera_projection_view(const Camera* camera);
vec2 camera_screen_to_ndc(vec2i screen, vec2 screen_pos);
vec3 camera_screen_to_ray(const Camera* camera, vec2i scr, vec2 screen_pos);
vec3 camera_ray(const Camera* camera, vec2i scr_wh, vec2 ndc_pos);

#endif
