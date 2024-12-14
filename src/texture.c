#include "texture.h"

#include "gl.h"

int texture_build_from_image(Texture* texture, const Image* image) {
  if (!image->ready) {
    texture->handle = 0;
    return 0;
  }

  glPixelStorei(GL_UNPACK_FLIP_Y_WEBGL, GL_TRUE);
  glGenTextures(1, &texture->handle);
  glBindTexture(GL_TEXTURE_2D, texture->handle);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, image->handle);
  if (isPow2(image_width(image)) && isPow2(image_height(image))) {
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }

  return 1; // TODO: proper error checking
}
