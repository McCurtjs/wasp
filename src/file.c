#include "file.h"

#include <stdio.h>
#include <stdlib.h>

void file_new(File* file) {
  file->handle = NULL;
  file->filename = NULL;
  file->text = NULL;
  file->length = 0;
}

void file_open_async(File* file, const char* filename) {
  file_new(file);
  file->filename = filename;
  file->handle = fopen(filename, "r");
}

uint file_get_length(File* file) {
  fseek(file->handle, 0, SEEK_END);
  return ftell(file->handle);
}

uint file_read(File* file) {
  if (file->text) return 0;
  file->length = file_get_length(file);
  if (file->length == 0) return 0;
  file->text = (char*)malloc(file->length + 1);
  fseek(file->handle, 0, SEEK_SET);
  if (!file->text) return 0;
  fread(file->text, sizeof(char), file->length, file->handle);
  fclose(file->handle);
  file->text[file->length] = '\0';
  file->handle = NULL;
  file->str.begin = file->text;
  file->str.length = file->length;
  return file->length;
}

/*
uint file_load(File* file) {
  if (file->text) return 0;
  file->length = file_get_length(file);
  if (file->length == 0) {
    file->bytes = NULL;
    return 0;
  }
  file->bytes = arr_byte_new_reserve(file->length);
  fseek(file->handle, 0, SEEK_SET);
  fread(file->bytes->arr, sizeof(byte), file->length, file->handle);
  fclose(file->handle);
  file->handle = NULL;
  return file->length;
}
//*/

void file_delete(File* file) {
  str_log("closing handle for {}", file->filename);
  if (file->handle) fclose(file->handle);
  str_log("freeing file text");
  str_log("0x{!x}", (size_t)file->text);
  str_log("{}", str_substring(file->str, 0, 20));
  if (file->text) free(file->text);
  str_log("zeroing out data");
  file_new(file);
}
