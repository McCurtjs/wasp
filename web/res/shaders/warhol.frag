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

  // Bottom Left
  if (false && uv.x < 0.5 && uv.y < 0.5) {
    uv *= 2.0;
    fragColor = texture(normSamp, uv);
  }

  // Top Left
  else if (false && uv.x < 0.5 && uv.y > 0.5) {
    uv *= 2.0;
    uv.y -= 1.0;
    fragColor = texture(texSamp, uv);
  }

  // Top Right
  else if (false && uv.x > 0.5 && uv.y > 0.5) {
    uv *= 2.0;
    uv.y -= 1.0;
    uv.x -= 1.0;
    float d = texture(depthSamp, uv).r / 50.0;
    fragColor = vec4(d, d, d, 1.0); //vec4(uv.x, uv.y, 0.0, 1.0);
  }

  // Bottom Right
  else if (true && uv.x > 0.5 && uv.y < 0.5 ) {
    uv *= 2.0;
    uv.x -= 1.0;
    vec4 ndc;
    ndc.xy = uv * 2.0 - 1.0;
    ndc.z = texture(depthSamp, uv).r * 2.0 - 1.0;
    ndc.w = 1.0;
    vec4 view = inverse(invProj) * ndc;
    view.xyz /= view.w;
    view.w = 1.0;
    view = fract(view);
    fragColor = view;//vec4(d, d, d, 1.0); //vec4(uv.x, uv.y, 0.0, 1.0);
  }

  // Cross?
  else {
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
}
