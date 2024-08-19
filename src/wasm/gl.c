#include "gl.h"

#include <string.h>

extern GLenum glGetError();
extern void   glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
extern void   glEnable(GLenum cap);
extern void   glDisable(GLenum cap);
extern void   glBlendFunc(GLenum sfactor, GLenum dfactor);

extern GLuint glCreateShader(GLuint shaderType);
extern void   js_glShaderSource(GLuint data_id, const char* src, GLint length);
void          glShaderSource(
  GLuint shader, GLsizei _, const GLchar** str, const GLint *length
) {
  js_glShaderSource(shader, *str, *length);
}
extern void   glCompileShader(GLuint data_id);
extern void   glDeleteShader(GLuint data_id);
extern int    js_glGetShaderParameter(GLuint data_id, GLenum pname);
void          glGetShaderiv(GLuint data_id, GLenum pname, GLint* params) {
  *params = js_glGetShaderParameter(data_id, pname);
}
extern GLuint glCreateProgram();
extern void   glAttachShader(GLuint program, GLuint shader);
extern void   glLinkProgram(GLuint program);
extern int    js_glGetProgramParameter(GLuint data_id, GLenum pname);
void          glGetProgramiv(GLuint program, GLenum pname, GLint* params) {
  *params = js_glGetProgramParameter(program, pname);
}
extern void   glUseProgram(GLuint program);
extern void   glDeleteProgram(GLuint program);

extern GLint  js_glGetUniformLocation(GLuint data_id, const char* nam, int len);
GLint         glGetUniformLocation(GLuint program, const GLchar* name) {
  return js_glGetUniformLocation(program, name, strlen(name));
}
extern void   glUniform1i(GLint loc, GLint v0);
extern void   glUniform4fv(GLint loc, GLsizei count, const GLfloat* value);
extern void   glUniformMatrix4fv(
                GLint loc, GLsizei count, GLboolean tpose, const GLfloat* mat);

extern GLuint js_glCreateBuffer();
void          glGenBuffers(GLsizei n, GLuint* buffers) {
  for (GLsizei i = 0; i < n; ++i) buffers[i] = js_glCreateBuffer();
}
extern void   glBindBuffer(GLenum target, GLuint buffer);
extern void   glBufferData(
                GLenum target, GLsizeiptr size, const void* src, GLenum usage);
extern void   js_glDeleteBuffer(int data_id);
void          glDeleteBuffers(GLsizei n, const GLuint* buffers) {
  for (GLsizei i = 0; i < n; ++i) js_glDeleteBuffer(buffers[i]);
}

extern GLuint js_glCreateVertexArray();
void          glGenVertexArrays(GLsizei n, GLuint* arrays) {
  for (GLsizei i = 0; i < n; ++i) arrays[i] = js_glCreateVertexArray();
}
extern void   glBindVertexArray(GLuint array);
extern void   js_glDeleteVertexArray(int data_id);
void          glDeleteVertexArrays(GLsizei n, const GLuint* arrays) {
  for (GLsizei i = 0; i < n; ++i) js_glDeleteVertexArray(arrays[i]);
}
extern void   glVertexAttribPointer(
                GLuint index, GLint size, GLenum type, GLboolean normalized,
                GLsizei stride, const void* pointer);
extern void   glEnableVertexAttribArray(GLuint index);
extern void   glDisableVertexAttribArray(GLuint index);
extern void   glDrawArrays(GLenum mode, GLint first, GLsizei count);
extern void   glDrawElements(
                GLenum mode, GLsizei count, GLenum type, const void* offset);

extern int    js_glCreateTexture();
void          glGenTextures(GLsizei n, GLuint* textures) {
  for (GLsizei i = 0; i < n; ++i) { textures[i] = js_glCreateTexture(); }
}

extern void   glActiveTexture(GLenum texture);
extern void   glBindTexture(GLenum target, GLuint texture);
extern void   js_glTexImage2D(
                GLenum target, GLint level, GLint internalFormat, GLenum format,
                GLenum type, const void* image_id);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
void          glTexImage2D(
                GLenum target, GLint level, GLint internalFormat,
                GLsizei _width, GLsizei _height, GLint _border, GLenum format,
                GLenum type, const void* data
) {
  js_glTexImage2D(target, level, internalFormat, format, type, data);
}
#pragma clang diagnostic pop
extern void   glGenerateMipmap(GLenum target);
extern void   glTexParameteri(GLenum target, GLenum pname, GLint param);
extern void   glPixelStorei(GLenum pname, GLint param);
extern void   js_glDeleteTexture(GLuint data_id);
void          glDeleteTextures(GLsizei n, const GLuint* textures) {
  for (GLsizei i = 0; i < n; ++i) js_glDeleteTexture(textures[i]);
}
