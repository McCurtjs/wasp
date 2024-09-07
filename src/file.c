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
  file->text = (char*)malloc(file->length);
  fseek(file->handle, 0, SEEK_SET);
  fread(file->text, sizeof(char), file->length, file->handle);
  fclose(file->handle);
  file->handle = NULL;
  return file->length;
}

void file_delete(File* file) {
  if (file->handle) fclose(file->handle);
  if (file->text) free(file->text);
  file_new(file);
}
