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

#ifndef WASP_PARTICLES_H_
#define WASP_PARTICLES_H_

#include "types.h"
#include "vec.h"
#include "slice.h"
#include "array_slice.h"
#include "slotkey.h"
#include "instance_attributes.h"
#include "camera.h"

typedef enum effect_flags_t {
  EF_DEFAULT      = 0,
  EF_BILLBOARD    = 0b0000'0001,
  EF_TRANSLUCENT  = 0b0000'0010,
  EF_MODEL        = 0b0000'0100,
  EF_COLOR        = 0b0000'1000,
  EF_RIBBON       = 0b0001'0000
} effect_flags_t;

typedef enum particle_format_t {
  // particle_inst_t:
  //    all-round particle, pos, quat-rotation, non-uniform scale, color
  PF_DEFAULT,

  // particle_inst_point_t: 3d position with uniform scale
  PF_POINT,

  PF_SUPPORTED_MAX
} particle_format_t;

typedef enum emitter_shape_t {
  ES_POINT,
  ES_SPHERE,
  ES_BOX_ALIGNED,
  ES_BOX,
  ES_DISC,
  ES_SHAPE_COUNT
} emitter_shape_t;

typedef struct particle_inst_t {
  vec3  pos;
  vec2  scale;
  quat  rot;
  vec4b color;
} particle_inst_t;

typedef struct particle_inst_point_t {
  vec3  pos;
  float scale;
} particle_inst_point_t;

typedef struct particle_inst_billboard_t {
  vec3  pos;
  float angle;
  vec2  scale;
} particle_inst_billboard_t;

typedef struct particle_inst_rot_t {
  vec3  pos;
  quat  rot;
} particle_inst_rot_t;

typedef struct particle_inst_color_t {
  vec3  pos;
  vec4b color;
} particle_inst_color_t;

typedef struct particle_inst_rot_color_t {
  vec3  pos;
  vec4b color;
  quat  rot;
} particle_inst_rot_color_t;

typedef struct particle_base_t {
  vec3  vel;
  float age;
  float duration;
  // slotkey_t force_id;
} particle_base_t;

typedef struct particle_t {
  particle_base_t*                base;
  union {
    particle_inst_t*              inst;
    particle_inst_point_t*        inst_point;
    particle_inst_rot_t*          inst_rot;
    particle_inst_color_t*        inst_color;
    particle_inst_rot_color_t*    inst_rot_color;
  };
} particle_t;

typedef struct _opaque_Shader_t*  Shader;
typedef struct texture_t*         Texture;
typedef struct _opaque_Model_t*   Model;

// \brief Manages all particles of a given type and its respective emitters
//
// \brief Position and other instance data is stored separately from other
//    particle data so it can be uploaded and rendered as one batch draw call
//
// \brief Should the emitter own the particle data? Combining all the particles
//    in the effect could have issues with transparency if multiple effects
//    are running at the same time (since it will only be sorted within one
//    effect).
typedef struct _opaque_ParticleEffect_t* ParticleEffect;

typedef struct _opaque_ParticleSystem_t* ParticleSystem;

typedef void (particle_behavior_fn_t)(ParticleEffect, particle_t, float dt);

typedef struct particle_defaults_t {
  float duration;
  float speed;
  float size;
  vec4b color;
  vec4b color_end;
} particle_defaults_t;

typedef struct emitter_defaults_t {
  vec3                pos;
  quat                dir;
  emitter_shape_t     shape;
  vec3                size;
  index_t             budget;
  float               rate;
  float               rate_variance;
  float               duration;
  float               offset;
  bool                color_blend;

  particle_defaults_t particle_defaults;
  particle_defaults_t particle_variance;
} emitter_defaults_t;

// \brief Emitters store the parameters for spawning new particles
//
// \brief When the emitter is finished, it will be removed but its particles
//    will continue in the effect until they expire
typedef struct particle_emitter_t {
  slotkey_t         CONST id;
  slotkey_t               entity_id;
  ParticleEffect    CONST effect;
  index_t           CONST count;
  float             CONST spawn_timer;
  float                   age;

  union {
    emitter_defaults_t    defaults;
    struct {
      vec3                pos;
      quat                dir;
      emitter_shape_t     shape;
      union {
        vec3              size;
        struct {
          float           radius;
          float           inner_radius;
        };
      };
      index_t             budget;
      float               rate;
      float               rate_variance;
      float               duration;
      float               offset;
      bool                color_blend;

      particle_defaults_t particle_defaults;
      particle_defaults_t particle_variance;
    };
  };
}* ParticleEmitter;

// Emitter definition
struct _opaque_ParticleEffect_t {
  slice_t             CONST name;
  effect_flags_t      CONST flags;
  particle_format_t   CONST format;
  ParticleSystem      CONST system;

  Shader              shader;
  Texture             texture;
  Model               model;

  emitter_defaults_t  emitter_defaults;

  void (*on_emitter_create)   (ParticleEmitter);
  void (*on_emitter_destroy)  (ParticleEmitter);
  void (*on_particle_create)  (ParticleEmitter, particle_t);
  void (*on_particle_update)  (ParticleEffect , particle_t, float dt);
  void (*on_particle_destroy) (ParticleEffect , particle_t);
};

// \brief Particle system type that manages a number of effects. Only one
//    system is needed for the game, probably.
struct _opaque_ParticleSystem_t {
  slice_t CONST name;
  index_t CONST effect_count;
};

ParticleSystem  ps_new(void);
void            ps_update(ParticleSystem, float dt);
void            ps_render(ParticleSystem, camera_t*);
void            ps_reset(ParticleSystem);
void            ps_delete(ParticleSystem*);

ParticleEffect  ps_add_effect(ParticleSystem,
                  slice_t name, particle_format_t, effect_flags_t);
ParticleEffect  ps_get_effect(ParticleSystem, slice_t name);
Array_slice     ps_get_effect_names(ParticleSystem);
bool            ps_set_effect_name(ParticleEffect, slice_t);

ParticleEmitter ps_add_emitter(ParticleEffect);
ParticleEmitter ps_get_emitter(ParticleSystem, slotkey_t emitter_id);
ParticleEmitter ps_get_next_emitter(ParticleSystem, slotkey_t* emitter_id_iter);

// \brief basic particle behaviors
particle_behavior_fn_t pb_gravity;

#endif
