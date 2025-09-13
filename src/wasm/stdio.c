#include <stdio.h>
#include <string.h>

#include "wasm.h"

extern int    js_fopen(const char* path, int path_len);
extern void   js_fopen_async(int data_id);
extern int    js_fseek(int data_id, int offset, int origin);
extern int    js_ftell(int data_id);
extern size_t js_fread (int data_id, int ptr, size_t size, size_t max_count);
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
