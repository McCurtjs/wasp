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

#include "gl.h"
#include "types.h"

#include <string.h>

extern GLenum glGetError();

extern int js_glGetParameter(GLenum);
void glGetIntegerv(GLenum pname, GLint* data) {
  *data = js_glGetParameter(pname);
}

extern void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

extern void glEnable(GLenum cap);

extern void glDisable(GLenum cap);

extern void glBlendFunc(GLenum sfactor, GLenum dfactor);

extern void glClear(GLbitfield mask);

extern void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);

////////////////////////////////////////////////////////////////////////////////
// Shaders
////////////////////////////////////////////////////////////////////////////////

extern GLuint glCreateShader(GLuint shaderType);

extern void js_glShaderSource(GLuint data_id, const char* src, GLint length);
void glShaderSource(
  GLuint shader, GLsizei _, const GLchar** str, const GLint *length
) {
  js_glShaderSource(shader, *str, *length);
}

extern void glCompileShader(GLuint data_id);

extern void glDeleteShader(GLuint data_id);

extern int js_glGetShaderParameter(GLuint data_id, GLenum pname);
void glGetShaderiv(GLuint data_id, GLenum pname, GLint* params) {
  *params = js_glGetShaderParameter(data_id, pname);
}

extern int js_glGetShaderInfoLog(GLuint data_id, GLsizei len, char* out_log);
void glGetShaderInfoLog(
  GLuint shader, GLsizei maxLength, GLsizei *length, GLchar* infoLog
) {
  *length = js_glGetShaderInfoLog(shader, maxLength, infoLog);
}

extern GLuint glCreateProgram(void);

extern void glAttachShader(GLuint program, GLuint shader);

extern void glLinkProgram(GLuint program);

extern int js_glGetProgramParameter(GLuint data_id, GLenum pname);
void glGetProgramiv(GLuint program, GLenum pname, GLint* params) {
  *params = js_glGetProgramParameter(program, pname);
}

extern int js_glGetProgramInfoLog(GLuint data_id, GLsizei len, char* out_log);
void glGetProgramInfoLog(
  GLuint program, GLsizei maxlength, GLsizei* length, GLchar* infoLog
) {
  *length = js_glGetProgramInfoLog(program, maxlength, infoLog);
}

extern void glUseProgram(GLuint program);

extern void glDeleteProgram(GLuint program);

extern GLint js_glGetAttribLocation(GLuint data_id, const char* name, int len);
GLint glGetAttribLocation(GLuint program, const GLchar* name) {
  return js_glGetAttribLocation(program, name, strlen(name));
}

extern GLint js_glGetUniformLocation(GLuint data_id, const char* nam, int len);
GLint glGetUniformLocation(GLuint program, const GLchar* name) {
  return js_glGetUniformLocation(program, name, strlen(name));
}

////////////////////////////////////////////////////////////////////////////////
// Shader uniforms
////////////////////////////////////////////////////////////////////////////////

extern void glUniform1i(GLint loc, GLint v0);

extern void glUniform2fv(GLint loc, GLsizei count, const GLfloat* value);

extern void glUniform3fv(GLint loc, GLsizei count, const GLfloat* value);

extern void glUniform4fv(GLint loc, GLsizei count, const GLfloat* value);

extern void glUniformMatrix4fv(
  GLint loc, GLsizei count, GLboolean tpose, const GLfloat* mat
);

////////////////////////////////////////////////////////////////////////////////
// Buffers and VAO
////////////////////////////////////////////////////////////////////////////////

extern GLuint js_glCreateBuffer();
void glGenBuffers(GLsizei n, GLuint* buffers) {
  for (GLsizei i = 0; i < n; ++i) buffers[i] = js_glCreateBuffer();
}

extern void glBindBuffer(GLenum target, GLuint buffer);

extern void glBufferData(
  GLenum target, GLsizeiptr size, const void* src, GLenum usage
);

extern void glBufferSubData(
  GLenum target, GLintptr offset, GLsizeiptr size, const void* data
);

extern void js_glDeleteBuffer(int data_id);
void glDeleteBuffers(GLsizei n, const GLuint* buffers) {
  for (GLsizei i = 0; i < n; ++i) js_glDeleteBuffer(buffers[i]);
}

extern GLuint js_glCreateVertexArray();
void glGenVertexArrays(GLsizei n, GLuint* arrays) {
  for (GLsizei i = 0; i < n; ++i) arrays[i] = js_glCreateVertexArray();
}
extern void glBindVertexArray(GLuint array);

extern void js_glDeleteVertexArray(int data_id);
void glDeleteVertexArrays(GLsizei n, const GLuint* arrays) {
  for (GLsizei i = 0; i < n; ++i) js_glDeleteVertexArray(arrays[i]);
}

extern void glVertexAttribPointer(
  GLuint index, GLint size, GLenum type, GLboolean normalized,
  GLsizei stride, const void* pointer
);

extern void glEnableVertexAttribArray(GLuint index);

extern void glDisableVertexAttribArray(GLuint index);

extern void glVertexAttribDivisor(GLuint index, GLuint divisor);

extern void glDrawArrays(
  GLenum mode, GLint first, GLsizei count
);

extern void glDrawElements(
  GLenum mode, GLsizei count, GLenum type, const void* offset
);

extern void glDrawArraysInstanced(
  GLenum mode, GLint first, GLsizei count, GLsizei primcount
);

extern void glDrawElementsInstanced(
  GLenum mode, GLsizei, GLenum type, const void* offset, GLsizei primcount
);

////////////////////////////////////////////////////////////////////////////////
// Textures
////////////////////////////////////////////////////////////////////////////////

extern int js_glCreateTexture();
void glGenTextures(GLsizei n, GLuint* textures) {
  for (GLsizei i = 0; i < n; ++i) { textures[i] = js_glCreateTexture(); }
}

extern void glActiveTexture(GLenum texture);

extern void glBindTexture(GLenum target, GLuint texture);

extern void js_glTexImage2D(
  GLenum target, GLint level, GLint internalFormat,
  GLsizei width, GLsizei height,
  GLenum format, GLenum type, const void* image_id
);
void glTexImage2D(
  GLenum target, GLint level, GLint inFormat,
  GLsizei width, GLsizei height, GLint _border, GLenum format,
  GLenum type, const void* data
) {
  UNUSED(_border);
  js_glTexImage2D(target, level, inFormat, width, height, format, type, data);
}

extern void glTexImage3D(
  GLenum target, GLint level, GLint internalformat,
  GLsizei width, GLsizei height, GLsizei depth,
  GLint border, GLenum format, GLenum type, const void* data
);

extern void glTexStorage3D(
  GLenum target, GLsizei levels, GLenum internalformat,
  GLsizei width, GLsizei height, GLsizei depth);

extern void glTexSubImage3D(
  GLenum target, GLint level,
  GLint xoffset, GLint yoffset, GLint zoffset,
  GLsizei width, GLsizei height, GLsizei depth,
  GLenum format, GLenum type, const void * pixels);

extern void glGenerateMipmap(GLenum target);

extern void glTexParameteri(GLenum target, GLenum pname, GLint param);

extern void glPixelStorei(GLenum pname, GLint param);

extern void js_glDeleteTexture(GLuint data_id);
void glDeleteTextures(GLsizei n, const GLuint* textures) {
  for (GLsizei i = 0; i < n; ++i) js_glDeleteTexture(textures[i]);
}

////////////////////////////////////////////////////////////////////////////////
// Render targets
////////////////////////////////////////////////////////////////////////////////

extern GLuint js_glCreateFramebuffer(void);
void glGenFramebuffers(GLsizei n, GLuint* framebuffers) {
  for (GLsizei i = 0; i < n; ++i) framebuffers[i] = js_glCreateFramebuffer();
}

extern void glBindFramebuffer(GLenum target, GLuint framebuffer);

extern GLenum glCheckFramebufferStatus(GLenum target);

extern void glFramebufferTexture2D(
  GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level
);

void glFramebufferTexture(
  GLenum target, GLenum attachment, GLuint texture, GLint level
) {
  glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, texture, level);
}

extern void glFramebufferRenderbuffer(
  GLenum target, GLenum attachment, GLenum renderbuftarget, GLuint renderbuffer
);

extern void js_glDeleteFramebuffer(int data_id);

void glDeleteFramebuffers(GLsizei n, const GLuint* buffers) {
  for (GLsizei i = 0; i < n; ++i) js_glDeleteFramebuffer(buffers[i]);
}

extern int js_glCreateRenderbuffer(void);
extern void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers) {
  for (GLsizei i = 0; i < n; ++i) renderbuffers[i] = js_glCreateRenderbuffer();
}

extern void glBindRenderbuffer(GLenum target, GLuint renderbuffer);

extern void glRenderbufferStorage(
  GLenum target, GLenum format, GLsizei width, GLsizei height
);

extern void js_glDeleteRenderbuffer(int data_id);
void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers) {
  for (GLsizei i = 0; i < n; ++i) js_glDeleteRenderbuffer(renderbuffers[i]);
}

void glDrawBuffers(GLsizei n, const GLenum* bufs);

