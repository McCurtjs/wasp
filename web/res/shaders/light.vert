#version 300 es
precision highp float;

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;
layout(location = 2) in vec2 uv;

uniform mat4 projViewMod;
uniform mat4 itViewMod;

out vec4 vPos;
out vec4 vNormal;
out vec2 vUV;

void main() {
  vPos = projViewMod * position;
  vUV = uv;
  vNormal = itViewMod * vec4(normal.xyz, 0);

  gl_Position = projViewMod * position;
}
