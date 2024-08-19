#ifndef _WASP_TEXTURE_H_
#define _WASP_TEXTURE_H_

#include "image.h"

typedef struct Texture {
  uint handle;
} Texture;

int texture_build_from_image(Texture* texture, const Image* image);

#endif
