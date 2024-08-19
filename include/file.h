#ifndef _FILE_H_
#define _FILE_H_

#include "types.h"

typedef struct {
  void* handle; // standin for FILE*
  const char* filename; // remove?
  char* text;
  int length;
} File;

void file_new(File* file);
void file_open_async(File* file, const char* filename);
uint file_get_length(File* file);
uint file_read(File* file);
void file_delete(File* file);

#endif
