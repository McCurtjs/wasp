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

void main() {
  vec4 albedo = texture(texSamp, vUV);

  if (albedo.w == 0.0) discard;

  vec4 n = normalize(vNormal);
  vec4 viewDir = normalize(cameraPos - vPos);
  vec4 lightDir = normalize(lightPos - vPos);
  vec4 reflectDir = reflect(-lightDir, n);
  float diffuse = max(dot(n, lightDir), 0.0);
  float specular = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

  fragColor = vec4(vUV, 0.0, 1.0) * (ambient + diffuse + specular);
  fragColor.xyz = albedo.xyz * (ambient + diffuse + specular);

  // multiply with transparency value because PDN uses transparent white :/
  fragColor.xyz *= albedo.w;
  fragColor.w = albedo.w;

  //fragColor = vec4(normalize(vec3(vUV, 0)) * 0.5 + vec3(0.5, 0.5, 0.5), 1);
  fragNormal = vec4(n.x, n.y, n.z, 1.0);
}
