#version 300 es
precision highp float;

#define ambient 0.5

in vec4 vPos;
in vec4 vNormal;
in vec2 vUV;

uniform vec4 lightPos;
uniform vec4 cameraPos;
uniform sampler2D texSamp;
float specularPower = 0.35;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragNormal;
//layout(location = 2) out float depthValue;

void main() {

  // Get base texture color - discard fully transparent fragments
  vec4 albedo = texture(texSamp, vUV);
  if (albedo.w == 0.0) discard;
  fragColor = albedo;

  // If the normal is zero, preserve that and skip (unlit/non-physical object)
  if (vNormal.z == 0.0) {
    fragNormal = vec4(0.0);
    return;
  }

  // Pack normal into octahedral space
  vec4 n = normalize(vNormal);
  n /= abs(n.x) + abs(n.y) + abs(n.z);
  if (n.z < 0.0) n.xy = (1.0 - abs(n.yx)) * sign(n.xy);
  fragNormal = n * 0.5 + 0.5;
}
