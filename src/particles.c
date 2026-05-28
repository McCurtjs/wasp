/*******************************************************************************
* MT License
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
#include "particles.h"
#include "str.h"
#include "shader.h"
#include "model.h"
#include "gl.h"
#include "utility.h"
#include "quat.h"
#include "entity.h"

#include <stdlib.h>

typedef struct particle_format_desc_t {
  int size;
} particle_format_desc_t;

static const particle_format_desc_t _particle_format[PF_SUPPORTED_MAX] = {
  {.size = sizeof(particle_inst_t) },
  {.size = sizeof(particle_inst_point_t) }
};

////////////////////////////////////////////////////////////////////////////////
// Particle Effect
////////////////////////////////////////////////////////////////////////////////

#define con_type particle_base_t
#define con_prefix particle
#include "array.h"
#undef con_prefix
#undef con_type

typedef struct ParticleEffect_Internal {
  struct _opaque_ParticleEffect_t pub;

  String          name_internal;
  Array_particle  metadata;
  Array           instances;
  uint            vao;
  uint            vbo_instances;
  index_t         vbo_capacity;
} ParticleEffect_Internal;

////////////////////////////////////////////////////////////////////////////////
// Particle System
////////////////////////////////////////////////////////////////////////////////

#define con_type struct particle_emitter_t
#define con_prefix emitter
#include "slotmap.h"
#undef con_prefix
#undef con_type

#define con_type ParticleEffect_Internal*
#define con_prefix effect
#include "map.h"
#undef con_prefix
#undef con_type

typedef struct ParticleSystem_Internal {
  struct _opaque_ParticleSystem_t pub;

  String          name_internal;
  SlotMap_emitter emitters;
  HMap_effect     effects;
} ParticleSystem_Internal;

////////////////////////////////////////////////////////////////////////////////

ParticleSystem ps_new(void) {
  ParticleSystem_Internal* ret = malloc(sizeof(*ret));
  assert(ret);

  *ret = (ParticleSystem_Internal) {
    .pub = {
      .effect_count = 0,
    },
    .emitters = smap_emitter_new(),
    .effects = map_effect_new(),
  };

  return (ParticleSystem)ret;
}

////////////////////////////////////////////////////////////////////////////////
// Particle update and creation
////////////////////////////////////////////////////////////////////////////////

void _effect_update(ParticleEffect_Internal* effect, float dt) {
  assert(effect);
  assert(effect->instances->size == effect->metadata->size);

  particle_t particle = {
    .base = effect->metadata->begin,
    .inst = effect->instances->begin,
  };

  for (index_t i = 0; i < effect->instances->size;) {
    particle.base->age += dt;

    if (particle.base->age > particle.base->duration) {

      if (effect->pub.on_particle_destroy) {
        effect->pub.on_particle_destroy((ParticleEffect)effect, particle);
      }

      arr_remove_unstable(effect->instances, i);
      arr_particle_remove_unstable(effect->metadata, i);
      continue;
    }

    v3add_eq(&particle.inst->pos, v3scale(particle.base->vel, dt));

    if (effect->pub.on_particle_update) {
      effect->pub.on_particle_update((ParticleEffect)effect, particle, dt);
    }

    // increment here to avoid skipping particles when removed
    ++i;
    particle.base += 1;
    particle.inst = arr_ref_unchecked(effect->instances, i);
  }
}

////////////////////////////////////////////////////////////////////////////////

void _emitter_create_particle(ParticleEmitter emitter, particle_t particle) {
  vec3 pos = emitter->pos;
  /*
  mat4 M = m4q(emitter->dir);
  vec3 dir = mv4mul(M, v4f(v3front.x, v3front.y, v3front.z, 0)).xyz;
  /*/
  vec3 dir = q4dir(emitter->dir);
  //*/

  // Select starting location offset within emitter volume
  switch (emitter->shape) {
    case ES_POINT: break;

    case ES_SPHERE:
      v3add_eq(&pos, v3rand_dumb(emitter->radius));
    break;

    case ES_BOX_ALIGNED:
      pos = v3mul(v3rand_box(), emitter->size);
    break;

    case ES_BOX: {
      vec3 right = q4right(emitter->dir);
      vec3 up = q4up(emitter->dir);
      v3add_eq(&pos, v3scale(right, emitter->size.x * (frand() * 2.f - 1.f)));
      v3add_eq(&pos, v3scale(up,    emitter->size.y * (frand() * 2.f - 1.f)));
      v3add_eq(&pos, v3scale(dir,   emitter->size.z * (frand() * 2.f - 1.f)));
    } break;

    case ES_DISC: {
      vec3 right = q4right(emitter->dir);
      vec3 up = q4up(emitter->dir);
      vec2 disc = v2scale(v2rand_dir(), emitter->radius);
      v3add_eq(&pos, v3scale(right, disc.x));
      v3add_eq(&pos, v3scale(up, disc.y));
    } break;

    default: break;
  }

  // Select starting velocity based on emitter direction
  if (emitter->offset != 0) {
    dir = v3rand_cone(emitter->offset);
    dir = v3rotate(dir, emitter->dir);
  }

  float duration = emitter->particle_defaults.duration;
  if (emitter->particle_variance.duration > 0) {
    duration += (frand() * 2.f - 1) * emitter->particle_variance.duration;
  }

  float speed = emitter->particle_defaults.speed;
  if (emitter->particle_variance.speed > 0) {
    speed += (frand() * 2.f - 1.f) * emitter->particle_variance.speed;
  }

  *particle.base = (particle_base_t) {
    .age = 0,
    .duration = duration,
    .vel = v3scale(dir, speed),
  };

  float size = emitter->particle_defaults.size;
  if (emitter->particle_variance.size > 0) {
    size += (frand() * 2.f - 1.f) * emitter->particle_variance.size;
    if (size < 0.0001f) size = 0.0001f;
  }

  particle.inst_point->pos = pos;
  particle.inst_point->scale = size;
}

////////////////////////////////////////////////////////////////////////////////

bool _emitter_update(ParticleEmitter emitter, float dt) {
  assert(emitter);

  ParticleEffect_Internal* effect = (ParticleEffect_Internal*)emitter->effect;

  // Don't deduct from the timer if it's exactly 0, that is a special value
  //    for an emitter that doesn't expire.
  if (emitter->duration > 0) {
    emitter->age += dt;

    if (emitter->age > emitter->duration) {
      if (effect->pub.on_emitter_destroy) {
        effect->pub.on_emitter_destroy(emitter);
      }

      // return false indicating it should be removed
      return false;
    }
  }

  // If an entity id was given to this emitter, update its position to match
  if (emitter->entity_id.hash) {
    Entity entity = entity_ref(emitter->entity_id);
    if (!entity) {
      emitter->entity_id = SK_NULL;
    }
    else {
      emitter->pos = entity->pos;
      emitter->dir = entity->rot;
    }
  }

  // create particles based on the rate and generation properties
  emitter->spawn_timer += emitter->rate * dt;

  index_t to_emit = (index_t)emitter->spawn_timer;
  emitter->spawn_timer -= (float)to_emit;

  if (to_emit > 0) {
    span_t base = arr_particle_emplace_back_range(effect->metadata, to_emit);
    span_t inst = arr_emplace_back_range(effect->instances, to_emit);

    for (index_t i = 0; i < to_emit; ++i) {
      particle_t particle = {
        .base = span_ref(base, i, effect->metadata->element_size),
        .inst = span_ref(inst, i, effect->instances->element_size),
      };

      _emitter_create_particle(emitter, particle);

      if (effect->pub.on_particle_create) {
        effect->pub.on_particle_create(emitter, particle);
      }
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

void ps_update(ParticleSystem _ps, float dt) {
  assert(_ps);
  ParticleSystem_Internal* ps = (ParticleSystem_Internal*)_ps;

  ParticleEffect_Internal** map_foreach(peffect, ps->effects) {
    _effect_update(*peffect, dt);
  }

  ParticleEmitter smap_foreach(emitter, ps->emitters) {
    if (!_emitter_update(emitter, dt)) {
      smap_emitter_remove(ps->emitters, emitter->id);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Effect rendering
////////////////////////////////////////////////////////////////////////////////

void _effect_render(ParticleEffect_Internal* effect, camera_t* camera) {
  assert(effect);

  if (effect->instances->size <= 0) return;

  // Use provided shader or enable base shader
  if (!effect->pub.shader) {
    effect->pub.shader = shader_new_from_default(S("particle"));
  }
  if (effect->pub.shader->status != S_READY) {
    return;
  }
  shader_bind(effect->pub.shader);

  // Bind the material if provided (NULL is ok)
  // TODO

  index_t element_size = effect->instances->element_size;

  if (!effect->vao) {
    // Use provided model or get the base billboard particle
    if (!effect->pub.model) {
      effect->pub.model = model_new_primitive(MODEL_PARTICLE);
    }

    glGenVertexArrays(1, &effect->vao);
    glBindVertexArray(effect->vao);

    model_bind(effect->pub.model);

    effect->vbo_capacity = effect->instances->size * element_size;

    glGenBuffers(1, &effect->vbo_instances);
    glBindBuffer(GL_ARRAY_BUFFER, effect->vbo_instances);
    glBufferData(GL_ARRAY_BUFFER
    , effect->instances->size_bytes
    , effect->instances->begin
    , GL_DYNAMIC_DRAW
    );

    // TODO: replace hard-coded attribute format
    attribute_bind(AF_PARTICLE_POINT, effect->pub.shader);
  }
  else {
    glBindBuffer(GL_ARRAY_BUFFER, effect->vbo_instances);

    if (effect->instances->size_bytes > effect->vbo_capacity) {
      effect->vbo_capacity = effect->instances->size * element_size;
      glBufferData(GL_ARRAY_BUFFER, effect->vbo_capacity, NULL, GL_STREAM_DRAW);
    }

    glBufferSubData(GL_ARRAY_BUFFER
    , 0
    , effect->instances->size_bytes
    , effect->instances->begin
    );

    glBindVertexArray(effect->vao);
  }

  assert(effect->pub.model);

  int loc_proj_view = shader_uniform_loc(effect->pub.shader, "in_pv_matrix");
  glUniformMatrix4fv(loc_proj_view, 1, 0, camera->projview.f);

  model_render_instanced(effect->pub.model, effect->instances->size);
  glBindVertexArray(0);
}

////////////////////////////////////////////////////////////////////////////////

void ps_render(ParticleSystem _ps, camera_t* camera) {
  assert(_ps);
  ParticleSystem_Internal* ps = (ParticleSystem_Internal*)_ps;

  ParticleEffect_Internal** map_foreach(peffect, ps->effects) {
    _effect_render(*peffect, camera);
  }
}

////////////////////////////////////////////////////////////////////////////////
// System and Effect management and creation
////////////////////////////////////////////////////////////////////////////////

void ps_delete(ParticleSystem* system) {
  UNUSED(system);
}

////////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
// Disable warning for the return line:
//    C33010: "Unchecked lower bound for enum format used as index"
// Obviously the bound is being checked in the assert...
# pragma warning ( disable : 33010 )
#endif

ParticleEffect ps_add_effect(
  ParticleSystem _ps, slice_t name, particle_format_t fmt, effect_flags_t flags
) {
  assert(_ps);
  assert(slice_is_valid(name));
  assert(fmt >= 0 && fmt < PF_SUPPORTED_MAX);

  ParticleSystem_Internal* ps = (ParticleSystem_Internal*)_ps;

  ParticleEffect_Internal* effect =
    map_effect_get_or_default(ps->effects, name, NULL);
  if (effect) return (ParticleEffect)effect;

  effect = malloc(sizeof(*effect));
  assert(effect);

  String name_copy = str_copy(name);

  *effect = (ParticleEffect_Internal) {
    .pub = {
      .name = name_copy->slice,
      .flags = flags,
      .format = fmt,
      .system = _ps,
      .emitter_defaults = {
        .pos = svNzero,
        .dir = q4identity,
        .shape = ES_POINT,
        .budget = 0,
        .rate = 10,
        .duration = 1,
        .particle_defaults = {
          .duration = 1,
          .speed = 1,
          .size = 1,
          .color = sb4white,
          .color_end = sb4white,
        }
      }
    },
    .name_internal = name_copy,
    .instances = iarr_new(sizeof(attribute_particle_point_t)),//_particle_format[fmt].size),
    .metadata = arr_particle_new(),
  };

  map_effect_insert(ps->effects, name_copy->slice, effect);

  return (ParticleEffect)effect;
}

////////////////////////////////////////////////////////////////////////////////

ParticleEffect ps_get_effect(ParticleSystem _ps, slice_t name) {
  assert(_ps);
  assert(slice_is_valid(name));

  ParticleSystem_Internal* ps = (ParticleSystem_Internal*)_ps;
  return (ParticleEffect)map_effect_get_or_default(ps->effects, name, NULL);
}

////////////////////////////////////////////////////////////////////////////////
// Emitter Functionality
////////////////////////////////////////////////////////////////////////////////

ParticleEmitter ps_add_emitter(ParticleEffect _effect) {
  assert(_effect);

  ParticleEffect_Internal* effect = (ParticleEffect_Internal*)_effect;
  ParticleSystem_Internal* system = (ParticleSystem_Internal*)_effect->system;

  slotkey_t key;
  ParticleEmitter ret = smap_emitter_emplace(system->emitters, &key);

  *ret = (struct particle_emitter_t) {
    .id = key,
    .effect = _effect,
    .count = 0,
    .spawn_timer = 0,
    .age = 0,
    .defaults = _effect->emitter_defaults,
  };

  if (effect->pub.on_emitter_create) {
    effect->pub.on_emitter_create(ret);
  }

  return ret;
}

////////////////////////////////////////////////////////////////////////////////

ParticleEmitter ps_get_emitter(ParticleSystem _system, slotkey_t emitter_id) {
  assert(_system);

  ParticleSystem_Internal* system = (ParticleSystem_Internal*)_system;

  ParticleEmitter emitter = smap_emitter_ref(system->emitters, emitter_id);
  if (!emitter) return NULL;

  assert(emitter->id.hash == emitter_id.hash);
  assert(_system == emitter->effect->system);

  return emitter;
}

////////////////////////////////////////////////////////////////////////////////
// Basic particle behaviors
////////////////////////////////////////////////////////////////////////////////

void pb_gravity(ParticleEffect effect, particle_t particle, float dt) {
  UNUSED(effect);
  particle.base->vel.y -= 9.8f * dt;
}
