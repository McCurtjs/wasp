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

#ifndef WASP_ENTITY_H_
#define WASP_ENTITY_H_

#include "mat.h"
#include "model.h"
#include "texture.h"
#include "shader.h"
#include "array.h"
#include "material.h"

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
  vec3 tint;

  union {
    quat rotation;
    float angle;
  };

  mat4 transform;
  const Model* model;
  texture_t texture;
  Material material;

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
