#ifndef _WASP_IMAGE_H_
#define _WASP_IMAGE_H_

#include "types.h"

// For now, implement using web images and store a handle to the resource.
// Will need an alternate mode in the future for decoding images locally
// (should load with normal File)

typedef struct Image {
  void* handle;
  int   ready;
  uint  width;
  uint  height;
} Image;

void image_open_async(Image* image, const char* filename);
uint image_width(const Image* image);
uint image_height(const Image* image);
void image_delete(Image* image);

#endif
