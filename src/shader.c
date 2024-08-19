#include "shader.h"

#include <string.h>

#include "gl.h"
#include "wasm.h"

#include "data/inline_shaders.h"

int hasext(const char* str, const char* ext) {
  const char* strext = strrchr(str, '.');
  return strcmp(strext + 1, ext) == 0;
}

int shader_build(Shader* s, int type, const char* buffer, uint buf_len) {
  s->handle = glCreateShader(type);
  glShaderSource(s->handle, 1, (const char**)&buffer, (const int*)&buf_len);
  glCompileShader(s->handle);

  glGetShaderiv(s->handle, GL_COMPILE_STATUS, &s->ready);
  return s->ready;
}

int shader_build_from_file(Shader* shader, File* file) {
  GLenum type = 0;
  if (hasext(file->filename, "vert")) type = GL_VERTEX_SHADER;
  else if (hasext(file->filename, "frag")) type = GL_FRAGMENT_SHADER;
  file_read(file);
  return shader_build(shader, type, file->text, file->length);
}

void shader_delete(Shader* shader) {
  glDeleteShader(shader->handle);
  shader->handle = 0;
}

void shader_program_new(ShaderProgram* program) {
  program->ready = 0;
}

int shader_program_build(ShaderProgram* p, Shader* vert, Shader* frag) {
  p->handle = glCreateProgram();
  glAttachShader(p->handle, vert->handle);
  glAttachShader(p->handle, frag->handle);
  glLinkProgram(p->handle);

  glGetProgramiv(p->handle, GL_LINK_STATUS, &p->ready);
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

int shader_program_uniform_location(ShaderProgram* program, const char* name) {
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
    } break;
  }
  glUseProgram(0);
}

void shader_program_use(const ShaderProgram* program) {
  glUseProgram(program->handle);
}

void shader_program_delete(ShaderProgram* p) {
  if (p->handle != basic_prog.handle) {
    glDeleteProgram(p->handle);
  }
}
