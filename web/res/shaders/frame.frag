#version 300 es
precision highp float;

in lowp vec2 vUV;

out lowp vec4 fragColor;

uniform mat4 in_proj_inverse;
uniform vec4 lightPos;

uniform sampler2D texSamp;
uniform sampler2D normSamp;
uniform sampler2D propSamp;
uniform sampler2D depthSamp;
uniform sampler2D lightSamp;

#define PI 3.141592653589

float D_GGX(float NdotH, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
  return a2 / (PI * denom * denom);
}

vec3 F_Schlick(float VdotH, vec3 F0) {
  return F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
}

float G_SchlickGGX(float NdotV, float roughness) {
  float r = roughness + 1.0;
  float k = (r * r) / 8.0;
  return NdotV / (NdotV * (1.0 - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness) {
  return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

void main() {
  // Get UV coordinates and basic color values from textures
  vec2 uv = vUV;
  vec4 albedo = texture(texSamp, uv);
  vec4 props = texture(propSamp, uv);

  // Light properties
  vec3  light_color = vec3(0.9, 0.9, 0.75);
  float light_intensity = 6.0;

  vec2 light_index = vec2(3.0 / 3.0, 0);
  light_intensity = texelFetch(lightSamp, ivec2(3, 0), 0).z;
  vec3 light_pos = texelFetch(lightSamp, ivec2(1, 0), 0).xyz;//texture(lightSamp, light_index).xyz;
  light_color = texelFetch(lightSamp, ivec2(3, 0), 0).xyz;//texture(lightSamp, light_index).xyz;
  fragColor = vec4(vec3(light_color), 1.0);
  //return;

  vec3 light_value = light_color * light_intensity;

  // Material properties
  vec3  metallic = albedo.xyz * props.g;
  float roughness = 1.0 - props.r;

  // Reconstruct fragment position in view space (camera at <0, 0, 0>)
  vec3 ndc = vec3(uv.x, uv.y, texture(depthSamp, uv).r);
  vec4 clip = vec4(ndc * 2.0 - 1.0, 1.0);
  vec4 view = in_proj_inverse * clip;
  view /= view.w;
  vec3 pos = view.xyz;

  // Unpack octahedral encoded normal
  vec3 norm = texture(normSamp, uv).xyz;
  if (norm.x == norm.y && norm.y == 0.0) {
    fragColor = albedo;
    return;
  }
  norm = norm * 2.0 - 1.0;
  norm = vec3(norm.xy, 1.0 - abs(norm.x) - abs(norm.y));
  if (norm.z < 0.0) norm.xy = (1.0 - abs(norm.yx)) * sign(norm.xy);

  // Pabst Blue Ribbon?
  vec3 N = normalize(norm);
  vec3 V = normalize(-pos);
  vec3 F0 = mix(vec3(0.04), albedo.xyz, metallic);
  vec3 result = vec3(0.0);

  ivec2 light_buffer_size = textureSize(lightSamp, 0);
  for (int i = 0; i < light_buffer_size.x; i += 3) {
    light_intensity = texelFetch(lightSamp, ivec2(i, 0), 0).z;
    light_pos = texelFetch(lightSamp, ivec2(i+1, 0), 0).xyz;
    light_color = texelFetch(lightSamp, ivec2(i+2, 0), 0).xyz;

    vec3 L = light_pos - pos;
    float light_dist = length(L);
    float attenuation = 1.0 / (light_dist * light_dist);
    L /= light_dist;
    vec3 H = normalize(V + L);
    vec3 radiance = light_color * light_intensity * attenuation;

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    float D = D_GGX(NdotH, roughness);
    float G = G_Smith(NdotV, NdotL, roughness);
    vec3  F = F_Schlick(VdotH, F0);

    vec3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 1e-4);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    vec3 diffuse = kD * albedo.xyz / PI;

    result += vec3((diffuse + specular) * radiance * NdotL);
  }

  // Gamma correction
  //vec3 ambient = vec3(0.00);// * albedo.xyz;
  //vec3 color = ambient + result;
  //color = color / (color + vec3(1.0));
  //color = pow(color, vec3(1.0/2.2));
  //result = color;

  //fragColor = vec4(vec3((diffuse + specular) * light_value * NdotL), 1.0);
  fragColor = vec4(result, 1.0);
  //fragColor = vec4(vec3(NdotV), 1.0);
  //fragColor = vec4(normalize(pos - light_pos.xyz), 1.0);
  //fragColor = vec4(vec3(norm), 1.0);
  //fragColor = vec4(vec3(roughness), 1.0);

  // ambient color (IBL approx)
  //vec3 irradiance_map = vec3(0.2, 0.1, 0.2);
  //vec3 ambient_diffuse = irradiance_map * albedo.xyz * kD;
  //vec3 ambient_specular = env * (F * brdf.x + brdf.y);
  //fragColor.xyz += ambient_diffuse;// + ambient_specular;
}
