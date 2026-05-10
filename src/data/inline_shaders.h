
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
layout(location = 0) out highp vec4 fragColor;\n\
layout(location = 1) out highp vec4 fragNormal;\n\
void main() {\n\
  fragColor = vec4(vColor, 1.0);\n\
  fragNormal = vec4(0.0);\n\
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
out vec4 frag_color;\n\
void main() {\n\
  frag_color = vec4(texture(samp_frame, vUV).xyz, 1.0);\n\
}\n";
