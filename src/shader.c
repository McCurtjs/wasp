/*******************************************************************************
* MIT License
*
* Copyright (c) 2025 Curtis McCoy
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include "shader.h"

#include <string.h>
#include <stdlib.h>

#include "str.h"
#include "file.h"

#include "gl.h"
#include "wasm.h"

#include "data/inline_shaders.h"

#include "map.h"

typedef struct gl_shader_t {
  uint    handle;
  String  filename;
  File    file;
  bool    ready;
} gl_shader_t;

typedef struct Shader_Internal {
  slice_t       name;
  bool          ready;

  // hidden values
  uint          program_handle;
  String        name_internal;
  String        vert_filename;
  String        frag_filename;
  HMap          uniforms;
} Shader_Internal;

#define SHADER_INTERNAL \
  Shader_Internal* s = (Shader_Internal*)(s_in); \
  assert(s)

HMap _shader_parts = NULL;
HMap _all_shaders = NULL;

////////////////////////////////////////////////////////////////////////////////
// Helper functions for OpenGL shader compiling
////////////////////////////////////////////////////////////////////////////////

bool _gl_shader_build(gl_shader_t* s, int type, slice_t buffer) {
  GLint size = (GLint)buffer.size;
  GLint status = 0;

  s->handle = glCreateShader(type);
  glShaderSource(s->handle, 1, &buffer.begin, &size);
  glCompileShader(s->handle);
  glGetShaderiv(s->handle, GL_COMPILE_STATUS, &status);

  if (status == GL_TRUE) {
    s->ready = true;
    return true;
  }

  // compile_error:

  int ilog_len = 0;
  glGetShaderiv(s->handle, GL_INFO_LOG_LENGTH, &ilog_len);
  GLsizei log_length = (GLsizei)ilog_len;

  char* log = malloc(log_length);
  glGetShaderInfoLog(s->handle, log_length, &log_length, log);

  str_log("[Shader.build] Error compiling shader:\n  {}\n", s->filename, log);
  free(log);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

bool _gl_shader_link(Shader_Internal* s, gl_shader_t* vert, gl_shader_t* frag) {
  if (s->ready || !vert->ready || !frag->ready) return 0;

  GLint status = 0;

  s->program_handle = glCreateProgram();
  glAttachShader(s->program_handle, vert->handle);
  glAttachShader(s->program_handle, frag->handle);
  glLinkProgram(s->program_handle);

  glGetProgramiv(s->program_handle, GL_LINK_STATUS, &status);

  if (status == GL_TRUE) {
    s->ready = true;
    return true;
  }

  // link_error:

  int ilog_len = 0;
  glGetProgramiv(s->program_handle, GL_INFO_LOG_LENGTH, &ilog_len);
  GLsizei log_length = (GLsizei)ilog_len;

  char* log = malloc(log_length);
  glGetProgramInfoLog(s->program_handle, log_length, &log_length, log);

  slice_t log_s = { .begin = log, .size = (index_t)log_length };
  str_log("[Program.build] Error compiling shader program:\n{}", log_s);

  free(log);

  return false;
}

////////////////////////////////////////////////////////////////////////////////

Shader _shader_basic = NULL;
void _shader_build_basic(Shader s_in) {
  SHADER_INTERNAL;
  if (_shader_basic) return;

  gl_shader_t vert = { 0 };
  gl_shader_t frag = { 0 };

  _gl_shader_build(&vert, GL_VERTEX_SHADER, slice_from_c_str(basic_vert_text));
  _gl_shader_build(&frag, GL_FRAGMENT_SHADER, slice_from_c_str(basic_frag_text));

  if (!vert.ready || !frag.ready) {
    str_write("[Shader.build_basic] Basic shader failed to build");
  }

  if (!_gl_shader_link(s, &vert, &frag)) {
    str_write("[Shader.build_basic] Basic shader failed to link");
  }

  _shader_basic = s_in;
}

////////////////////////////////////////////////////////////////////////////////
// Initialize a new shader by name
////////////////////////////////////////////////////////////////////////////////

Shader shader_new(slice_t name) {

  if (!_all_shaders) {
    _all_shaders = map_new(slice_t, Shader, slice_hash_vptr, slice_compare_vptr);
  }

  Shader in_map = map_ref(_all_shaders, &name);
  if (in_map) {
    str_log("[Shader.new] Name already in use: {}", name);
    return in_map;
  }

  str_log("[Shader.new] Building shader: {}", name);

  Shader_Internal* ret = malloc(sizeof(Shader_Internal));
  assert(ret);

  String name_str = str_copy(name);

  *ret = (Shader_Internal) {
    .name = name_str->slice,
    .ready = false,

    .program_handle = 0,
    .name_internal = name_str,
    .vert_filename = NULL,
    .frag_filename = NULL,
    .uniforms = NULL
  };

  // set up uniforms map
  ret->uniforms = map_new(slice_t, GLint, slice_hash_vptr, slice_compare_vptr);

  map_insert(_all_shaders, &ret->name, &ret);

  if (str_eq(name, "basic")) {
    _shader_build_basic((Shader)ret);
  }

  return (Shader)ret;
}

////////////////////////////////////////////////////////////////////////////////
// Create shader and start loading
////////////////////////////////////////////////////////////////////////////////

Shader shader_new_load(slice_t name) {
  Shader ret = shader_new(name);
  shader_load_async(ret);
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Start loading the shader files
////////////////////////////////////////////////////////////////////////////////

void shader_load_async(Shader s_in) {
  SHADER_INTERNAL;

  if (s->program_handle) {
    str_log("[Shader.load] Shader alrady loaded: {}", s->name);
    if (s->ready) return;
  }

  if (!s->vert_filename) {
    s->vert_filename = str_format("./res/shaders/{}.vert", s->name);
  }

  if (!s->frag_filename) {
    s->frag_filename = str_format("./res/shaders/{}.frag", s->name);
  }

  if (!_shader_parts) {
    _shader_parts = map_new(
      slice_t, gl_shader_t, slice_hash_vptr, slice_compare_vptr
    );
  }

  res_ensure_t vert_ens = map_ensure(_shader_parts, s->vert_filename);
  gl_shader_t* vert = vert_ens.value;

  if (vert_ens.is_new) {
    *vert = (gl_shader_t) {
      .handle = 0,
      .filename = s->vert_filename,
      .file = file_new(s->vert_filename->slice),
      .ready = false
    };
  }
  else {
    str_delete(&s->vert_filename);
  }

  res_ensure_t frag_ens = map_ensure(_shader_parts, s->frag_filename);
  gl_shader_t* frag = frag_ens.value;

  if (frag_ens.is_new) {
    *frag = (gl_shader_t) {
      .handle = 0,
      .filename = s->frag_filename,
      .file = file_new(s->frag_filename->slice),
      .ready = false
    };
  }
  else {
    str_delete(&s->frag_filename);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Build the shader
////////////////////////////////////////////////////////////////////////////////

void shader_build(Shader s_in) {
  SHADER_INTERNAL;
  if (s->ready) return;

  gl_shader_t* vert = map_ref(_shader_parts, s->vert_filename);
  gl_shader_t* frag = map_ref(_shader_parts, s->frag_filename);

  if (!vert || !frag) {
    if (!vert) str_log("[Shader.build] Missing shader: {}", s->vert_filename);
    if (!frag) str_log("[Shader.build] Missing shader: {}", s->frag_filename);
    return;
  }

  file_read(vert->file);
  file_read(frag->file);

  if (!vert->ready) {
    if (vert->file && vert->file->data) {
      _gl_shader_build(vert, GL_VERTEX_SHADER, vert->file->str);
      file_delete(&vert->file);
    } else {
      str_write("[Shader.build] Vertex shader broke");
    }
  }

  if (!frag->ready) {
    if (frag->file && frag->file->data) {
      _gl_shader_build(frag, GL_FRAGMENT_SHADER, frag->file->str);
      file_delete(&frag->file);
    } else {
      str_write("[Shader.build] Fragment shader broken");
    }
  }

  if (!vert->ready || !frag->ready) {
    str_log("[Shader.build] Shader component not ready: {} for {}",
      vert->ready ? "vertex" : "fragment", s->name
    );
    return;
  }

  if (!_gl_shader_link(s, vert, frag)) {
    str_log("[Shader.build] Failed to link shader: {}", s->name);
  }

  s->ready = true;
}

////////////////////////////////////////////////////////////////////////////////
// Apply the shader for use
////////////////////////////////////////////////////////////////////////////////

void shader_bind(Shader s_in) {
  SHADER_INTERNAL;
  assert(s->ready);
  glUseProgram(s->program_handle);
}

////////////////////////////////////////////////////////////////////////////////
// Get a uniform location by name from the shader
////////////////////////////////////////////////////////////////////////////////

int shader_uniform_loc(Shader s_in, const char* name) {
  SHADER_INTERNAL;
  assert(s->ready);

  slice_t name_slice = slice_from_c_str(name);
  res_ensure_t slot = map_ensure(s->uniforms, &name_slice);

  if (slot.is_new) {
    *(GLint*)slot.value = glGetUniformLocation(s->program_handle, name);
  }

  return *(int*)slot.value;
}
