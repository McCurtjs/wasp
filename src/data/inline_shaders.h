
static const char basic_frag_text[] = "\
#version 300 es\n\
in lowp vec4 vColor;\n\
out lowp vec4 fragColor;\n\
void main() {\n\
  fragColor = vColor;\n\
}";

static const char basic_vert_text[] = "\
#version 300 es\n\
layout(location = 0) in vec4 position;\n\
layout(location = 1) in vec4 color;\n\
uniform mat4 projViewMod;\n\
out lowp vec4 vColor;\n\
void main() {\n\
  gl_Position = projViewMod * position;\n\
  vColor = color;\n\
}";
