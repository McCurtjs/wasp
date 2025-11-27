
#ifndef _WASP_SHADER_H_
#define _WASP_SHADER_H_

#include "types.h"
#include "file.h"

#define SHADER_VERTEX   0x8B31
#define SHADER_PIXEL    0x8B30

typedef int shaderloc;

typedef struct Shader {
  uint  handle;
  int   ready;
} Shader;

int  shader_build(Shader* s, int type, const char* buffer, uint buffer_length);
int  shader_build_from_file(Shader* s, File f);
void shader_delete(Shader* s);

typedef enum UniformSet {
  UNIFORMS_PVM = 0, // only default project * view * model
  UNIFORMS_PHONG    // world, lightPos, cameraPos, texSamp
} UniformSet;

typedef struct UniformLocsPhong {
  shaderloc world;
  shaderloc lightPos;
  shaderloc cameraPos;
  shaderloc sampler;
  shaderloc useVertexColor;
} UniformLocsPhong;

typedef struct ShaderUniformLocs {
  shaderloc projViewMod;
  union {
    UniformLocsPhong phong;
  };
} ShaderUniformLocs;

typedef struct ShaderProgram {
  uint  handle;
  int   ready;
  ShaderUniformLocs uniform;
} ShaderProgram;

void shader_program_new(ShaderProgram* program);
int  shader_program_build(ShaderProgram* program, Shader* vert, Shader* frag);
int  shader_program_build_basic(ShaderProgram* program);
int  shader_program_build_frame(ShaderProgram* program);
int  shader_program_uniform_location(ShaderProgram* program, const char* name);
void shader_program_load_uniforms(ShaderProgram* program, UniformSet set);
void shader_program_use(const ShaderProgram* program);
void shdaer_program_delete(ShaderProgram* program);

////////////////////////////////////////////////////////////////////////////////

typedef struct _opaque_Shader {
  const slice_t name;
  const index_t uniforms;
  const bool ready;
}* ShaderX;

ShaderX shader_new(slice_t name);
ShaderX shader_new_load(slice_t name);
void    shader_load_async(ShaderX shader);
void    shader_deletex(ShaderX* shader);


void    shader_check_updates(void);

#endif
