#version 300 es
precision highp float;

layout(location = 0) in vec3 position;
layout(location = 4) in vec3 color;

uniform mat4 in_pvm_matrix;

out highp vec3 vColor;

void main() {
  gl_Position = in_pvm_matrix * vec4(position, 1.0);
  vColor = color;
}
