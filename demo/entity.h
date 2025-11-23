#ifndef _ENTITY_H_
#define _ENTITY_H_

#include "mat.h"
#include "model.h"
#include "texture.h"
#include "shader.h"
#include "array.h"

typedef struct Game Game;
typedef struct Entity Entity;
typedef void (*UpdateFn)(Entity* e, Game* game, float dt);
typedef void (*RenderFn)(Entity* e, Game* game);
typedef void (*DeleteFn)(Entity* e);

// todo: if "Entity" is not partially opaque, should it be "entity" instead?
//          should it just be opaque? Should entities be created via a prefab
//          or entity_builder type object (want a way to define them inline in
//          levels and whatnot of course).
typedef struct Entity {
  uint type;

  vec3 pos;

  union {
    quat rotation;
    float angle;
  };

  mat4 transform;
  const ShaderProgram* shader;
  const Model* model;
  texture_t texture;

  bool hidden;

  RenderFn render;
  UpdateFn behavior;
  DeleteFn delete;

} Entity;


#define con_type Entity
#define con_prefix entity
#include "span.h"
#include "array.h"
#undef con_type
#undef con_prefix

#endif
