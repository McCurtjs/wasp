
static const char shader_basic_vert[] = "\
#version 300 es\n\
layout(location = 0) in vec3 position;\n\
layout(location = 4) in vec3 color;\n\
uniform mat4 in_pvm_matrix;\n\
out highp vec3 vColor;\n\
void main() {\n\
  gl_Position = in_pvm_matrix * vec4(position, 1.0);\n\
  vColor = color;\n\
}\n";

static const char shader_basic_frag[] = "\
#version 300 es\n\
in highp vec3 vColor;\n\
layout(location = 0) out highp vec4 frag_color;\n\
layout(location = 1) out highp vec4 frag_normal;\n\
void main() {\n\
  frag_color = vec4(vColor, 1.0);\n\
  frag_normal = vec4(0.0);\n\
}\n";

static const char shader_particle_vert[] = "\
#version 300 es\n\
layout(location = 0) in vec3 position;\n\
layout(location = 1) in vec2 uv;\n\
layout(location = 5) in vec3 model_pos;\n\
layout(location = 11) in float model_scale;\n\
uniform mat4 in_pv_matrix;\n\
out highp vec2 vUV;\n\
void main() {\n\
  vec4 final = vec4(model_pos + position * model_scale, 1.0);\n\
  gl_Position = in_pv_matrix * (final);\n\
  vUV = uv;\n\
}\n";

static const char shader_particle_frag[] = "\
#version 300 es\n\
precision highp float;\n\
in vec2 vUV;\n\
layout(location = 0) out vec3 frag_color;\n\
layout(location = 1) out vec2 frag_norm;\n\
layout(location = 2) out vec4 frag_props;\n\
layout(location = 3) out float frag_depth;\n\
void main() {\n\
  frag_color = vec3(vUV.x, vUV.y, 0.0);\n\
  frag_norm = vUV;\n\
  frag_props = vec4(0.0);\n\
  frag_depth = gl_FragCoord.z;\n\
}\n";

static const char shader_quad_vert[] = "\
#version 300 es\n\
precision highp float;\n\
layout(location = 0) in vec4 position;\n\
out vec2 vUV;\n\
void main() {\n\
  gl_Position = position;\n\
  vUV = (position.xy + vec2(1, 1)) / 2.0;\n\
}\n";

static const char shader_quad_frag[] = "\
#version 300 es\n\
precision highp float;\n\
uniform sampler2D samp_frame;\n\
in vec2 vUV;\n\
layout(location = 0) out vec4 frag_color;\n\
void main() {\n\
  frag_color = vec4(texture(samp_frame, vUV).xyz, 1.0);\n\
}\n";
