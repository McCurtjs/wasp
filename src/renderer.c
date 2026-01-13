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

#include "renderer.h"
#include "game.h"
#include "gl.h"

////////////////////////////////////////////////////////////////////////////////
// Registers an entity with a renderer
////////////////////////////////////////////////////////////////////////////////

void renderer_entity_register(entity_t* entity, renderer_t* renderer) {
  assert(renderer);
  assert(entity);

  if (entity->render_id.hash) {
    if (entity->renderer) {
      if (entity->renderer == renderer) return;
      renderer_entity_unregister(entity);
    }
    entity->render_id = SK_NULL;
  }

  if (renderer->register_entity) {
    Game game = game_get_active();
    entity->renderer = renderer;
    entity->render_id = renderer->register_entity(entity, game);
  }

  if (!entity->render_id.hash) {
    entity->renderer = NULL;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Removes an entity from its renderer
////////////////////////////////////////////////////////////////////////////////

void renderer_entity_unregister(entity_t* entity) {
  renderer_t* renderer = entity->renderer;
  assert(renderer || !entity->render_id.hash);
  if (!renderer) return;
  if (renderer->unregister_entity) {
    renderer->unregister_entity(entity);
  }
  renderer = NULL;
  entity->render_id = SK_NULL;
}

////////////////////////////////////////////////////////////////////////////////
// "Default" callback functions to use for renderers (still need to assign)
////////////////////////////////////////////////////////////////////////////////

slotkey_t renderer_callback_register(entity_t* e, Game game) {
  assert(e);
  assert(e->renderer);
  assert(e->renderer->groups);
  assert(e->model);
  UNUSED(game);

  // get associated render group
  render_group_key_t key = { e->model, e->material, e->is_static };
  res_ensure_rg_t group_slot = map_rg_ensure(e->renderer->groups, key);

  if (group_slot.is_new) {
    *group_slot.value = (render_group_t){
      .instances = pmap_new(instance_attribute_default_t),
      .material = e->material,
      .model = e->model,
      .is_static = e->is_static,
    };
  }

  group_slot.value->needs_update = true;

  // add instance to the group and save its instance id
  return pmap_insert(group_slot.value->instances,
    &(instance_attribute_default_t) {
      .transform = e->transform,
    }
  );
}

////////////////////////////////////////////////////////////////////////////////

void renderer_callback_unregister(entity_t* e) {
  assert(e);
  render_group_key_t key = { e->model, e->material, e->is_static };
  render_group_t* group = map_rg_ref(e->renderer->groups, key);
  if (!group) return;
  pmap_remove(group->instances, e->render_id);
  group->needs_update = true;
  e->renderer = NULL;
  e->render_id = SK_NULL;
}

////////////////////////////////////////////////////////////////////////////////

static void _renderer_bind_default_attributes(Shader s, render_group_t* group) {
  assert(s);
  assert(group);
  assert(group->model);
  assert(!group->vao);
  assert(!group->instance_buffer);
  assert(group->instances);

  glGenVertexArrays(1, &group->vao);
  glBindVertexArray(group->vao);

  model2_bind(group->model2);

  glGenBuffers(1, &group->instance_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, group->instance_buffer);
  glBufferData(GL_ARRAY_BUFFER
  , group->instances->size_bytes
  , group->instances->begin
  , group->is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW
  );

  GLsizei stride = sizeof(instance_attribute_default_t);
  GLsizei base = shader_attribute_loc(s, "model_matrix");
  GLuint i = base;

  glEnableVertexAttribArray(i);
  glVertexAttribPointer(i, 4, GL_FLOAT, false, stride, (void*)(0 * v4bytes));
  glVertexAttribDivisor(i, 1);

  glEnableVertexAttribArray(++i);
  glVertexAttribPointer(i, 4, GL_FLOAT, false, stride, (void*)(1 * v4bytes));
  glVertexAttribDivisor(i, 1);

  glEnableVertexAttribArray(++i);
  glVertexAttribPointer(i, 4, GL_FLOAT, false, stride, (void*)(2 * v4bytes));
  glVertexAttribDivisor(i, 1);

  glEnableVertexAttribArray(++i);
  glVertexAttribPointer(i, 4, GL_FLOAT, false, stride, (void*)(3 * v4bytes));
  glVertexAttribDivisor(i, 1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
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

  // get uniform/attribute locations for active material
  int loc_sampler_tex = shader_uniform_loc(shader, "texSamp");
  int loc_sampler_norm = shader_uniform_loc(shader, "normSamp");
  int loc_sampler_rough = shader_uniform_loc(shader, "roughSamp");
  int loc_sampler_metal = shader_uniform_loc(shader, "metalSamp");
  int loc_props = shader_uniform_loc(shader, "in_weights");

  // apply globally shared uniforms
  int loc_proj_view = shader_uniform_loc(shader, "in_pv_matrix");
  int loc_view = shader_uniform_loc(shader, "in_view_matrix");
  glUniformMatrix4fv(loc_proj_view, 1, 0, game->camera.projview.f);
  glUniformMatrix4fv(loc_view, 1, 0, game->camera.view.f);

  // render each individual render group as batches
  render_group_t* map_foreach(group, renderer->groups) {
    if (!group->instances || !group->instances->size) continue;

    // matset_apply(renderer->materials);
    tex_apply(group->material->map_diffuse, 0, loc_sampler_tex);
    tex_apply(group->material->map_normals, 1, loc_sampler_norm);
    tex_apply(group->material->map_roughness, 2, loc_sampler_rough);
    tex_apply(group->material->map_metalness, 3, loc_sampler_metal);
    glUniform3fv(loc_props, 1, group->material->weights.f);

    // if the group's VAO hasn't been set, create it
    if (!group->vao) {
      _renderer_bind_default_attributes(shader, group);

      if (renderer->bind_attributes) {
        renderer->bind_attributes(shader, group);
      }
    }

    // if the instance data has updated since creation, upload it to the GPU
    else if (group->needs_update) {
      // TODO: add path for glBufferSubData for small changes
      glBindBuffer(GL_ARRAY_BUFFER, group->instance_buffer);
      glBufferData(GL_ARRAY_BUFFER
      , group->instances->size_bytes
      , group->instances->begin
      , group->is_static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW
      );
      glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    group->needs_update = false;

    glBindVertexArray(group->vao);

    if (group->model->index_count) {
      // indexed draw
      glDrawElementsInstanced
      ( GL_TRIANGLES
      , (GLsizei)group->model->vert_count
      , GL_UNSIGNED_INT
      , 0
      , (GLsizei)group->instances->size
      );
    }
    else {
      // non-indexed draw
      glDrawArraysInstanced
      ( GL_TRIANGLES
      , 0
      , (GLsizei)group->model->vert_count
      , (GLsizei)group->instances->size
      );
    }

    glBindVertexArray(0);
  }
}

////////////////////////////////////////////////////////////////////////////////

void renderer_callback_update(renderer_t* renderer, entity_t* e) {
  assert(renderer);
  assert(e);
  assert(renderer == e->renderer);
  assert(e->render_id.hash);

  render_group_key_t key = { e->model, e->material, e->is_static };
  render_group_t* group = map_rg_ref(e->renderer->groups, key);
  assert(group);
  assert(group->instances);

  instance_attribute_default_t* att = pmap_ref(group->instances, e->render_id);
  assert(att);

  // TODO: update to track range of updates for glBufferSubData
  att->transform = e->transform;
  group->needs_update = true;
}
