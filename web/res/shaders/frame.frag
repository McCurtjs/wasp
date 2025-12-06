#version 300 es
precision highp float;

in lowp vec2 vUV;

out lowp vec4 fragColor;

uniform mat4 invProj;
uniform sampler2D texSamp;
uniform sampler2D normSamp;
uniform sampler2D depthSamp;

void main() {
  vec2 uv = vUV;

  vec4 ndc;
  ndc.xy = uv * 2.0 - 1.0;
  ndc.z = texture(depthSamp, uv).r * 2.0 - 1.0;
  ndc.w = 1.0;
  vec4 pos = inverse(invProj) * ndc;
  pos.xyz /= pos.w;

  vec4 norm = texture(normSamp, uv) * 2.0 - 1.0;
  vec4 albedo = texture(texSamp, uv);
  
  fragColor = albedo + norm + pos;//vec4(d, d, d, 1.0); //vec4(uv.x, uv.y, 0.0, 1.0);
}
