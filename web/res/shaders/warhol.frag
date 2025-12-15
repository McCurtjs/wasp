#version 300 es
precision highp float;

in lowp vec2 vUV;

out lowp vec4 fragColor;

uniform mat4 in_proj_inverse;

uniform sampler2D texSamp;
uniform sampler2D normSamp;
uniform sampler2D depthSamp;

void main() {
  vec2 uv = vUV;

  // Bottom Left
  if (uv.x <= 0.5 && uv.y <= 0.5) {
    uv *= 2.0;
    fragColor = texture(normSamp, uv);
  }

  // Top Left
  else if (uv.x <= 0.5 && uv.y > 0.5) {
    uv *= 2.0;
    uv.y -= 1.0;
    fragColor = texture(texSamp, uv);
  }

  // Top Right
  else if (uv.x > 0.5 && uv.y > 0.5) {
    uv *= 2.0;
    uv.y -= 1.0;
    uv.x -= 1.0;
    vec3 ndc = vec3(uv.x, uv.y, texture(depthSamp, uv).r);
    vec4 clip = vec4(ndc * 2.0 - 1.0, 1.0);
    vec4 view = in_proj_inverse * clip;
    view /= view.w;
    vec3 pos = view.xyz;
    fragColor = vec4(vec3(length(pos)/50.0), 1.0); //vec4(uv.x, uv.y, 0.0, 1.0);
  }

  // Bottom Right
  else if (uv.x > 0.5 && uv.y <= 0.5 ) {
    uv *= 2.0;
    uv.x -= 1.0;
    vec4 ndc;
    ndc.xy = uv * 2.0 - 1.0;
    ndc.z = texture(depthSamp, uv).r * 2.0 - 1.0;
    ndc.w = 1.0;
    vec4 view = in_proj_inverse * ndc;
    view.xyz /= view.w;
    view.w = 1.0;
    view = fract(view);
    fragColor = vec4(ndc.xy * 2.0 - 1.0, ndc.z, 1.0);//view;//vec4(d, d, d, 1.0); //vec4(uv.x, uv.y, 0.0, 1.0);
  }

  // Cross?
  else {
    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
  }
}
