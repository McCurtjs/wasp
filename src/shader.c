/*******************************************************************************
* MIT License
*
* Copyright (c) 2026 Curtis McCoy
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

#define MCLIB_INTERNAL_IMPL
#include "shader.h"
#undef MCLIB_INTERNAL_IMPL

#include "str.h"
#include "file.h"
#include "material.h"
#include "map.h"
#include "thread.h"

// Have to undefine this here becuase apparently windows also defines it
#undef CONST

#include <string.h>
#include <stdlib.h>

#include "gl.h"
#include "wasm.h"

#include "data/inline_shaders.h"

typedef enum shader_part_type_t {
  ST_UNKNOWN,
  ST_VERTEX,
  ST_FRAGMENT
} shader_part_type_t;

typedef struct gl_shader_t {
  uint                handle;
  shader_part_type_t  type;
  status_t            status;
  String              filename;
  File                file;
  String              text;
#ifdef _WIN32
  FILETIME            filetime;
#endif
} gl_shader_t;

#define con_type GLint
#define con_prefix int
#include "map.h"
#undef con_type
#undef con_prefix

typedef struct Shader_Internal {
  struct _opaque_Shader_t pub;

  // hidden values
  uint      program_handle;
  String    name_internal;
  slice_t   vert_filename;
  slice_t   frag_filename;
  HMap_int  uniforms;
} Shader_Internal;

#define con_type Shader_Internal*
#define con_prefix shader
#include "map.h"
#undef con_type
#undef con_prefix

#define con_type gl_shader_t
#define con_prefix part
#include "map.h"
#undef con_type
#undef con_prefix

static HMap_shader  _all_shaders_map = NULL;
static HMap_part    _all_parts_map = NULL;
static index_t      _shaders_linked_count = 0;

#define SHADER_INTERNAL                                                       \
  Shader_Internal* s = (Shader_Internal*)(s_in);                              \
  assert(s)                                                                   //

////////////////////////////////////////////////////////////////////////////////
// Helpers for handling default shader names
////////////////////////////////////////////////////////////////////////////////

static bool _shader_is_default_name(slice_t name) {
  return  str_eq(name, "quad")
  ||      str_eq(name, "basic")
  ;
}

////////////////////////////////////////////////////////////////////////////////

static gl_shader_t* _part_get_or_create(shader_part_type_t, slice_t, slice_t);

static gl_shader_t* _part_check_default(slice_t name) {
  if (str_eq(name, "quad_vert"))
    return _part_get_or_create(ST_VERTEX, name, S(shader_quad_vert));
  if (str_eq(name, "quad_frag"))
    return _part_get_or_create(ST_FRAGMENT, name, S(shader_quad_frag));
  if (str_eq(name, "basic_vert"))
    return _part_get_or_create(ST_VERTEX, name, S(shader_basic_vert));
  if (str_eq(name, "basic_frag"))
    return _part_get_or_create(ST_FRAGMENT, name, S(shader_basic_frag));
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Helper functions for managing and compiling GL shader "parts"
////////////////////////////////////////////////////////////////////////////////

static bool _part_build_check(gl_shader_t* s) {
  assert(s);

  if (s->status == S_READY)       { assert(s->handle); return true; }
  if (s->status != S_COMPILING)   { return false; }
  if (!thread_is_main())          { return false; }

  GLint size = (GLint)s->text->size;
  GLint status = 0;
  GLenum type = s->type == ST_VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;

  s->handle = glCreateShader(type);
  assert(s->handle);
  glShaderSource(s->handle, 1, &s->text->begin, &size);
  glCompileShader(s->handle);
  glGetShaderiv(s->handle, GL_COMPILE_STATUS, &status);

  if (status == GL_TRUE) {
    s->status = S_READY;
    str_log("[Shader.build] Compiled: {}", s->filename);
    return true;
  }

  // compile_error:

  int ilog_len = 0;
  glGetShaderiv(s->handle, GL_INFO_LOG_LENGTH, &ilog_len);
  GLsizei log_length = (GLsizei)ilog_len;

  if (!log_length) {
    str_write(
      "[Shader.build] Error compiling shader: no error message generated. "
      "Possible threading issue?"
    );
  }

  char* log = malloc(log_length);
  glGetShaderInfoLog(s->handle, log_length, &log_length, log);

  str_log("[Shader.build] Error compiling shader:\n  {}\n{}", s->filename, log);
  free(log);

  glDeleteShader(s->handle);
  s->handle = 0;
  s->status = S_ERROR;

  return false;
}

////////////////////////////////////////////////////////////////////////////////

static void _part_load_check(gl_shader_t* s) {
  assert(s);
  assert(s->status == S_LOADING);

  if (!s->file || s->file->status != S_READY) return;

  str_delete(&s->text);
  s->text = str_copy(s->file->slice);
  s->status = S_COMPILING;
}

////////////////////////////////////////////////////////////////////////////////

static gl_shader_t* _part_get_or_create(
  shader_part_type_t type, slice_t file_or_name, slice_t text
) {
  assert(type == ST_VERTEX || type == ST_FRAGMENT);
  assert(!slice_is_empty(file_or_name));
  assert(slice_is_valid(text));

  if (!_all_parts_map) _all_parts_map = map_part_new();

  bool is_text_shader = !slice_is_empty(text);

  gl_shader_t* ret = _part_check_default(is_text_shader ? text : file_or_name);
  if (ret) return ret;

  // Use ref/emplace here instead of ensure because the map doesn't store an
  //    extra copy of the key, instead just using the string allocated by the
  //    shader to match object lifetimes.
  ret = map_part_ref(_all_parts_map, file_or_name);
  if (ret) {
    assert(ret->type == type);
    return ret;
  }

  String key_copy = str_copy(file_or_name);

  ret = map_part_emplace(_all_parts_map, key_copy->slice);

  *ret = (gl_shader_t) {
    .handle = 0,
    .type = type,
    .status = S_NEW,
    .filename = key_copy,
    .file = NULL,
    .text = str_empty,
  };

  // Attempt to build the part immediately if its text is available
  if (is_text_shader) {
    ret->status = S_COMPILING;
    ret->text = str_copy(text);

    str_log("[Shader.new] Building text or default shader: {}", key_copy);
    _part_build_check(ret);
  }
  // If it's not available yet, set it to loading
  else {
    ret->status = S_LOADING;
    str_log("[Shader.new] Loading shader text from file: {}", key_copy);
    ret->file = file_new(file_or_name, FM_READ);
  }

  return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Helper functions for managing and linking GL shader programs
////////////////////////////////////////////////////////////////////////////////

static Shader_Internal* _shader_get_or_create(slice_t name) {
  assert(!slice_is_empty(name));

  if (!_all_shaders_map) _all_shaders_map = map_shader_new();

  Shader_Internal** in_map = map_shader_ref(_all_shaders_map, name);
  if (in_map) {
    return *in_map;
  }

  String name_copy = str_copy(name);

  Shader_Internal* ret = malloc(sizeof(Shader_Internal));
  assert(ret);

  *ret = (Shader_Internal) {
    .pub = {
      .name = name_copy->slice,
      .attrib_format = AF_NONE,
      .vertex_format = VF_POS_ONLY,
      .status = S_NEW,
    },
    .program_handle = 0,
    .name_internal = name_copy,
    .vert_filename = slice_empty,
    .frag_filename = slice_empty,
    .uniforms = map_int_new(),
  };

  map_shader_insert(_all_shaders_map, ret->pub.name, ret);

  return ret;
}

////////////////////////////////////////////////////////////////////////////////

static bool _shader_link(Shader_Internal* s, gl_shader_t* v, gl_shader_t* f) {
  assert(s);
  assert(v);
  assert(f);

  if (s->pub.status == S_READY)     { assert(s->program_handle); return true; }
  if (s->pub.status != S_LINKING)   { return false; }
  if (v->status != S_READY)         { return false; }
  if (f->status != S_READY)         { return false; }
  if (!thread_is_main())            { return false; }

  assert(v->handle);
  assert(f->handle);

  GLint status = 0;

  s->program_handle = glCreateProgram();
  assert(s->program_handle);
  glAttachShader(s->program_handle, v->handle);
  glAttachShader(s->program_handle, f->handle);
  glLinkProgram(s->program_handle);

  glGetProgramiv(s->program_handle, GL_LINK_STATUS, &status);

  if (status == GL_TRUE) {
    s->pub.status = S_READY;
    ++_shaders_linked_count;
    str_log("[Shader.link] Linked: {}", s->pub.name);
    return true;
  }

  // link_error:

  s->pub.status = S_ERROR;

  int ilog_len = 0;
  glGetProgramiv(s->program_handle, GL_INFO_LOG_LENGTH, &ilog_len);
  GLsizei log_length = (GLsizei)ilog_len;

  char* log = malloc(log_length);
  glGetProgramInfoLog(s->program_handle, log_length, &log_length, log);

  slice_t log_s = { .begin = log, .size = (index_t)log_length };
  str_log("[Shader.link] Error linking program: {}\n{}", s->pub.name, log_s);

  free(log);

  return false;
}

////////////////////////////////////////////////////////////////////////////////

static bool _shader_link_check(Shader_Internal* shader) {
  assert(shader);
  assert(shader->vert_filename.begin);
  assert(shader->frag_filename.begin);

  if (shader->pub.status == S_READY) return true;

  gl_shader_t* vert = map_part_ref(_all_parts_map, shader->vert_filename);
  gl_shader_t* frag = map_part_ref(_all_parts_map, shader->frag_filename);

  if (shader->pub.status == S_LOADING) {
    if (vert->status == S_READY && frag->status == S_READY) {
      shader->pub.status = S_LINKING;
    }
  }

  if (shader->pub.status != S_LINKING) return false;

  return _shader_link(shader, vert, frag);
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Initialization for shaders based on names, files, or inline scripts
////////////////////////////////////////////////////////////////////////////////

Shader shader_new_from_default(slice_t name) {
  assert(_shader_is_default_name(name));

  str_log("[Shader.new] Building shder from default: {}", name);

  return shader_new(name, slice_empty, slice_empty);
}

////////////////////////////////////////////////////////////////////////////////

Shader shader_new(slice_t name, slice_t vert_text, slice_t frag_text) {
  assert(!slice_is_empty(name));
  assert(!slice_is_empty(vert_text) || _shader_is_default_name(name));
  assert(!slice_is_empty(frag_text) || _shader_is_default_name(name));

  str_log("[Shader.new] Building shader from text: {}", name);

  Shader_Internal* ret = _shader_get_or_create(name);
  if (ret->pub.status != S_NEW) return (Shader)ret;

  String vert_name = str_concat(name, "_vert");
  String frag_name = str_concat(name, "_frag");

  // Set up vertex part
  gl_shader_t* vert = 
    _part_get_or_create(ST_VERTEX, vert_name->slice, vert_text);

  assert(vert);
  ret->vert_filename = vert->filename->slice;
  status_t vert_status = vert->status;

  // Set up fragment part
  gl_shader_t* frag = 
    _part_get_or_create(ST_FRAGMENT, frag_name->slice, frag_text);

  assert(frag);
  ret->frag_filename = frag->filename->slice;
  status_t frag_status = frag->status;

  // Finish up and link
  str_delete(&vert_name);
  str_delete(&frag_name);

  // Attempt to link immediately (since it's text based)
  if (vert_status == S_READY && frag_status == S_READY) {
    ret->pub.status = S_LINKING;
    _shader_link_check(ret);
  }

  // If they're not ready (in case this was called off the main thread)
  else {
    assert(vert_status == S_COMPILING && frag_status == S_COMPILING);
    ret->pub.status = S_COMPILING;
  }

  return (Shader)ret;
}

////////////////////////////////////////////////////////////////////////////////

Shader shader_new_from_name(slice_t name) {
  assert(!slice_is_empty(name));

  str_log("[Shader.new] Building shader from name: {}", name);

  if (_shader_is_default_name(name)) {
    return shader_new_from_default(name);
  }

  Shader_Internal* test = _shader_get_or_create(name);
  if (test->pub.status != S_NEW) return (Shader)test;

  Shader ret = shader_new_from_files(name, name, name);
  assert(ret == (Shader)test);

  return (Shader)ret;
}

////////////////////////////////////////////////////////////////////////////////

Shader shader_new_from_files(
  slice_t name, slice_t vert_name, slice_t frag_name
) {
  assert(!slice_is_empty(name));
  assert(!slice_is_empty(vert_name));
  assert(!slice_is_empty(frag_name));

  str_log("[Shader.new] Building shader from files: {}\n  Vert: {}\n  Frag: {}",
    name, vert_name, frag_name
  );

  Shader_Internal* ret = _shader_get_or_create(name);
  if (ret->pub.status != S_NEW) return (Shader)ret;

  // set up vertex shader
  gl_shader_t* vert = _part_check_default(vert_name);
  if (!vert) {
    String filename = str_concat("./res/shaders/", vert_name, ".vert");
    vert = _part_get_or_create(ST_VERTEX, filename->slice, slice_empty);
    str_delete(&filename);
  }
  ret->vert_filename = vert->filename->slice;
  status_t vert_status = vert->status;

  // set up fragment shader
  gl_shader_t* frag = _part_check_default(frag_name);
  if (!frag) {
    String filename = str_concat("./res/shaders/", frag_name, ".frag");
    frag = _part_get_or_create(ST_FRAGMENT, filename->slice, slice_empty);
    str_delete(&filename);
  }
  ret->frag_filename = frag->filename->slice;
  status_t frag_status = frag->status;

  // link if all parts are available
  ret->pub.status = S_LINKING;

  if (vert_status == S_LOADING || frag_status == S_LOADING)
    ret->pub.status = S_LOADING;
  else if (vert_status == S_COMPILING || frag_status == S_COMPILING)
    ret->pub.status = S_COMPILING;
  else if (vert_status == S_READY && frag_status == S_READY)
    _shader_link_check(ret);

  return (Shader)ret;
}

////////////////////////////////////////////////////////////////////////////////
// Shader loading management
////////////////////////////////////////////////////////////////////////////////

index_t shader_manage_update(void) {
  assert(thread_is_main());

  if (_shaders_linked_count == _all_shaders_map->size) return 0;

  gl_shader_t* map_foreach(part, _all_parts_map) {
    if (part->status == S_LOADING) {
      _part_load_check(part);
    }

    if (part->status == S_COMPILING) {
      _part_build_check(part);
    }
  }

  Shader_Internal** map_foreach(pshader, _all_shaders_map) {
    Shader_Internal* shader = *pshader;
    if (shader->pub.status != S_READY) {
      _shader_link_check(shader);
    }
  }

  index_t ret = _all_shaders_map->size - _shaders_linked_count;
  assert(ret >= 0);
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Remove after use
////////////////////////////////////////////////////////////////////////////////

void shader_delete(Shader* shader) {
  if (!shader || !*shader) return;
  Shader_Internal* s = (Shader_Internal*)*shader;
  str_delete(&s->name_internal);
  map_int_delete(&s->uniforms);

  // TODO: Safely free all the shader related memory/handles
  *shader = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Apply the shader for use
////////////////////////////////////////////////////////////////////////////////

bool shader_bind(Shader s_in) {
  SHADER_INTERNAL;
  if (s->pub.status != S_READY) return false;
  glUseProgram(s->program_handle);
  return true;
}

////////////////////////////////////////////////////////////////////////////////

void shader_bind_attributes(Shader s_in) {
  SHADER_INTERNAL;
  assert(s_in->status == S_READY);
  attribute_bind(s->pub.attrib_format, s_in);
}

////////////////////////////////////////////////////////////////////////////////

void shader_bind_material(Shader s_in, Material material) {
  SHADER_INTERNAL;
  assert(s_in->status == S_READY);
  assert(material);

  // get uniform/attribute locations for active material
  int loc_sampler_tex =   shader_uniform_loc(s_in, "samp_tex");
  int loc_sampler_norm =  shader_uniform_loc(s_in, "samp_norm");
  int loc_sampler_rough = shader_uniform_loc(s_in, "samp_rough");
  int loc_sampler_metal = shader_uniform_loc(s_in, "samp_metal");
  int loc_props =         shader_uniform_loc(s_in, "in_weights");

  // matset_apply(renderer->materials);
  tex_apply(material->map_diffuse, 0, loc_sampler_tex);
  tex_apply(material->map_normals, 1, loc_sampler_norm);
  tex_apply(material->map_roughness, 2, loc_sampler_rough);
  tex_apply(material->map_metalness, 3, loc_sampler_metal);
  glUniform3fv(loc_props, 1, material->weights.f);
}

////////////////////////////////////////////////////////////////////////////////
// Get a uniform location by name from the shader
////////////////////////////////////////////////////////////////////////////////

int shader_uniform_loc(Shader s_in, const char* name) {
  SHADER_INTERNAL;
  assert(s->pub.status == S_READY);

  slice_t name_slice = slice_from_c_str(name);
  res_ensure_int_t slot = map_int_ensure(s->uniforms, name_slice);

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
// Get an attribute location by name from the shader
////////////////////////////////////////////////////////////////////////////////

int shader_attribute_loc(Shader s_in, const char* name) {
  SHADER_INTERNAL;
  assert(s->pub.status == S_READY);

  slice_t name_slice = slice_from_c_str(name);
  res_ensure_int_t slot = map_int_ensure(s->uniforms, name_slice);

  if (slot.is_new) {
    *slot.value = glGetAttribLocation(s->program_handle, name);
    int err = glGetError();
    if (err) {
      str_log("[Shader.attrib_loc] Failed: {}, error: 0x{!x}", name, err);
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

void _gl_shader_update_program(Shader_Internal* s, gl_shader_t* part) {
  gl_shader_t* vert = NULL;
  gl_shader_t* frag = NULL;

  // check if this shader is affected by a part being updated
  if (str_eq(s->vert_filename, part->filename)) {
    vert = part;
  }
  else if (str_eq(s->frag_filename, part->filename)) {
    frag = part;
  }
  else {
    return;
  }

  if (!vert) vert = map_part_ref(_all_parts_map, s->vert_filename);
  if (!frag) frag = map_part_ref(_all_parts_map, s->frag_filename);

  assert(vert);
  assert(frag);

  if (vert->status != S_READY || frag->status != S_READY) return;

  Shader_Internal temp = *s;
  temp.pub.status = S_LINKING;
  temp.program_handle = 0;

  if (!_shader_link(&temp, vert, frag)) {
    str_log("[Shader.watch] Failed to re-link shader: {}", s->pub.name);
    return;
  }

  --_shaders_linked_count;
  glDeleteProgram(s->program_handle);

  str_log("[Shader.watch] Re-linked: {}", s->pub.name);

  int err = glGetError();
  if (err) {
    str_log("[Shader.watch] Error during reload: 0x{!x:04}", err);
  }

  *s = temp;
  map_int_clear(s->uniforms);
}

////////////////////////////////////////////////////////////////////////////////

void _gl_shader_update_programs(gl_shader_t* part) {
  Shader_Internal** map_foreach(pshader, _all_shaders_map) {
    _gl_shader_update_program(*pshader, part);
  }
}

////////////////////////////////////////////////////////////////////////////////

void _gl_shader_update(gl_shader_t* part) {

  gl_shader_t temp = (gl_shader_t) {
    .handle = 0,
    .filename = part->filename,
    .file = file_new(part->filename->slice, FM_READ),
    .status = S_COMPILING,
    .type = part->type
  };

  if (!temp.file || temp.file->status != S_READY) {
    str_write("[Shader.watch] Failed to read new file");
    file_delete(&temp.file);
    return;
  }

  temp.text = str_copy(temp.file->slice);

  if (_part_build_check(&temp)) {
    str_write("[Shader.watch] Reload success!");
    glDeleteShader(part->handle);
    file_delete(&part->file);
    str_delete(&part->text);
    *part = temp;

    _gl_shader_update_programs(part);
  }
  else {
    str_write("[Shader.watch] Reload failed.");
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
  if (_all_parts_map) {
    gl_shader_t* map_foreach(shader, _all_parts_map) {
      _shader_check_update(shader);
    }
  }
}

#endif
