#version 300 es

in vec4 position;
in vec4 color;

uniform mat4 projViewMod;

out lowp vec4 vColor;
out lowp float vDepth;

void main() {
  gl_Position = projViewMod * position;
  vColor = color;
  vDepth = gl_Position.z;
}
