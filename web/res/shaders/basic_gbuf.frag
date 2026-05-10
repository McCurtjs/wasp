#version 300 es
precision highp float;

in vec3 vColor;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_norm;
layout(location = 2) out vec4 frag_props;
layout(location = 3) out float frag_depth;

void main() {
  frag_color = vColor;
  frag_norm = vec2(0.0);
  frag_props = vec4(0.0);
  frag_depth = gl_FragCoord.z;
}
