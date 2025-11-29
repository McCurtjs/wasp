#version 300 es

layout(location = 0) in vec4 position;

out vec2 vUV;

void main() {
  gl_Position = position;
  vUV = (position.xy + vec2(1, 1)) / 2.0f;
}
