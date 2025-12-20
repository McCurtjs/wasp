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

enum shader_part_type_t {
  ST_UNKNOWN,
  ST_VERTEX,
  ST_FRAGMENT
};

typedef struct gl_shader_t {
  uint      handle;
  String    filename;
  File      file;
  bool      ready;
  byte      type;
#ifdef _WIN32
  FILETIME  filetime;
#endif
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

bool _gl_shader_build(gl_shader_t* s, slice_t buffer) {
  GLint size = (GLint)buffer.size;
  GLint status = 0;
  GLenum type = s->type == ST_VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;

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

  str_log("[Shader.build] Error compiling shader:\n  {}\n\n{}", s->filename, log);
  free(log);

  glDeleteShader(s->handle);
  s->handle = 0;

  return false;
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

  gl_shader_t vert = { .type = ST_VERTEX };
  gl_shader_t frag = { .type = ST_FRAGMENT };

  _gl_shader_build(&vert, slice_from_c_str(basic_vert_text));
  _gl_shader_build(&frag, slice_from_c_str(basic_frag_text));

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
    _all_shaders =
      map_new(slice_t, Shader, slice_hash_vptr, slice_compare_vptr);
  }

  Shader* in_map = map_ref(_all_shaders, &name);
  if (in_map) {
    str_log("[Shader.new] Name already in use: {}", name);
    return *in_map;
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

Shader shader_new_load_async(slice_t name) {
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

  if (!_shader_parts) {
    _shader_parts = map_new(
      slice_t, gl_shader_t, slice_hash_vptr, slice_compare_vptr
    );
  }

  if (!s->vert_filename) {
    s->vert_filename = str_format("./res/shaders/{}.vert", s->name);
  }

  res_ensure_t vert_ens = map_ensure(_shader_parts, s->vert_filename);
  gl_shader_t* vert = vert_ens.value;

  if (vert_ens.is_new) {
    *vert = (gl_shader_t) {
      .handle = 0,
      .filename = s->vert_filename,
      .file = file_new(s->vert_filename->slice),
      .ready = false,
      .type = ST_VERTEX
    };
  }
  else {
    str_delete(&s->vert_filename);
  }

  if (!s->frag_filename) {
    s->frag_filename = str_format("./res/shaders/{}.frag", s->name);
  }

  res_ensure_t frag_ens = map_ensure(_shader_parts, s->frag_filename);
  gl_shader_t* frag = frag_ens.value;

  if (frag_ens.is_new) {
    *frag = (gl_shader_t) {
      .handle = 0,
      .filename = s->frag_filename,
      .file = file_new(s->frag_filename->slice),
      .ready = false,
      .type = ST_FRAGMENT
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
      _gl_shader_build(vert, vert->file->str);
      file_delete(&vert->file);
    } else {
      str_write("[Shader.build] Vertex shader broke");
    }
  }

  if (!frag->ready) {
    if (frag->file && frag->file->data) {
      _gl_shader_build(frag, frag->file->str);
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
    return;
  }

  s->ready = true;
}

////////////////////////////////////////////////////////////////////////////////

void shader_build_all(void) {
  Shader* map_foreach(shader, _all_shaders) {
    shader_build(*shader);
  }
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
    int err = glGetError();
    if (err) {
      str_log("[Shader.uniform_loc] Failed: {}, error: 0x{!x}", name, err);
    }

  }

  return *(int*)slot.value;
}

////////////////////////////////////////////////////////////////////////////////
// Automatic refresh of shaders on Windows for debugging
////////////////////////////////////////////////////////////////////////////////

#ifndef _WIN32
void shader_check_updates(void) {
  // do nothing on WASM or other platforms
}
#else

#include <Windows.h>

void _gl_shader_update_program(Shader s_in, gl_shader_t* part) {
  SHADER_INTERNAL;

  gl_shader_t* vert = NULL;
  gl_shader_t* frag = NULL;

  if (str_eq(s->vert_filename, part->filename)) {
    vert = part;
  }
  else if (str_eq(s->frag_filename, part->filename)) {
    frag = part;
  }
  else {
    return;
  }

  if (!vert) vert = map_ref(_shader_parts, s->vert_filename);
  if (!frag) frag = map_ref(_shader_parts, s->frag_filename);

  Shader_Internal temp = *s;
  temp.ready = false;
  temp.program_handle = 0;

  if (!_gl_shader_link(&temp, vert, frag)) {
    str_log("[Shader.watch] Failed to re-link shader: {}", s->name);
    return;
  }

  glDeleteProgram(s->program_handle);

  str_log("[Shader.watch] Re-linked: {}", s->name);

  *s = temp;
  map_clear(s->uniforms);
}

////////////////////////////////////////////////////////////////////////////////

void _gl_shader_update_programs(gl_shader_t* part) {
  Shader* map_foreach(shader, _all_shaders) {
    _gl_shader_update_program(*shader, part);
  }
}

////////////////////////////////////////////////////////////////////////////////

void _gl_shader_update(gl_shader_t* part) {

  gl_shader_t temp = (gl_shader_t) {
    .handle = 0,
    .filename = part->filename,
    .file = file_new(part->filename->slice),
    .ready = false,
    .type = part->type
  };

  if (!temp.file || !file_read(temp.file) || !temp.file->data) {
    str_write("[Shader.watch] Failed to read new file");
    file_delete(&temp.file);
    return;
  }

  if (_gl_shader_build(&temp, temp.file->str)) {
    str_write("[Shader.watch] Reload success!");
    glDeleteShader(part->handle);
    *part = temp;

    _gl_shader_update_programs(part);
  }
  else {
    str_write("[Shader.write] Reload failed.");
  }

  file_delete(&temp.file);
}

////////////////////////////////////////////////////////////////////////////////

void _shader_check_update(gl_shader_t* part) {
  HANDLE hFile = CreateFile
  ( part->filename->begin
  , GENERIC_READ
  , FILE_SHARE_READ
  , NULL
  , OPEN_EXISTING
  , FILE_ATTRIBUTE_NORMAL
  , NULL
  );

  if (!hFile) return;

  FILETIME last_write_time;
  if (!GetFileTime(hFile, NULL, NULL, &last_write_time)) goto close_handle;

  // set initial filetime if we're watching
  if (part->filetime.dwHighDateTime == 0 && part->filetime.dwLowDateTime == 0) {
    part->filetime = last_write_time;
    goto close_handle;
  }

  // if the update time is different, reload the files
  if (CompareFileTime(&last_write_time, &part->filetime) != 0) {
    str_log("[Shader.watch] Updated: {}", part->filename);

    _gl_shader_update(part);

    // update the last write time regardless of success/replacement
    part->filetime = last_write_time;
  }

close_handle:

  CloseHandle(hFile);
}

////////////////////////////////////////////////////////////////////////////////

void shader_check_updates(void) {
  gl_shader_t* map_foreach(shader, _shader_parts) {
    _shader_check_update(shader);
  }
}

#endif
