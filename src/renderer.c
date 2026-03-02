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

#define MCLIB_INTERNAL_IMPL
#include "renderer.h"
#include "game.h"
#include "gl.h"

////////////////////////////////////////////////////////////////////////////////
// Clears instance data and resets bookkeeping values
////////////////////////////////////////////////////////////////////////////////

void renderer_clear_instances(renderer_t* renderer) {
  if (renderer->groups) {
    render_group_t* map_foreach(group, renderer->groups) {
      group->update_range_low = -1;
      group->update_range_high = -1;
      group->update_full = false;
      if (group->instances) pmap_clear(group->instances);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Registers an entity with a renderer
////////////////////////////////////////////////////////////////////////////////

void renderer_entity_register(renderer_t* renderer, Entity entity) {
  assert(renderer);
  assert(entity);

  if (entity->render_id.hash) {
    if (entity->renderer) {
      if (entity->renderer == renderer) return;
      renderer_entity_unregister(entity);
    }
    entity->render_id = SK_NULL;
  }

  if (renderer->entity_register) {
    Game game = game_get_active();
    entity->renderer = renderer;
    entity->render_id = renderer->entity_register(entity, game);
  }

  if (!entity->render_id.hash) {
    assert(false); // failed to register with the renderer
    entity->renderer = NULL;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Updates the entity's instance data
////////////////////////////////////////////////////////////////////////////////

void renderer_entity_update(Entity entity) {
  assert(entity);
  renderer_t* renderer = entity->renderer;
  assert(renderer || !entity->render_id.hash);
  if (!renderer || !renderer->entity_update) return;
  slotkey_t new_id = renderer->entity_update(entity);
  if (new_id.hash != entity->render_id.hash) entity->render_id = new_id;
}

////////////////////////////////////////////////////////////////////////////////
// Removes an entity from its renderer
////////////////////////////////////////////////////////////////////////////////

void renderer_entity_unregister(Entity entity) {
  assert(entity);
  renderer_t* renderer = entity->renderer;
  assert(renderer || !entity->render_id.hash);
  if (!renderer) return;
  if (renderer->entity_unregister) {
    renderer->entity_unregister(entity);
  }
  entity->render_id = SK_NULL;
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// "Default" callback functions to use for renderers (still need to assign)
////////////////////////////////////////////////////////////////////////////////

void _renderer_callback_unregister_internal(
  Entity e, render_group_key_t key
) {
  render_group_t* group = map_rg_ref(e->renderer->groups, key);
  if (!group) return;
  pmap_remove(group->instances, e->render_id);
  group->update_full = true;
}

////////////////////////////////////////////////////////////////////////////////

static void _render_group_expand_update_range(
  render_group_t* group, int32_t index
) {
  if (group->update_range_low < 0) {
    group->update_range_low = index;
    group->update_range_high = index;
  }
  else if (index < group->update_range_low) {
    group->update_range_low = index;
  }
  else if (index > group->update_range_high) {
    group->update_range_high = index;
  }
}

////////////////////////////////////////////////////////////////////////////////

slotkey_t renderer_callback_entity_register(Entity e, Game game) {
  assert(e);
  assert(e->renderer);
  assert(e->renderer->groups);
  assert(e->model);
  UNUSED(game);

  // get associated render group
  render_group_key_t key = { e->model, e->material, !!e->is_static };
  res_ensure_rg_t group_slot = map_rg_ensure(e->renderer->groups, key);

  if (group_slot.is_new) {
    attribute_format_t attrib_format = e->renderer->shader->attrib_format;
    *group_slot.value = (render_group_t) {
      .instances = ipmap_new(attribute_size(attrib_format)),
      .material = e->material,
      .model = e->model,
      .is_static = e->is_static,
      .update_range_low = -1,
      .update_range_high = -1,
      .update_full = 1,
    };
  }

  // add instance to the group and save its instance id
  PackedMap instances = group_slot.value->instances;
  bool expanded = instances->size == instances->capacity;
  slotkey_t ret = pmap_insert(instances,
    &(attribute_base_t) {
      .transform = entity_transform(e),
    }
  );

  // flag the modified value range for update
  group_slot.value->update_full = true;
  if (expanded) {
    group_slot.value->update_full = true;
  }
  else {
    _render_group_expand_update_range(group_slot.value, sk_index(ret));
  }

  return ret;
}

////////////////////////////////////////////////////////////////////////////////

slotkey_t renderer_callback_entity_update(Entity e) {
  assert(e);
  assert(e->renderer);
  assert(e->renderer->shader);
  assert(e->render_id.hash);

  if (e->is_dirty_static) {
    render_group_key_t key = { e->model, e->material, !e->is_static };
    _renderer_callback_unregister_internal(e, key);
    return renderer_callback_entity_register(e, game_get_active());
  }

  render_group_key_t key = { e->model, e->material, !!e->is_static };
  render_group_t* group = map_rg_ref(e->renderer->groups, key);
  assert(group);
  assert(group->instances);

  attribute_base_t* att = pmap_ref(group->instances, e->render_id);
  assert(att);

  // Update default supported attribute values
  att->transform = entity_transform(e);

  _render_group_expand_update_range(group, sk_index(e->render_id));
  return e->render_id;
}

////////////////////////////////////////////////////////////////////////////////

void* renderer_callback_entity_attributes(Entity e, bool update) {
  assert(e);
  assert(e->renderer);
  if (e->is_hidden || !e->render_id.hash) return NULL;
  assert(e->render_id.hash);

  render_group_key_t key = { e->model, e->material, !!e->is_static };
  render_group_t* group = map_rg_ref(e->renderer->groups, key);
  assert(group);
  assert(group->instances);

  void* attributes = pmap_ref(group->instances, e->render_id);
  assert(attributes);

  if (update) {
    _render_group_expand_update_range(group, sk_index(e->render_id));
  }

  return attributes;
}

////////////////////////////////////////////////////////////////////////////////

void renderer_callback_entity_unregister(entity_t* e) {
  assert(e);
  render_group_key_t key = { e->model, e->material, e->is_static };
  _renderer_callback_unregister_internal(e, key);
}

////////////////////////////////////////////////////////////////////////////////

void renderer_callback_instance_update(render_group_t* group) {
  assert(group->update_range_low >= 0);
  assert(group->update_range_high >= 0);
  assert(group->update_range_low < group->instances->size);
  assert(group->update_range_high <= group->instances->size);
  //assert(group->instance_buffer);

  if (!group->instance_buffer) return;

  // adding one to the count because range_high is inclusive
  int32_t update_count = group->update_range_high - group->update_range_low + 1;

  glBindBuffer(GL_ARRAY_BUFFER, group->instance_buffer);

  // update all instances if there are lots of them
  // (will also be triggered if a full upload is required for expanding the set)
  if (update_count > group->instances->size / 2) {
    glBufferData(GL_ARRAY_BUFFER
    , group->instances->size_bytes
    , group->instances->begin
    , group->is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW
    );
  }
  // update a range if we only have a few changes
  else {
    glBufferSubData(GL_ARRAY_BUFFER
    , group->instances->element_size * group->update_range_low
    , group->instances->element_size * update_count
    , pmap_ref_index(group->instances, group->update_range_low)
    );
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

////////////////////////////////////////////////////////////////////////////////
// Instanced rendering/draw call
////////////////////////////////////////////////////////////////////////////////

static void _renderer_create_vao(Shader s, render_group_t* group) {
  assert(s);
  assert(group);
  assert(group->model);
  assert(!group->vao);
  assert(!group->instance_buffer);
  assert(group->instances);

  glGenVertexArrays(1, &group->vao);
  glBindVertexArray(group->vao);

  model_bind(group->model);

  glGenBuffers(1, &group->instance_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, group->instance_buffer);
  glBufferData(GL_ARRAY_BUFFER
  , group->instances->size_bytes
  , group->instances->begin
  , group->is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW
  );

  shader_bind_attributes(s);
}

////////////////////////////////////////////////////////////////////////////////

void renderer_callback_render(renderer_t* renderer, Game game) {
  assert(renderer);
  assert(game);
  if (!renderer->groups || !renderer->groups->size) return;

  // set up values shared for each pass
  Shader shader = renderer->shader;
  assert(shader);
  shader_bind(renderer->shader);

  // apply globally shared uniforms
  int loc_proj_view = shader_uniform_loc(shader, "in_pv_matrix");
  int loc_view = shader_uniform_loc(shader, "in_view_matrix");
  glUniformMatrix4fv(loc_proj_view, 1, 0, game->camera.projview.f);
  glUniformMatrix4fv(loc_view, 1, 0, game->camera.view.f);

  // render each individual render group as batches
  render_group_t* map_foreach(group, renderer->groups) {
    if (!group->instances || !group->instances->size) continue;

    shader_set_material(renderer->shader, group->material);

    // if the group's VAO hasn't been set, create it
    if (!group->vao) {
      _renderer_create_vao(shader, group);
    }
    else {
      glBindVertexArray(group->vao);
    }

    model_render_instanced(group->model, group->instances->size);
    glBindVertexArray(0);
  }
}
