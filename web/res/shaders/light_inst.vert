#version 300 es
precision highp float;

layout(location = 0 ) in vec4 position;
layout(location = 1 ) in vec2 uv;
layout(location = 2 ) in vec4 normal;
layout(location = 3 ) in vec4 tangent;
layout(location = 4 ) in vec3 color; // vertex color

layout(location = 5 ) in mat4 model_matrix;
layout(location = 9 ) in vec4 model_tint; // instance color
layout(location = 10) in int  model_material_index;

uniform mat4 in_pv_matrix;
uniform mat4 in_view_matrix;

out vec4 vNormal;
out vec2 vUV;
out vec3 vTintColor;
out mat3 vTangentTransform;
flat out int vMaterialIndex;

void main() {
  mat4 pvm = in_pv_matrix * model_matrix;
  gl_Position = pvm * position;
  vUV = uv;
  vTintColor = model_tint.xyz;
  vMaterialIndex = model_material_index;

  // Calculate tangent space transform
  mat3 model_view_matrix = mat3(in_view_matrix * model_matrix);
  mat3 normal_matrix = transpose(inverse(model_view_matrix));
  vec3 tbn_normal = normalize(normal_matrix * normal.xyz);
  vec3 tbn_tangent = normalize(normal_matrix * tangent.xyz);
  vec3 tbn_bitangent = cross(tbn_normal, tbn_tangent) * tangent.w;
  vTangentTransform = mat3(tbn_tangent, tbn_bitangent, tbn_normal);
}
