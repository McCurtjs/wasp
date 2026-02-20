
static const char basic_vert_text[] = "\
#version 300 es\n\
layout(location = 0) in vec3 position;\n\
layout(location = 4) in vec3 color;\n\
uniform mat4 in_pvm_matrix;\n\
out lowp vec3 vColor;\n\
void main() {\n\
  gl_Position = in_pvm_matrix * vec4(position, 1.0);\n\
  vColor = color;\n\
}";

static const char basic_frag_text[] = "\
#version 300 es\n\
in lowp vec3 vColor;\n\
layout(location = 0) out lowp vec4 fragColor;\n\
layout(location = 1) out lowp vec4 fragNormal;\n\
void main() {\n\
  fragColor = vec4(vColor, 1.0);\n\
  fragNormal = vec4(0.0);\n\
}";

static const char frame_vert_text[] = "\
#version 300 es\n\
layout(location = 0) in vec4 position;\n\
out vec2 vUV;\n\
void main() {\n\
  gl_Position = position;\n\
  vUV = (position.xy + vec2(1, 1)) / 2.0f;\n\
}";

static const char frame_frag_text[] = "\
#version 300 es\n\
in lowp vec2 vUV;\n\
out lowp vec4 fragColor;\n\
uniform sampler2D texSamp;\n\
void main() {\n\
  fragColor = texture(texSamp, vUV);\n\
}";
