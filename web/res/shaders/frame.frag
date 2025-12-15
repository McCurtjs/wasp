#version 300 es
precision highp float;

in lowp vec2 vUV;

out lowp vec4 fragColor;

uniform mat4 in_proj_inverse;
uniform vec4 lightPos;

uniform sampler2D texSamp;
uniform sampler2D normSamp;
uniform sampler2D depthSamp;

void main() {
  vec2 uv = vUV;
  vec4 albedo = texture(texSamp, uv);

  // reconstruct the fragment position in view space (world scale, camera at <0, 0, 0>)
  vec3 ndc = vec3(uv.x, uv.y, texture(depthSamp, uv).r);
  vec4 clip = vec4(ndc * 2.0 - 1.0, 1.0);
  vec4 view = in_proj_inverse * clip;
  view /= view.w;
  vec3 pos = view.xyz;

  // get view-space normals from G-buffer
  vec3 norm = normalize(texture(normSamp, uv).xyz * 2.0 - 1.0);

  vec3 light_dir = normalize(lightPos.xyz - pos.xyz);
  vec3 view_dir = normalize(-pos);
  float diffuse = max(dot(norm, light_dir), 0.0);
  
  //*
  float vdotn = dot(norm, light_dir);
  fragColor = vec4(vec3(vdotn), 1.0);//vec4(diffuse, diffuse, diffuse, 1.0);
  /*/
  //fragColor = vec4(norm.xyz, 1.0);
  fragColor = vec4(pos.x, pos.y, (pos.z > 0.0 ? 1.0 : 0.0), 1.0);
  //*/

  //vec4 lightDist = vec4(distance(pos.xyz, lightPos.xyz) / 10.0, 0.0, 0.0, 1.0);
  
  //fragColor = vec4((lightDist.xyz), 1.0);//vec4(uv.x, uv.y, 0.0, 1.0);
}
