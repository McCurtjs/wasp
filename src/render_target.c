/*******************************************************************************
* MIT License
*
* Copyright (c) 2025 Curtis McCoy
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

#include "render_target.h"

#include "texture.h"

#include "str.h"
#include "gl.h"

#include <stdlib.h>
#include <string.h>

typedef struct RenderTarget_Internal {
  index_t           slot_count;
  texture_t*        textures;
  texture_format_t* formats;
  depth_format_t    depth_format;
  color3            clear_color;
  bool              ready;

  // private
  uint              handle;
  uint              depth_buffer;
  index_t           attachment_count;
  GLenum*           color_attachments;

} RenderTarget_Internal;

#define RT_INTERNAL \
  RenderTarget_Internal* rt = (RenderTarget_Internal*)(rt_in); \
  assert(rt)

static const GLenum _rt_depth_formats[] = {
  0, GL_DEPTH_COMPONENT32F, GL_DEPTH24_STENCIL8
};

static const GLenum _rt_depth_attachments[] = {
  0, GL_DEPTH_ATTACHMENT, GL_DEPTH_STENCIL_ATTACHMENT
};

////////////////////////////////////////////////////////////////////////////////
// Initialize a render target
////////////////////////////////////////////////////////////////////////////////

RenderTarget _rt_new(index_t size, texture_format_t formats[]) {
  index_t depth_textures = 0;
  for (index_t i = 0; i < size; ++i) {
    if (formats[i] >= TF_DEPTH_32) ++depth_textures;
  }

  RenderTarget_Internal* ret = malloc(
    sizeof(RenderTarget_Internal) +
    size * (sizeof(texture_t) + sizeof(texture_format_t)) +
    (size - depth_textures) * sizeof(GLenum)
  );
  assert(ret);

  byte* format_loc = (byte*)(ret + 1) + size * sizeof(texture_t);
  byte* attach_loc = format_loc + size * sizeof(texture_format_t);

  *ret = (RenderTarget_Internal) {
    .slot_count = size,
    .textures = NULL,
    .formats = (texture_format_t*)format_loc,
    .depth_format = depth_textures ? F_DEPTH_NONE : F_DEPTH_32,
    .clear_color = v3f(0.f, 0.f, 0.8f),
    .ready = false,
    .handle = 0,
    .depth_buffer = 0,
    .color_attachments = (GLenum*)attach_loc,
    .attachment_count = size - depth_textures,
  };

  memcpy(ret->formats, formats, sizeof(texture_format_t) * size);

  for (GLenum i = 0; i < (GLenum)ret->attachment_count; ++i) {
    ret->color_attachments[i] = GL_COLOR_ATTACHMENT0 + i;
  }

  return (RenderTarget)ret;
}

////////////////////////////////////////////////////////////////////////////////
// Build the textures and OpenGL objects for the render target
////////////////////////////////////////////////////////////////////////////////

bool rt_build(RenderTarget rt_in, vec2i screen) {
  RT_INTERNAL;
  if (rt->ready) return false;

  // framebuffer
  if (!rt->handle) {
    glGenFramebuffers(1, &rt->handle);
    glBindFramebuffer(GL_FRAMEBUFFER, rt->handle);
  }

  // depth and stencil
  if (!rt->depth_buffer && rt->depth_format != F_DEPTH_NONE) {
    assert(
      rt->depth_format > F_DEPTH_NONE && rt->depth_format < F_DEPTH_FORMAT_MAX
    );

    GLenum depth_format = _rt_depth_formats[rt->depth_format];
    GLenum depth_attachment = _rt_depth_attachments[rt->depth_format];

    glGenRenderbuffers(1, &rt->depth_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, rt->depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, depth_format, screen.w, screen.h);

    glFramebufferRenderbuffer(
      GL_FRAMEBUFFER, depth_attachment, GL_RENDERBUFFER, rt->depth_buffer
    );
  }

  // textures
  if (!rt->textures) {
    rt->textures = (texture_t*)(rt + 1);
    index_t depth_binds = 0;

    for (index_t i = 0; i < rt->slot_count; ++i) {
      rt->textures[i] = tex_generate(rt->formats[i], screen);

      // handle texture-based depth attachment
      if (rt->formats[i] >= TF_DEPTH_32) {
        glFramebufferTexture
        ( GL_FRAMEBUFFER
        , GL_DEPTH_ATTACHMENT
        , rt->textures[i].handle
        , 0
        );
        ++depth_binds;
      }
      else {
        glFramebufferTexture
        ( GL_FRAMEBUFFER
        , GL_COLOR_ATTACHMENT0 + (GLenum)(i - depth_binds)
        , rt->textures[i].handle
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
    rt->ready = true;
  }
  else {
    str_log("[RenderTarget.build] Failed with error: 0x{!x}", status);
    rt->ready = false;
  }

  return rt->ready;
}

////////////////////////////////////////////////////////////////////////////////
// Clears out textures and the depth buffer from this render target
////////////////////////////////////////////////////////////////////////////////

void rt_clear(RenderTarget rt_in) {
  RT_INTERNAL;

  if (rt->handle) {
    glDeleteFramebuffers(1, &rt->handle);
    rt->handle = 0;
  }

  if (rt->depth_buffer) {
    glDeleteRenderbuffers(1, &rt->depth_buffer);
    rt->depth_buffer = 0;
  }

  if (rt->textures) {
    for (index_t i = 0; i < rt->slot_count; ++i) {
      tex_free(&rt->textures[i]);
    }
    rt->textures = NULL;
  }

  rt->ready = false;
}

////////////////////////////////////////////////////////////////////////////////
// Set the render target as the active target
////////////////////////////////////////////////////////////////////////////////

void rt_bind(RenderTarget rt_in) {
  RT_INTERNAL;
  if (!rt->ready) {
    str_write("[RenderTarget.bind] Target not ready");
    return;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, rt->handle);

  color3 c = rt->clear_color;
  glClearColor(c.r, c.g, c.b, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

////////////////////////////////////////////////////////////////////////////////
// Binds the default backbuffer/screen
////////////////////////////////////////////////////////////////////////////////

void rt_bind_default(void) {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glClearColor(0.2f, 0.2f, 0.2f, 1);
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
    rt_clear(rt);
    rt_build(rt, screen);

    if (!rt->ready) {
      str_write("[RenderTarget.resize] Failed to resize");
    }
  }
  else {
    // Resize the default back-buffer if no specific target was given
    glViewport(0, 0, screen.w, screen.h);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Clears and deletes the render target
////////////////////////////////////////////////////////////////////////////////

void rt_delete(RenderTarget* rt_in) {
  if (!rt_in || !*rt_in) return;
  RenderTarget_Internal* rt = (RenderTarget_Internal*)*rt_in;
  rt_clear((RenderTarget)rt);
  free(rt);
  *rt_in = NULL;
}
