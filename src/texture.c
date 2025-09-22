#include "texture.h"

#include "gl.h"

texture_t tex_from_image(Image image) {
  texture_t ret = { 0 };

  if (!image || !image->ready) return ret;

  GLenum fmt = GL_RGBA;
  switch (image->channels) {

    case 3:
      fmt = GL_RGB;
      glPixelStorei(GL_UNPACK_ALIGNMENT, GL_TRUE);
      break;

    case 4:
      glPixelStorei(GL_UNPACK_ALIGNMENT, GL_FALSE);
      break;

#ifdef __WASM__
    case 0: break;
#endif

    default:
      assert(false);
      break;
  }

  glPixelStorei(GL_UNPACK_FLIP_Y_WEBGL, GL_TRUE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &ret.handle);
  glBindTexture(GL_TEXTURE_2D, ret.handle);
  glTexImage2D(
    GL_TEXTURE_2D, 0, fmt, image->width, image->height,
    0, fmt, GL_UNSIGNED_BYTE, image->data
  );

  if (isPow2(image->width) && isPow2(image->height)) {
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  return ret;
}

void tex_free(texture_t* texture) {
  glDeleteTextures(1, &texture->handle);
  texture->handle = 0;
}
