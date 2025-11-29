#version 300 es

uniform sampler2D texSamp;

in lowp vec2 vUV;

out lowp vec4 fragColor;

void main() {
  fragColor = texture(texSamp, vUV);
}
