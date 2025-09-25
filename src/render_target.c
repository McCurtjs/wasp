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

render_target_t rt_setup(vec2i screen) {
  // Set up frame buffer object
  render_target_t target;
  glGenFramebuffers(1, &target.handle);
  glBindFramebuffer(GL_FRAMEBUFFER, target.handle);

  // Set up depth buffer for frame buffer
  glGenRenderbuffers(1, &target.depth_buffer);
  glBindRenderbuffer(GL_RENDERBUFFER, target.depth_buffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screen.w, screen.h);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target.depth_buffer);

  // Create texture target and assign to color slot 0
  target.texture = tex_generate_blank(screen.w, screen.h);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target.texture.handle, 0);

  GLenum draw_buffer[] = { GL_COLOR_ATTACHMENT0 };
  glDrawBuffers(ARRAY_COUNT(draw_buffer), draw_buffer);

  // Set viewport size
  glViewport(0, 0, screen.w, screen.h);

  // Validate
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    str_log("Framebuffer didn't work");
    target.ready = FALSE;
  }

  target.ready = TRUE;
  return target;
}

void rt_apply(render_target_t target) {
  if (!target.ready) return;
  glBindFramebuffer(GL_FRAMEBUFFER, target.handle);
  glClearColor(0, 0, 0.8f, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void rt_apply_default(void) {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClearColor(0.2f, 0.2f, 0.2f, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void rt_delete(render_target_t* target) {
  rt_apply_default();
  glDeleteRenderbuffers(1, &target->depth_buffer);
  tex_free(&target->texture);
  glDeleteFramebuffers(1, &target->handle);
  *target = (render_target_t){ 0 };
}
