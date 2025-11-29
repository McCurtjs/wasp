#version 300 es
precision highp float;
in lowp vec2 vUV;
out lowp vec4 fragColor;
uniform sampler2D texSamp;
uniform sampler2D normSamp;
void main() {
  vec2 uv = vUV;
  if (uv.x < 0.5 && uv.y < 0.5) {
    uv *= 2.0;
    fragColor = texture(normSamp, uv);
  }
  else if (uv.x < 0.5 && uv.y >= 0.5) {
    uv *= 2.0;
    uv.y -= 1.0;
    fragColor = texture(texSamp, uv);
  }
  else {
    fragColor = vec4(uv.x, uv.y, 0.0, 1.0);
  }
}
