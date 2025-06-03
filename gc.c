#include "gc.h"

#include <stdio.h>
#include <stdlib.h>

#include "atom.h"
#include "env.h"
#include "intern.h"
#include "log.h"
#include "third_party/clog.h"

struct gcnode {
  enum GCType type;
  size_t size;
  int marked;
  struct gcnode *next;
} __attribute__((aligned(8)));

struct gcroot {
  struct gcnode *node;
  struct gcroot *next;
};

static struct gcnode *gc_head = NULL;
static struct gcnode *gc_tail = NULL;

static struct gcroot *roots = NULL;

static struct gcnode *gc_node(void *ptr) {
  return (struct gcnode *)ptr - 1;
}

void *gc_new(enum GCType type, size_t size) {
  struct gcnode *node = malloc(sizeof(struct gcnode) + size);
  if (!node) {
    fprintf(stderr, "Error: could not allocate memory for GC node\n");
    return NULL;
  }

  node->type = type;
  node->size = size;
  node->marked = 0;
  node->next = NULL;

  if (!gc_head) {
    gc_head = node;
    gc_tail = node;
  } else {
    gc_tail->next = node;
    gc_tail = node;
  }

  return (void *)(node + 1);  // Return pointer to the memory after the gcnode
}

void gc_retain(void *ptr) {
  struct gcnode *node = gc_node(ptr);

  struct gcroot *new_root = malloc(sizeof(struct gcroot));
  new_root->node = node;
  new_root->next = roots;
  roots = new_root;
}

void gc_release(void *ptr) {
  struct gcnode *node = gc_node(ptr);

  // Find and remove the root
  struct gcroot *curr = roots;
  struct gcroot *prev = NULL;
  while (curr) {
    if (curr->node == node) {
      if (prev) {
        prev->next = curr->next;
      } else {
        roots = curr->next;
      }
      free(curr);
      return;
    }

    prev = curr;
    curr = curr->next;
  }

  fprintf(stderr, "Warning: gc_release called on a pointer not retained by GC\n");
}

int gc_mark(void *ptr) {
  struct gcnode *node = gc_node(ptr);
  int marked = node->marked;
  node->marked = 1;
  return marked;
}

void gc_init(void) {
  gc_head = NULL;
  gc_tail = NULL;
}

void gc_run(void) {
  // Mark phase
  for (struct gcroot *root = roots; root; root = root->next) {
    struct gcnode *node = root->node;
    if (node && !node->marked) {
      if (node->type == GC_TYPE_ATOM) {
        struct atom *atom = (struct atom *)(node + 1);

        // mark atom and its reachable parts
        atom_mark(atom);
      } else if (node->type == GC_TYPE_ENVIRONMENT) {
        struct environment *env = (struct environment *)(node + 1);

        environment_gc_mark(env);
      } else if (node->type == GC_TYPE_BINDING_CELL) {
        // Should already be marked by environment marking.
        node->marked = 1;
      } else {
        fprintf(stderr, "Warning: gc_run called with unsupported GC type %d", node->type);
      }
    }
  }

  intern_gc_mark();

  // Sweep phase
  struct gcnode *node = gc_head;
  struct gcnode *prev = NULL;
  while (node) {
    int marked = node->marked;
    node->marked = 0;

    if (marked) {
      clog_debug(CLOG(LOGGER_GC), "GC: skipping %p of type %d (marked)", (void *)node, node->type);
      // Safe.
      prev = node;
      node = node->next;
      continue;
    }

    clog_debug(CLOG(LOGGER_GC), "GC: sweeping %p of type %d (not marked)", (void *)node,
               node->type);

    // Not marked, so it's available for collection.
    if (prev) {
      prev->next = node->next;
    } else {
      gc_head = node->next;
    }

    if (node == gc_tail) {
      gc_tail = prev;
    }

    // Erase primitives inside the data type
    if (node->type == GC_TYPE_ATOM) {
      struct atom *atom = (struct atom *)(node + 1);
      erase_atom(atom);
    } else if (node->type == GC_TYPE_ENVIRONMENT) {
      struct environment *env = (struct environment *)(node + 1);
      erase_environment(env);
    } else if (node->type == GC_TYPE_BINDING_CELL) {
      // Nothing within a binding cell needs to be erased.
    } else {
      fprintf(stderr, "Warning: gc_run called with unsupported GC type for erasure %d\n",
              node->type);
    }

    struct gcnode *next = node->next;

    clog_debug(CLOG(LOGGER_GC), "GC: collected %p of type %d", (void *)node, node->type);
    free(node);

    node = next;
  }
}

void gc_shutdown(void) {
  if (gc_head != NULL) {
    fprintf(stderr, "Warning: GC shutdown called with uncollected nodes\n");
  }

  struct gcnode *current = gc_head;
  while (current) {
    struct gcnode *next = current->next;
    free(current);
    current = next;
  }

  gc_head = NULL;
  gc_tail = NULL;
}
