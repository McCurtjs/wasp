#ifndef _WASP_TEXTURE_H_
#define _WASP_TEXTURE_H_

#include "image.h"

typedef struct texture_t {
  uint handle;
} texture_t;

texture_t tex_from_image(Image image);
void tex_free(texture_t* handle);

#endif
