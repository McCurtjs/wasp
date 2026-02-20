#version 300 es

layout(location = 0) in vec3 position;
layout(location = 4) in vec3 color;

uniform mat4 in_pvm_matrix;

out lowp vec3 vColor;
out lowp float vDepth;

void main() {
  gl_Position = in_pvm_matrix * vec4(position, 1.0);
  vColor = color;
  vDepth = gl_Position.z;
}
