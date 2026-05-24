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
#include "file.h"
#undef MCLIB_INTERNAL_IMPL

#include <stdio.h>
#include <stdlib.h>

#include "SDL3/SDL.h"

typedef struct File_Internal {
  struct _opaque_File_t pub;
  String        name_internal;
  SDL_AsyncIO*  stream;
  byte*         buffer;
  index_t       size;
} File_Internal;

#ifndef __WASM__
static char* _sdl_modes[FM_COUNT] = {
  "r",
  "w",
  "r+",
  "w+"
};

static SDL_AsyncIOQueue* _sdl_queue = NULL;
#endif

static index_t _async_loading_count = 0;

#ifdef _MSC_VER

// Disable warning for the return line:
//    C33010: "Unchecked lower bound for enum format used as index"
// Obviously the bound is being checked in the assert...
# pragma warning ( disable : 33010 )
#endif

////////////////////////////////////////////////////////////////////////////////

File_Internal* _file_new(slice_t filename, file_mode_t mode) {
  assert(!slice_is_empty(filename));
  assert(mode >= 0 && mode < FM_COUNT);

#ifndef __WASM__
  if (!_sdl_queue) {
    _sdl_queue = SDL_CreateAsyncIOQueue();
    assert(_sdl_queue);
  }
#endif

  File_Internal* ret = malloc(sizeof(File_Internal));
  assert(ret);

  String name_copy = str_copy(filename);

  *ret = (File_Internal) {
    .pub = {
      .name = name_copy->slice,
      .status = S_NEW,
      .mode = mode,
    },
    .stream = NULL,
    .name_internal = name_copy,
  };

  return ret;
}

////////////////////////////////////////////////////////////////////////////////

void _file_assign_buffer(File_Internal* file, void* buffer, index_t size) {
  file->buffer = buffer;

  // Connect the public data handles for the file data
  file->pub.slice = (slice_t) {
    .begin = buffer,
    .size = size
  };

  file->pub.data = (view_byte_t) {
    .begin = buffer,
    .end = (byte*)buffer + size,
  };
}

////////////////////////////////////////////////////////////////////////////////

#ifndef __WASM__
File file_new(slice_t filename, file_mode_t mode) {
  File_Internal* ret = _file_new(filename, mode);

  str_log("[File.new] Loading: {}", filename);

  ret->stream = SDL_AsyncIOFromFile(
    ret->name_internal->begin, _sdl_modes[mode]
  );

  if (mode != FM_READ && mode != FM_READ_WRITE) return (File)ret;

  ret->size = SDL_GetAsyncIOSize(ret->stream);

  if (ret->size < 0) {
    str_log("[File.new] Failed to open file ({}): {}", ret->size, filename);
    ret->pub.status = S_FAILED;
    return (File)ret;
  }

  if (ret->size > 0) {
    ret->buffer = malloc(ret->size);
    ++_async_loading_count;

    bool async_started = SDL_ReadAsyncIO(
      ret->stream, ret->buffer, 0, ret->size, _sdl_queue, ret
    );

    if (async_started) {
      ret->pub.status = S_LOADING;
    }
    else {
      ret->pub.status = S_FAILED;
      str_log("[File.new] Could not start loading file: {}", filename);
    }
  }

  return (File)ret;
}
#else

#include "wasp.h"

extern SDL_AsyncIO* js_file_create(File_Internal*, const char* name, int len);
extern void         js_file_open_async(SDL_AsyncIO* stream);
extern bool         js_file_read(SDL_AsyncIO* stream, void* dst, index_t size);
extern void         js_file_close(SDL_AsyncIO* stream);

void export(file_open_async_done)(File_Internal* file, index_t size) {
  assert(file);
  assert(file->pub.status == S_LOADING);

  --_async_loading_count;

  if (!size) {
    str_log("[File.read] Failed to open file: {}", file->pub.name);
    file->pub.status = S_NOT_FOUND;
    return;
  }

  file->buffer = malloc(size);
  assert(file->buffer);

  if (!js_file_read(file->stream, file->buffer, size)) {
    str_log("[File.read] Failed to read from file: {}", file->pub.name);
    file->pub.status = S_FAILED;
    return;
  }

  _file_assign_buffer(file, file->buffer, size);

  file->pub.status = S_READY;
}

File file_new(slice_t filename, file_mode_t mode) {
  assert(mode == FM_READ);
  File_Internal* ret = _file_new(filename, mode);

  slice_t name = ret->name_internal->slice;
  ret->stream = js_file_create(ret, name.begin, name.size);
  assert(ret->stream);

  ret->pub.status = S_LOADING;
  ++_async_loading_count;

  js_file_open_async(ret->stream);

  return (File)ret;
}

#endif

////////////////////////////////////////////////////////////////////////////////

void file_loading_manager(void) {
#ifndef __WASM__
  SDL_AsyncIOOutcome result;

  while (SDL_GetAsyncIOResult(_sdl_queue, &result)) {
    File_Internal* file = result.userdata;

    // If we closed a stream successfully, remove its now invalid pointer
    if (result.type == SDL_ASYNCIO_TASK_CLOSE) {
      file->stream = NULL;
      continue;
    }

    // If we're not reading, skip to the next one
    if (result.type != SDL_ASYNCIO_TASK_READ) continue;

    --_async_loading_count;
    assert(_async_loading_count >= 0);
    assert(file->buffer == result.buffer);

    str_log("[File.load] Loaded: {}\n  Size: {}, Expected: {}",
      file->pub.name, result.bytes_transferred, result.bytes_requested
    );

    _file_assign_buffer(file, result.buffer, result.bytes_requested);

    file->pub.status = S_READY;

    bool should_flush = file->pub.mode != FM_READ;
    SDL_CloseAsyncIO(file->stream, should_flush, _sdl_queue, file);
  }
#endif
}

////////////////////////////////////////////////////////////////////////////////

index_t file_loading_count(void) {
  return _async_loading_count;
}

////////////////////////////////////////////////////////////////////////////////
// Closing and deleting
////////////////////////////////////////////////////////////////////////////////

void file_delete(File* _file) {
  if (!_file || !*_file) return;
  File_Internal* file = (File_Internal*)*_file;

  if (file->buffer) free(file->buffer);

  if (file->stream) {
#ifndef __WASM__
    assert(_sdl_queue);
    SDL_CloseAsyncIO(file->stream, false, _sdl_queue, NULL);
#else
    js_file_close(file->stream);
#endif
  }

  str_delete(&file->name_internal);

  free(file);
  *_file = NULL;
}

////////////////////////////////////////////////////////////////////////////////

span_byte_t file_release_data(File _file) {
  File_Internal* file = (File_Internal*)_file;
  assert(file);

  if (file->pub.status != S_READY) return span_byte_empty;

  span_byte_t ret = span_byte(file->buffer, file->size);

  file->buffer = NULL;
  file->size = 0;
  file->pub.data = span_byte_empty.view;
  file->pub.slice = slice_empty;
  file->pub.status = S_CLOSED;

  return ret;
}
