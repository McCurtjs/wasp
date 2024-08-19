
#include "wasm.h"

int main(int argc, char* argv[]) {
  String result = str_format("Running demo from: {} ({})", argv[0], argc);

  str_print(result->range);
  str_delete(&result);

  return 0;
}
