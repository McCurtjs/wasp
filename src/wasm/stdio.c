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

#include <stdio.h>
#include <string.h>

#include "wasm.h"

extern int    js_fopen(const char* path, int path_len);
extern void   js_fopen_async(int data_id);
extern int    js_fseek(int data_id, int offset, int origin);
extern int    js_ftell(int data_id);
extern size_t js_fread(int data_id, int ptr, size_t size, size_t max_count);
extern int    js_fclose(int data_id);

FILE* fopen(const char* path, const char* mode) {
  if (mode[0] != 'r') wasm_alert(S("fopen read mode other than 'r' not supported!"));
  int data_id = js_fopen(path, strlen(path));
  js_fopen_async(data_id);
  return (FILE*) data_id;
}

int fseek(FILE* stream, long offset, int origin) {
  return js_fseek((int)stream, offset, origin);
}

long ftell(FILE* stream) {
  return js_ftell((int)stream);
}

size_t fread(void* ptr, size_t size, size_t count, FILE* stream) {
  return js_fread((int)stream, (int)ptr, size, count);
}

int fclose(FILE* stream) {
  js_fclose((unsigned int)stream);
  return 0;
}
