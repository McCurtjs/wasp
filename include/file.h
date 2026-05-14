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

#ifndef WASP_FILE_H_
#define WASP_FILE_H_

#include "types.h"
#include "str.h"
#include "status.h"

#include "span_byte.h"

typedef enum file_mode_t {
  FM_READ,
  FM_WRITE,
  FM_READ_WRITE,
  FM_REPLACE,
  FM_COUNT
} file_mode_t;

typedef struct _opaque_File_t {
  slice_t       CONST name;
  status_t      CONST status;
  file_mode_t   CONST mode;
  view_byte_t   CONST data;
  slice_t       CONST slice;
}* File;

File        file_new(slice_t filename, file_mode_t mode);

void        file_loading_manager(void);
index_t     file_loading_count(void);

void        file_delete(File*);
span_byte_t file_release_data(File);

#endif
