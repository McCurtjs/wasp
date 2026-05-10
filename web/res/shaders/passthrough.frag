#version 300 es
precision highp float;
uniform sampler2D samp_frame;
in vec2 vUV;
out vec4 frag_color;
void main() {
  frag_color = vec4(texture(samp_frame, vUV).xyz, 1.0);
}
