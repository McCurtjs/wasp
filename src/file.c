#include "file.h"

#include <stdio.h>
#include <stdlib.h>

File file_new(slice_t filename) {
  String name = str_copy(filename);
  FILE* handle = fopen(name->begin, "r");
  File file = NULL;
  
  if (!handle) goto error;

  file = malloc(sizeof(*file));

  if (!file) goto error;

  file->handle = handle;
  file->name = name;
  file->data = NULL;
  file->length = 0;

  return file;

error:

  str_log("[File.new] Failed to open file: {}", filename);

  str_delete(&name);
  if (file) file_delete(&file);
  if (handle) fclose(handle);

  return NULL;
}

long file_read_length(File file) {
  if (!file) return 0;
  if (!file->handle) return (long)file->length;
  fseek(file->handle, 0, SEEK_END);
  return ftell(file->handle);
}

bool file_read(File file) {
  if (!file) return FALSE;
  if (file->data) return TRUE;
  assert(file->handle);

  long length = file_read_length(file);
  if (length == 0) goto error_empty;

  file->data = (byte*)malloc(length + 1); // always null-terminate
  if (!file->data) goto error_too_big;

  fseek(file->handle, 0, SEEK_SET);

  // Get the actual read length, ftell can overshoot, apparently
  file->length = (index_t)fread(file->data, sizeof(byte), length, file->handle);

  fclose(file->handle);
  file->handle = NULL;
  file->data[length] = '\0';

  return TRUE;

error_too_big:

  str_log("[File.read] File too large: {}", file->name);
  return FALSE;

error_empty:

  str_log("[File.read] Empty file: {}", file->name);
  return FALSE;
}

void file_delete(File* file) {
  if (!file || !*file) return;
  File f = *file;

  if (f->handle) fclose(f->handle);
  if (f->data) free(f->data);
  if (f->name) str_delete(&f->name);

  *file = NULL;
}
