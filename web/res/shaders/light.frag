#version 300 es
precision highp float;

#define ambient 0.5

in vec4 vPos;
in vec2 vUV;
in mat3 vTangentTransform;

uniform vec4 lightPos;
uniform vec4 cameraPos;

uniform sampler2D texSamp;
uniform sampler2D normSamp;
uniform sampler2D specSamp;

uniform vec2 in_props;
uniform vec3 in_tint;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragNormal;
layout(location = 2) out vec4 fragProps;

void main() {

  // Get base texture color - discard fully transparent fragments
  vec4 albedo = texture(texSamp, vUV);
  if (albedo.w == 0.0) discard;
  fragColor = albedo.xyz * in_tint;

  // If the normal is zero, preserve that and skip (unlit/non-physical object)
  //if (n.z == 0.0) {//vNormal.z == 0.0) {
  //  fragNormal = vec2(0.0);
  //  return;
  //}

  // props.r = roughness component
  // props.g = metalness component
  fragProps.rg = texture(specSamp, vUV).rg * in_props.rg;

  // Pack normal into octahedral space
  //mat3 tangent_transform = mat3(
  //  normalize(vTangentTransform[0]),
  //  normalize(vTangentTransform[1]),
  //  normalize(vTangentTransform[2])
  //);
  vec3 tangent_normal = texture(normSamp, vUV).xyz * 2.0 - 1.0;
  vec3 n = normalize(vTangentTransform * tangent_normal);
  n /= abs(n.x) + abs(n.y) + abs(n.z);
  if (n.z < 0.0) n.xy = (1.0 - abs(n.yx)) * sign(n.xy);
  fragNormal = n.xy * 0.5 + 0.5;
}
