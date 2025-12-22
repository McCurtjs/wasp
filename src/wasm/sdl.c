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

#include "SDL3/SDL.h"

#include "wasm.h"

#include <stdlib.h>

typedef struct EventNode {
  struct EventNode* next;
  SDL_Event* event;
} EventNode;

static EventNode queue = {&queue, NULL};
static EventNode* tail = &queue;

int SDL_PushEvent(SDL_Event* event_src) {
  EventNode* node = malloc(sizeof(EventNode));
  node->event = malloc(sizeof(SDL_Event));
  node->next = &queue;
  tail->next = node;
  tail = node;
  *node->event = *event_src;
  return 1;
}

SDL_bool SDL_PollEvent(SDL_Event* event) {
  if (queue.next == &queue) return SDL_FALSE;

  EventNode* node = queue.next;
  queue.next = node->next;
  if (node == tail) tail = &queue;

  *event = *node->event;

  free(node->event);
  free(node);
  return SDL_TRUE;
}

int debug_count_events() {
  int count = 0;
  EventNode* it = queue.next;
  while (it) {
    ++count;
    it = it->next;
  }
  return count;
}
