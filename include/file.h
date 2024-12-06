#ifndef _FILE_H_
#define _FILE_H_

#include "types.h"
#include "str.h"

typedef struct {
  void* handle; // standin for FILE*
  String name;

  union {
    struct {
      byte* data;
      index_s length;
    };
    StringRange str;
  };

}* File;

File    file_new(StringRange filename);
long    file_read_length(File file);
bool    file_read(File file);
void    file_delete(File* file);

#endif

// TODO: Update to use better types (StringRanges, index_s, etc)
