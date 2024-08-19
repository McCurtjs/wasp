#ifndef _WASP_GL_H_
#define _WASP_GL_H_

#ifdef __WASM__
#include <GL/gl.h>
#else
//#include <SDL3/SDL_opengles2.h>

//#define GL_GLEXT_PROTOTYPES
//#include <SDL3/SDL_opengl.h>
//#include <SDL3/SDL_opengl_glext.h>

#include "galogen.h"

#define GL_UNPACK_FLIP_Y_WEBGL          0x9240
#define UNPACK_PREMULTIPLY_ALPHA_WEBGL  0x9241

#endif

#endif
