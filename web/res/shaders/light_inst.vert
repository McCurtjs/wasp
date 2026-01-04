#version 300 es
precision highp float;

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 tangent;
layout(location = 4) in vec3 color;

layout(location = 5) in mat4 model;
layout(location = 9) in vec3 tint;

uniform mat4 in_pv_matrix;
uniform mat4 in_view_matrix;

out vec4 vNormal;
out vec2 vUV;
out vec3 vColor;
out mat3 vTangentTransform;

void main() {
  mat4 pvm = in_pv_matrix * model;
  gl_Position = pvm * position;
  vUV = uv;
  vColor = vec3(1.0) - color;

  // Calculate tangent space transform
  mat4 normal_matrix = in_view_matrix * model;
  normal_matrix = transpose(inverse(in_view_matrix));
  vec3 tbn_normal = normalize(mat3(normal_matrix) * normal.xyz);
  vec3 tbn_tangent = normalize(mat3(normal_matrix) * tangent.xyz);
  vec3 tbn_bitangent = cross(tbn_normal, tbn_tangent) * tangent.w;
  vTangentTransform = mat3(tbn_tangent, tbn_bitangent, tbn_normal);
}
