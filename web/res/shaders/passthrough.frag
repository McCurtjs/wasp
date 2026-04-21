#version 300 es
precision highp float;

in vec2 vUV;
out vec4 frag_color;
uniform sampler2D samp_frame;

void main() {
  frag_color = vec4(texture(samp_frame, vUV).xyz, 1.0);
}
