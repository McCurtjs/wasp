#include "file.h"

#include <stdio.h>
#include <stdlib.h>

FILE*   wasm_fopen(const char* path, const char* mode);
int     wasm_fseek(FILE* stream, long offset, int origin);
long    wasm_ftell(FILE* stream);
size_t  wasm_fread(void* ptr, size_t size, size_t count, FILE* stream);
int     wasm_fclose(FILE* stream);

void file_new(File* file) {
  file->handle = NULL;
  file->filename = NULL;
  file->text = NULL;
  file->length = 0;
}

void file_open_async(File* file, const char* filename) {
  file_new(file);
  file->filename = filename;
  file->handle = wasm_fopen(filename, "r");
}

uint file_get_length(File* file) {
  wasm_fseek(file->handle, 0, SEEK_END);
  return wasm_ftell(file->handle);
}

uint file_read(File* file) {
  if (file->text) return 0;
  file->length = file_get_length(file);
  if (file->length == 0) return 0;
  file->text = (char*)malloc(file->length);
  wasm_fseek(file->handle, 0, SEEK_SET);
  wasm_fread(file->text, sizeof(char), file->length, file->handle);
  wasm_fclose(file->handle);
  file->handle = NULL;
  return file->length;
}

void file_delete(File* file) {
  if (file->handle) wasm_fclose(file->handle);
  if (file->text) free(file->text);
  file_new(file);
}
