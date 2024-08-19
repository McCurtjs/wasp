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
