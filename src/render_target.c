/*******************************************************************************
* MIT License
*
* Copyright (c) 2026 Curtis McCoy
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#define MCLIB_INTERNAL_IMPL
#include "render_target.h"

#include "texture.h"

#include "str.h"
#include "gl.h"

#include <stdlib.h>
#include <string.h>

typedef struct RenderTarget_Internal {
  struct _opaque_RenderTarget_t pub;

  // private
  uint    handle;
  uint    depth_buffer;
  index_t attachment_count;
  GLenum* color_attachments;

} RenderTarget_Internal;

#define RT_INTERNAL                                                           \
  RenderTarget_Internal* rt = (RenderTarget_Internal*)(rt_in);                \
  assert(rt)                                                                  //

static const GLenum _rt_depth_formats[] = {
  0, GL_DEPTH_COMPONENT32F, GL_DEPTH24_STENCIL8
};

static const GLenum _rt_depth_attachments[] = {
  0, GL_DEPTH_ATTACHMENT, GL_DEPTH_STENCIL_ATTACHMENT
};

extern vec2i game_get_window_size(void);

////////////////////////////////////////////////////////////////////////////////
// Initialize a render target
////////////////////////////////////////////////////////////////////////////////

RenderTarget _rt_new(index_t size, depth_format_t df, tex_format_t formats[]) {
  index_t depth_textures = 0;
  for (index_t i = 0; i < size; ++i) {
    if (formats[i] >= TF_DEPTH_32) ++depth_textures;
  }
  assert(!depth_textures || df == F_DEPTH_NONE);

  RenderTarget_Internal* ret = malloc(
    sizeof(RenderTarget_Internal) +
    size * (sizeof(Texture) + sizeof(tex_format_t)) +
    (size - depth_textures) * sizeof(GLenum)
  );
  assert(ret);

  byte* format_loc = (byte*)(ret + 1) + size * sizeof(Texture);
  byte* attach_loc = format_loc + size * sizeof(tex_format_t);

  *ret = (RenderTarget_Internal) {
    .pub = {
      .resolution = i2zero,
      .slot_count = size,
      .textures = NULL,
      .formats = (tex_format_t*)format_loc,
      .depth_format = df,
      .clear_color = v3f(0.f, 0.f, 0.8f),
      .status = S_NEW,
    },
    .handle = 0,
    .depth_buffer = 0,
    .color_attachments = (GLenum*)attach_loc,
    .attachment_count = size - depth_textures,
  };

  memcpy(ret->pub.formats, formats, sizeof(tex_format_t) * size);

  for (GLenum i = 0; i < (GLenum)ret->attachment_count; ++i) {
    ret->color_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
  }

  return (RenderTarget)ret;
}

////////////////////////////////////////////////////////////////////////////////
// Build the textures and OpenGL objects for the render target
////////////////////////////////////////////////////////////////////////////////

status_t rt_build(RenderTarget rt_in, vec2i screen) {
  RT_INTERNAL;
  if (rt->pub.status == S_READY) return S_READY;

  rt->pub.resolution = screen;

  // framebuffer
  if (!rt->handle) {
    glGenFramebuffers(1, &rt->handle);
    glBindFramebuffer(GL_FRAMEBUFFER, rt->handle);
  }

  // depth and stencil
  if (!rt->depth_buffer && rt->pub.depth_format != F_DEPTH_NONE) {
    assert(rt->pub.depth_format > F_DEPTH_NONE);
    assert(rt->pub.depth_format < F_DEPTH_FORMAT_MAX);

    GLenum depth_format = _rt_depth_formats[rt->pub.depth_format];
    GLenum depth_attachment = _rt_depth_attachments[rt->pub.depth_format];

    glGenRenderbuffers(1, &rt->depth_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, rt->depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, depth_format, screen.w, screen.h);

    glFramebufferRenderbuffer(
      GL_FRAMEBUFFER, depth_attachment, GL_RENDERBUFFER, rt->depth_buffer
    );
  }

  // textures
  if (!rt->pub.textures) {
    rt->pub.textures = (Texture*)(rt + 1);
    index_t depth_binds = 0;

    for (index_t i = 0; i < rt->pub.slot_count; ++i) {
      rt->pub.textures[i] = tex_generate(rt->pub.formats[i], screen);

      // handle texture-based depth attachment
      if (rt->pub.formats[i] >= TF_DEPTH_32) {
        glFramebufferTexture
        ( GL_FRAMEBUFFER
        , GL_DEPTH_ATTACHMENT
        , rt->pub.textures[i]->handle
        , 0
        );
        ++depth_binds;
      }
      else {
        glFramebufferTexture
        ( GL_FRAMEBUFFER
        , GL_COLOR_ATTACHMENT0 + (GLenum)(i - depth_binds)
        , rt->pub.textures[i]->handle
        , 0
        );
      }
    }
  }

  // color attachments
  glDrawBuffers((GLsizei)rt->attachment_count, rt->color_attachments);

  // viewport size
  glViewport(0, 0, screen.w, screen.h);

  // check success of the result
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status == GL_FRAMEBUFFER_COMPLETE) {
    rt->pub.status = S_READY;
  }
  else {
    str_log("[RenderTarget.build] Failed with error: 0x{!x}", status);
    rt->pub.status = S_ERROR;
  }

  return rt->pub.status;
}

////////////////////////////////////////////////////////////////////////////////
// Clears out textures and the depth buffer from this render target
////////////////////////////////////////////////////////////////////////////////

void rt_reset(RenderTarget rt_in) {
  RT_INTERNAL;

  if (rt->handle) {
    glDeleteFramebuffers(1, &rt->handle);
    rt->handle = 0;
  }

  if (rt->depth_buffer) {
    glDeleteRenderbuffers(1, &rt->depth_buffer);
    rt->depth_buffer = 0;
  }

  if (rt->pub.textures) {
    for (index_t i = 0; i < rt->pub.slot_count; ++i) {
      tex_delete(&rt->pub.textures[i]);
    }
    rt->pub.textures = NULL;
  }

  rt->pub.status = S_NEW;
}

////////////////////////////////////////////////////////////////////////////////
// Set the render target as the active target
////////////////////////////////////////////////////////////////////////////////

void rt_bind(RenderTarget rt_in) {
  RT_INTERNAL;
  if (rt->pub.status != S_READY) {
    str_write("[RenderTarget.bind] Target not ready");
    return;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, rt->handle);
}

////////////////////////////////////////////////////////////////////////////////

void rt_bind_default(void) {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

////////////////////////////////////////////////////////////////////////////////
// Binds and clears the contents of the buffer
////////////////////////////////////////////////////////////////////////////////

void rt_bind_clear(RenderTarget rt_in) {
  RT_INTERNAL;

  rt_bind(rt_in);

  color3 c = rt->pub.clear_color;
  glViewport(0, 0, rt->pub.resolution.w, rt->pub.resolution.h);

  glClearColor(c.r, c.g, c.b, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

////////////////////////////////////////////////////////////////////////////////

void rt_bind_clear_default(void) {
  rt_bind_default();
  vec2i window = game_get_window_size();
  glViewport(0, 0, window.w, window.h);

  glClearColor(0.2f, 0.2f, 0.3f, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

////////////////////////////////////////////////////////////////////////////////
// Checks if the given render target is the active one
////////////////////////////////////////////////////////////////////////////////

bool rt_is_bound(RenderTarget rt_in) {
  RT_INTERNAL;
  GLint bound_value = 0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &bound_value);
  return (uint)bound_value == rt->handle;
}

////////////////////////////////////////////////////////////////////////////////
// Clears the render target textures and rebuild at the new screen size
////////////////////////////////////////////////////////////////////////////////

void rt_resize(RenderTarget rt, vec2i screen) {
  rt_bind_default();

  if (rt) {
    rt_reset(rt);
    rt_build(rt, screen);

    if (rt->status != S_READY) {
      str_write("[RenderTarget.resize] Failed to resize");
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Clears and deletes the render target
////////////////////////////////////////////////////////////////////////////////

void rt_delete(RenderTarget* rt_in) {
  if (!rt_in || !*rt_in) return;
  RenderTarget_Internal* rt = (RenderTarget_Internal*)*rt_in;
  rt_reset((RenderTarget)rt);
  free(rt);
  *rt_in = NULL;
}
