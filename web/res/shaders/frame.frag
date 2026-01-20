#version 300 es
precision highp float;

in lowp vec2 vUV;

out lowp vec4 frag_color;

uniform mat4 in_proj_inverse;

uniform sampler2D samp_tex;
uniform sampler2D samp_norm;
uniform sampler2D samp_prop;
uniform sampler2D samp_depth;
uniform sampler2D samp_light;

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
  float unused;
};

struct Light {
  LightPos transform;
  LightSpot spot;
  LightColor color;
};

LightPos get_light_transform(int i) {
  vec4 T = texelFetch(samp_light, ivec2(0, i), 0);
  return LightPos(T.xyz, T.w);
}

LightSpot get_light_spot(int i) {
  vec4 T = texelFetch(samp_light, ivec2(1, i), 0);
  bool use = !(T.x == 0.0 && T.y == 0.0);
  if (!use || any(isnan(T))) {
    return LightSpot(vec3(0.0), 0.0, 0.0, false);
  }
  return LightSpot(oct_decode(T.xy), T.z, T.w, use);
}

LightColor get_light_color(int i) {
  vec4 T = texelFetch(samp_light, ivec2(2, i), 0);
  return LightColor(T.xyz, T.w);
}

////////////////////////////////////////////////////////////////////////////////
// Geometric functions

vec3 snap_to_circle_3d(
  vec3 frag_pos, vec3 light_pos, vec3 light_dir, float radius, vec3 Lv
) {
  float dist_to_plane = dot(-Lv, light_dir); // light_dir must be normalized
  vec3 plane_pos = frag_pos - light_dir * dist_to_plane;
  vec3 L2P = plane_pos - light_pos;
  float r = length(L2P);
  bool is_outside_circle = r > radius;
  return light_pos + (is_outside_circle ? (L2P * (radius / r)) : L2P);
}

////////////////////////////////////////////////////////////////////////////////
// Main processing

void main() {
  // Get UV coordinates and basic color values from textures
  vec2 uv = vUV;
  vec4 albedo = texture(samp_tex, uv);
  vec4 props = texture(samp_prop, uv);

  // Material properties
  vec3  metallic = albedo.xyz * props.g;
  float roughness = props.r;

  // Reconstruct fragment position in view space (camera at <0, 0, 0>)
  vec3 ndc = vec3(uv.x, uv.y, texture(samp_depth, uv).r);
  vec4 clip = vec4(ndc * 2.0 - 1.0, 1.0);
  vec4 view = in_proj_inverse * clip;
  view /= view.w;
  vec3 frag_pos = view.xyz;

  // Unpack octahedral encoded normal
  vec3 norm = texture(samp_norm, uv).xyz;
  if (norm.x == norm.y && norm.y == 0.0) {
    frag_color = albedo;
    return;
  }
  norm = oct_decode(norm.xy * 2.0 - 1.0);

  // Set up shared values
  vec3 N = norm;
  vec3 V = normalize(-frag_pos);
  vec3 F0 = mix(vec3(0.04), albedo.xyz, metallic);
  vec3 result = vec3(0.0);

  // Run PBR lighting calaculations for each light in the scene
  int light_buffer_size = textureSize(samp_light, 0).y;
  for (int i = 0; i < light_buffer_size; ++i) {
    Light light;
    light.transform = get_light_transform(i);
    vec3 light_center = light.transform.pos;
    vec3 light_pos = light_center;

    // calculate basic lighting terms and skip if behind surface
    vec3 Lv = light_pos - frag_pos;
    if (light.transform.radius == 0.0 && dot(N, Lv) < 0.0) continue;
    float light_dist = length(Lv);
    vec3 L = Lv / light_dist;
    float falloff = 1.0;

    light.spot = get_light_spot(i);
    float light_radius = light.transform.radius;

    // Spotlight culling and falloff (radius = disc)
    if (light.spot.use_bounds) {

      // calculate lighting for a disk with given radius
      if (light_radius > 0.00001) {
        light_pos = snap_to_circle_3d(
          frag_pos, light_pos, light.spot.dir, light_radius, Lv
        );
        Lv = light_pos - frag_pos;
        light_dist = length(Lv);
        L = Lv / light_dist;
      }

      // calculate blended falloff for cone extents
      float LdotS = max(-1.0, dot(-L, light.spot.dir));
      if (LdotS < light.spot.outer) {
        continue;
      }
      if (LdotS < light.spot.inner) {
        LdotS -= light.spot.outer;
        LdotS /= (light.spot.inner - light.spot.outer);
        falloff *= smoothstep(0.0, 1.0, LdotS);
      }
    }
    // Globelight (no direction + radius = sphere)
    else {
      light_dist = max(light_dist - light_radius, 0.00001);
    }

    // calculate final attenuation component for the light
    float omega;
    if (light_radius > 0.0) {
      // solid-angle surface brightness for spherical emitters
      float d = length(light_center - frag_pos);
      float o_denom_sq = d*d + light_radius*light_radius;
      omega = 2.0 * PI * (1.0 - d / (sqrt(o_denom_sq)));
    }
    else {
      // attenuation for point lights
      omega = max(0.0, 1.0 / (light_dist*light_dist));
    }
    float attenuation = falloff * omega;

    // apply light color
    light.color = get_light_color(i);
    vec3 radiance = light.color.color * attenuation;

    // specular modifier based on emitter size
    vec3 Rdir = reflect(-V, N);
    vec3 Ps = light_pos + Rdir * light_radius;
    vec3 Ls = normalize(Ps - frag_pos);

    // halfway "normal" vector between light and view position
    vec3 H = normalize(V + Ls);

    float NdotL = max(dot(N, Ls), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    // calculate PBR terms
    float D = D_GGX(NdotH, roughness);
    float G = G_Smith(NdotV, NdotL, roughness);
    vec3  F = F_Schlick(VdotH, F0);

    // final diffuse and specular values
    vec3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 1e-4);
    vec3 kD = (1.0 - F) * (1.0 - metallic);
    vec3 diffuse = kD * albedo.xyz / PI;

    result += vec3((diffuse + specular) * radiance * NdotL);
  }

  /* // Gamma correction
  vec3 ambient = vec3(0.00);// * albedo.xyz;
  vec3 color = ambient + result;
  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0/2.2));
  result = color;
  //*/

  frag_color = vec4(result, 1.0);

  // ambient color (IBL approx) (TODO)
  //vec3 irradiance_map = vec3(0.2, 0.1, 0.2);
  //vec3 ambient_diffuse = irradiance_map * albedo.xyz * kD;
  //vec3 ambient_specular = env * (F * brdf.x + brdf.y);
  //frag_color.xyz += ambient_diffuse;// + ambient_specular;

  /* // Test render the metallicness map
  frag_color = vec4(vec3(metallic), 1.0);
  //*/

  /* // Test render the roughness map
  frag_color = vec4(vec3(roughness), 1.0);
  //*/

  /* // Test render the view-space normal map
  frag_color = vec4(norm, 1.0);
  //*/

  /* // Test render the lights texture
  ivec2 test = ivec2(int(uv.x * 3.0), int(uv.y * float(light_buffer_size)));
  frag_color = texelFetch(samp_light, test, 0);
  //*/
}
