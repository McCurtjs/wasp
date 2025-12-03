#include "camera.h"

#include <math.h>

static void camera_basis(vec3 out[3], vec3 back, vec3 up) {
  out[2] = v3norm(back); // back
  out[0] = v3norm(v3cross(up, out[2])); // right
  out[1] = v3cross(out[2], out[0]); // up
}

static mat4 camera_look(vec3 pos, vec3 target, vec3 up) {
  vec3 c[3];
  camera_basis(c, v3sub(pos, target), up);

  return m4basis(c[0], c[1], c[2], pos);
}

void camera_build_perspective(Camera* camera) {
  Camera_PerspectiveParams params = camera->persp;
  camera->projection = m4perspective(
    params.fov, params.aspect, params.near, params.far);
}

void camera_build_orthographic(Camera* camera) {
  Camera_OrthographicParams params = camera->ortho;
  camera->projection = m4ortho(params.left, params.right, params.top,
                               params.bottom, params.near, params.far);
}

void camera_rotate(Camera* camera, vec2 euler) {
  vec3 c[3];
  camera_basis(c, v3neg(camera->front.xyz), camera->up.xyz);
  mat4 transform = m4rotation(c[0], euler.x);
  transform = m4mul(transform, m4rotation(camera->up.xyz, euler.y));

  camera->front = mv4mul(transform, camera->front);
}

void camera_rotate_local(Camera* camera, vec3 euler) {
  vec3 c[3];
  camera_basis(c, v3neg(camera->front.xyz), camera->up.xyz);

  mat4 transform = m4rotation(c[0], euler.x);
  transform = m4mul(transform, m4rotation(c[1], euler.y));
  transform = m4mul(transform, m4rotation(c[2], euler.z));

  camera->front = mv4mul(transform, camera->front);
  camera->up = mv4mul(transform, camera->up);
}

void camera_orbit(Camera* camera, vec3 center, vec2 euler) {
  vec4 center2pos = v4zero;
  center2pos.xyz = v3sub(camera->pos.xyz, center);
  vec3 c[3];
  camera_basis(c, v3neg(camera->front.xyz), camera->up.xyz);

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

void camera_orbit_local(Camera* camera, vec3 center, vec3 euler) {
  vec4 center2pos = v4zero;
  center2pos.xyz = v3sub(camera->pos.xyz, center);
  vec3 c[3];
  camera_basis(c, v3neg(camera->front.xyz), camera->up.xyz);

  mat4 transform = m4rotation(c[0], euler.x);
  transform = m4mul(transform, m4rotation(c[1], euler.y));
  transform = m4mul(transform, m4rotation(c[2], euler.z));

  center2pos = mv4mul(transform, center2pos);
  camera->pos.xyz = v3add(center, center2pos.xyz);
  camera->front = mv4mul(transform, camera->front);
  camera->up = mv4mul(transform, camera->up);
}

void camera_look_at(Camera* camera, vec3 target) {
  camera->front.xyz = v3norm(v3sub(target, camera->pos.xyz));
}

mat4 camera_view(const Camera* camera) {
  vec3 target = v3add(camera->pos.xyz, camera->front.xyz);
  return camera_look(camera->pos.xyz, target, camera->up.xyz);
}

mat4 camera_projection_view(const Camera* camera) {
  return m4mul(camera->projection, camera_view(camera));
}

// Gets a vector from the camera pos in the direction of the cursor
vec3 camera_ray(const Camera* camera, vec2i scr_wh, vec2 ndc_pos) {
  // find a point 1 unit away on a plane defined by our field of view
  float half_field_of_view = camera->persp.fov / 2;
  float screen_plane_halfwidth = tanf(half_field_of_view);
  return v3f(
    screen_plane_halfwidth * i2aspect(scr_wh) * ndc_pos.x,
    screen_plane_halfwidth * ndc_pos.y,
    -1
  );
}

// convert screen space from [0, w] and [0, 1-h] to [-1, 1]
vec2 camera_screen_to_ndc(vec2i scr_wh, vec2 screen_pos) {
  return (vec2) {
    .x =      (screen_pos.x / (float)scr_wh.w - 0.5f) * 2,
    .y = (1 - (screen_pos.y / (float)scr_wh.h) - 0.5f) * 2,
  };
}

vec3 camera_screen_to_ray(const Camera* camera, vec2i window, vec2 screen_pos) {
  vec2 ndc = camera_screen_to_ndc(window, screen_pos);
  return camera_ray(camera, window, ndc);
}
