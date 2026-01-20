#version 300 es
precision highp float;

in vec4 vColor;
in float vDepth;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_norm;
layout(location = 2) out float frag_depth;

void main() {
  frag_color = vColor;
  frag_norm = vec4(0.0);
  frag_depth = vDepth;
}
