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

////////////////////////////////////////////////////////////////////////////////
// PBR functions

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

////////////////////////////////////////////////////////////////////////////////
// Octahedral decode

vec3 oct_decode(vec2 n) {
  vec3 r = vec3(n, 1.0 - abs(n.x) - abs(n.y));
  if (r.z < 0.0) r.xy = (1.0 - abs(r.yx)) * sign(r.xy);
  return normalize(r);
}

////////////////////////////////////////////////////////////////////////////////
// Light indexing

struct LightPos {
  vec3 pos;
  float radius;
};

struct LightSpot {
  vec3 dir;
  float outer; // cosine against dir
  float inner; // cosine against dir
  bool use_bounds;
};

struct LightColor {
  vec3 color;
  float intensity; // todo: remove
};

struct Light {
  LightPos transform;
  LightSpot spot;
  LightColor color;
};

LightPos get_light_transform(int i) {
  vec4 T = texelFetch(lightSamp, ivec2(0, i), 0);
  return LightPos(T.xyz, T.z);
}

LightSpot get_light_spot(int i) {
  vec4 T = texelFetch(lightSamp, ivec2(1, i), 0);
  bool use = !(T.x == 0.0 && T.y == 0.0);
  if (!use || any(isnan(T))) {
    return LightSpot(vec3(0.0), 0.0, 0.0, false);
  }
  return LightSpot(oct_decode(T.xy), T.z, T.w, use);
}

LightColor get_light_color(int i) {
  vec4 T = texelFetch(lightSamp, ivec2(2, i), 0);
  return LightColor(T.xyz, T.w);
}

////////////////////////////////////////////////////////////////////////////////
// Main processing

void main() {
  // Get UV coordinates and basic color values from textures
  vec2 uv = vUV;
  vec4 albedo = texture(texSamp, uv);
  vec4 props = texture(propSamp, uv);

  // Material properties
  vec3  metallic = albedo.xyz * props.g;
  float roughness = props.r;

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
  //norm = norm * 2.0 - 1.0;
  norm = oct_decode(norm.xy * 2.0 - 1.0);

  // Pabst Blue Ribbon?
  vec3 N = norm;
  vec3 V = normalize(-pos);
  vec3 F0 = mix(vec3(0.04), albedo.xyz, metallic);
  vec3 result = vec3(0.0);

  int light_buffer_size = textureSize(lightSamp, 0).y;
  for (int i = 0; i < light_buffer_size; ++i) {
    Light light;
    light.transform = get_light_transform(i);

    vec3 light_pos = light.transform.pos;
    vec3 Lv = light_pos - pos;
    float light_dist = length(Lv);
    vec3 L = Lv / light_dist;
    float falloff = 1.0;

    light.spot = get_light_spot(i);

    // Spotlight (radius = disc)
    if (light.spot.use_bounds) {

      // calculate lighting for a disk with given radius
      float light_radius = light.transform.radius;
      if (light_radius > 0.00001) {
        float dist_to_plane = dot(-Lv, light.spot.dir);
        vec3 plane_pos = pos - light.spot.dir * dist_to_plane;
        vec3 C = plane_pos - light_pos;
        float r = length(C);
        light_pos += mix(C, C * (light_radius / r), r > light_radius);
        Lv = light_pos - pos;
        light_dist = length(Lv);
        L = Lv / light_dist;
        falloff *= max(0.0, dot(light.spot.dir, -L));
      }

      float LdotS = max(0.0, dot(-L, light.spot.dir));
      if (LdotS < light.spot.outer) {
        continue;
      }
      if (LdotS < light.spot.inner) {
        LdotS -= light.spot.outer;
        LdotS /= (light.spot.inner - light.spot.outer);
        falloff *= smoothstep(0.0, 1.0, LdotS);
      }
    }
    // Globelight (radius = sphere)
    else {
      //light_dist = max(light_dist - light.transform.radius, 0.0);
      //vec3 L = Lv / light_dist;
      //float falloff = 1.0;
    }



    float attenuation = falloff / (light_dist * light_dist);
    light.color = get_light_color(i);

    vec3 H = normalize(V + L);
    vec3 radiance = light.color.color * light.color.intensity * attenuation;

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
  //ivec2 test = ivec2(int(uv.x * 3.0), int(uv.y * float(light_buffer_size)));
  //fragColor = texelFetch(lightSamp, test, 0);
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
