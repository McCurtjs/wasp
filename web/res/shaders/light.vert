#version 300 es
precision highp float;

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;
layout(location = 2) in vec2 uv;

uniform mat4 in_pvm_matrix;
uniform mat4 in_normal_matrix;

out vec4 vPos;
out vec4 vNormal;
out vec2 vUV;
out float vDepth;

void main() {
  vPos = in_pvm_matrix * position;
  vUV = uv;
  vNormal = in_normal_matrix * vec4(normal.xyz, 0);
  gl_Position = vPos;
  //vDepth = length((viewMod * position).xyz);
}
