#version 300 es
precision highp float;

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 tangent;

uniform mat4 in_pvm_matrix;
uniform mat4 in_normal_matrix;

out vec4 vPos;
out vec4 vNormal;
out vec2 vUV;

out mat3 vTangentTransform;

void main() {
  vPos = in_pvm_matrix * position;
  gl_Position = vPos;
  vUV = uv;

  // Calculate tangent space transform
  vec3 tbn_normal = normalize(mat3(in_normal_matrix) * normal.xyz);
  vec3 tbn_tangent = normalize(mat3(in_normal_matrix) * tangent.xyz);
  vec3 tbn_bitangent = cross(tbn_normal, tbn_tangent) * tangent.w;
  vTangentTransform = mat3(tbn_tangent, tbn_bitangent, tbn_normal);
}
