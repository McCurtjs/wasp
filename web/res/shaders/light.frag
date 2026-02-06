#version 300 es
precision highp float;

#define ambient 0.5

in vec4 vPos;
in vec2 vUV;
in vec3 vColor;
in mat3 vTangentTransform;

uniform sampler2D samp_tex;
uniform sampler2D samp_norm;
uniform sampler2D samp_rough;
uniform sampler2D samp_metal;
uniform sampler2DArray samp_test;

uniform vec3 in_weights; // normal, roughness, metallic
uniform vec3 in_tint;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_norm;
layout(location = 2) out vec4 frag_props;

////////////////////////////////////////////////////////////////////////////////
// Octahedral encode

vec2 oct_encode(vec3 n) {
  n = normalize(n);
  n /= abs(n.x) + abs(n.y) + abs(n.z);
  if (n.z < 0.0) n.xy = (1.0 - abs(n.yx)) * sign(n.xy);
  return n.xy;
}

////////////////////////////////////////////////////////////////////////////////
// Write out to G-buffer

void main() {

  //* // Get base texture color - discard fully transparent fragments
  vec4 albedo = texture(samp_tex, vUV); /*/
  vec4 albedo = texture(samp_test, vec3(vUV, 18)); //*/

  if (albedo.w == 0.0) discard;
  frag_color = albedo.xyz * vColor;

  // Extract material properties and apply weights
  frag_props.r = texture(samp_rough, vUV).r * in_weights.g; // roughness value
  frag_props.g = texture(samp_metal, vUV).r * in_weights.b; // metallic value

  // Pack normal into octahedral space
  vec3 tangent_normal = texture(samp_norm, vUV).xyz * 2.0 - 1.0;
  tangent_normal = mix(vec3(0.0, 0.0, 1.0), tangent_normal, in_weights.r);
  vec3 n = vTangentTransform * tangent_normal;
  vec2 oct = oct_encode(n);
  frag_norm = oct * 0.5 + 0.5;
}
