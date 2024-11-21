#ifndef _FILE_H_
#define _FILE_H_

#include "types.h"
#include "str.h"

typedef struct {
  void* handle; // standin for FILE*
  const char* filename; // remove?

  char* text;
  int length;

  StringRange str;

} File;

void file_new(File* file);
void file_open_async(File* file, const char* filename);
uint file_get_length(File* file);
uint file_read(File* file);
//uint file_load(File* file);
void file_delete(File* file);

#endif

// TODO: Update to use better types (StringRanges, index_s, etc)
