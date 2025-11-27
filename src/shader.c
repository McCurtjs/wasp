#include "shader.h"

#include <string.h>
#include <stdlib.h>

#include "str.h"

#include "gl.h"
#include "wasm.h"

#include "data/inline_shaders.h"

int shader_build(Shader* s, int type, const char* buffer, uint buf_len) {
  s->handle = glCreateShader(type);
  glShaderSource(s->handle, 1, (const char**)&buffer, (const int*)&buf_len);
  glCompileShader(s->handle);

  glGetShaderiv(s->handle, GL_COMPILE_STATUS, &s->ready);

  if (s->ready) return s->ready;

// compile_error:

  int ilog_len = 0;
  glGetShaderiv(s->handle, GL_INFO_LOG_LENGTH, &ilog_len);
  GLsizei log_length = (GLsizei)ilog_len;

  char* log = malloc(log_length);
  glGetShaderInfoLog(s->handle, log_length, &log_length, log);

  str_log("[Shader.build] Error compiling shader:\n{}", log);
  free(log);

  return 0;
}

int shader_build_from_file(Shader* shader, File file) {
  if (!file) goto build_fail;

  GLenum type = 0;
  if (str_ends_with(file->name, ".vert")) type = GL_VERTEX_SHADER;
  else if (str_ends_with(file->name, ".frag")) type = GL_FRAGMENT_SHADER;
  else goto build_fail;

  if (file_read(file)) {
    int ret = shader_build(shader, type, file->str.begin, (uint)file->length);
    if (!ret) {
      str_log("[Shader.build] Failed to build shader: {}", file->name);
    }
    return ret;
  }

build_fail:

  shader->handle = 0;
  shader->ready = 0;

  return 0;
}

void shader_delete(Shader* shader) {
  glDeleteShader(shader->handle);
  shader->handle = 0;
}

void shader_program_new(ShaderProgram* program) {
  program->ready = 0;
}

int shader_program_build(ShaderProgram* p, Shader* vert, Shader* frag) {
  if (!vert->ready || !frag->ready) return 0;
  p->handle = glCreateProgram();
  glAttachShader(p->handle, vert->handle);
  glAttachShader(p->handle, frag->handle);
  glLinkProgram(p->handle);

  glGetProgramiv(p->handle, GL_LINK_STATUS, &p->ready);

  if (!p->ready) {
    int ilog_len = 0;
    glGetProgramiv(p->handle, GL_INFO_LOG_LENGTH, &ilog_len);
    GLsizei log_length = (GLsizei)ilog_len;

    char* log = malloc(log_length);
    glGetProgramInfoLog(p->handle, log_length, &log_length, log);

    str_log("[Program.build] Error compiling shader program:\n{}", log);
    free(log);
  }

  return p->ready;
}

static Shader basic_vert;
static Shader basic_frag;
static ShaderProgram basic_prog;
static int basic_loaded = 0;
int shader_program_build_basic(ShaderProgram* p) {
  if (!basic_loaded) {
    uint vlen = sizeof(basic_vert_text);
    uint flen = sizeof(basic_frag_text);
    shader_build(&basic_vert, GL_VERTEX_SHADER, basic_vert_text, vlen);
    shader_build(&basic_frag, GL_FRAGMENT_SHADER, basic_frag_text, flen);

    basic_loaded = shader_program_build(&basic_prog, &basic_vert, &basic_frag);
    shader_program_load_uniforms(&basic_prog, UNIFORMS_PVM);
  }
  *p = basic_prog;
  return p->ready;
}

static Shader frame_vert;
static Shader frame_frag;
static ShaderProgram frame_prog;
static int frame_loaded = 0;
int shader_program_build_frame(ShaderProgram* p) {
  if (!frame_loaded) {
    uint vlen = sizeof(frame_vert_text);
    uint flen = sizeof(frame_frag_text);
    shader_build(&frame_vert, GL_VERTEX_SHADER, frame_vert_text, vlen);
    shader_build(&frame_frag, GL_FRAGMENT_SHADER, frame_frag_text, flen);

    frame_loaded = shader_program_build(&frame_prog, &frame_vert, &frame_frag);
  }
  *p = frame_prog;
  return p->ready;
}

int shader_program_uniform_location(ShaderProgram* program, const char* name) {
  //assert(program->ready);
  return glGetUniformLocation(program->handle, name);
}

void shader_program_load_uniforms(ShaderProgram* p, UniformSet set) {
  glUseProgram(p->handle);

  p->uniform.projViewMod = glGetUniformLocation(p->handle, "projViewMod");

  switch(set) {
    case UNIFORMS_PVM: break;
    case UNIFORMS_PHONG: {
      p->uniform.phong.world = glGetUniformLocation(p->handle, "world");
      p->uniform.phong.lightPos = glGetUniformLocation(p->handle, "lightPos");
      p->uniform.phong.cameraPos = glGetUniformLocation(p->handle, "cameraPos");
      p->uniform.phong.sampler = glGetUniformLocation(p->handle, "texSamp");
      p->uniform.phong.useVertexColor = glGetUniformLocation(p->handle, "useVertexColor");
    } break;
  }
  glUseProgram(0);
}

void shader_program_use(const ShaderProgram* program) {
  glUseProgram(program->handle);
}

void shader_program_delete(ShaderProgram* p) {
  if (p->handle != basic_prog.handle && p->handle != frame_prog.handle) {
    glDeleteProgram(p->handle);
  }
}
