
#ifndef _WASP_SHADER_H_
#define _WASP_SHADER_H_

#include "types.h"
#include "file.h"

typedef struct _opaque_Shader {
  const slice_t name;
  const index_t uniforms;
  const bool ready;
}* Shader;

Shader  shader_new(slice_t name);
Shader  shader_new_load(slice_t name);
void    shader_load_async(Shader shader);
void    shader_build(Shader shader);
void    shader_bind(Shader shader);
void    shader_delete(Shader* shader);

int     shader_uniform_loc(Shader shader, const char* name);

void    shader_check_update(Shader shader);

#endif
