#include "gc.h"

#include <clog.h>
#include <stdio.h>
#include <stdlib.h>

#include "atom.h"
#include "env.h"
#include "intern.h"
#include "lex.h"
#include "log.h"

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

static const char *gc_type_to_str(enum GCType type) {
  switch (type) {
    case GC_TYPE_TOKEN:
      return "token";
    case GC_TYPE_ATOM:
      return "atom";
    case GC_TYPE_ENVIRONMENT:
      return "environment";
    case GC_TYPE_BINDING_CELL:
      return "binding_cell";
    case GC_TYPE_LEXER:
      return "lexer";
  }

  return "unknown";
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

  clog_debug(CLOG(LOGGER_GC), "GC: retaining %p (actual %p) of type %s", (void *)node,
             (void *)(node + 1), gc_type_to_str(node->type));

  struct gcroot *new_root = malloc(sizeof(struct gcroot));
  new_root->node = node;
  new_root->next = roots;
  roots = new_root;
}

void gc_release(void *ptr) {
  struct gcnode *node = gc_node(ptr);

  clog_debug(CLOG(LOGGER_GC), "GC: removing root %p (actual %p) of type %s", (void *)node,
             (void *)(node + 1), gc_type_to_str(node->type));

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
  if (!ptr) {
    return 0;
  }

  struct gcnode *node = gc_node(ptr);
  int marked = node->marked;
  node->marked = 1;
  return marked;
}

void gc_init(void) {
  gc_head = NULL;
  gc_tail = NULL;
}

size_t gc_run(void) {
  // Mark phase
  for (struct gcroot *root = roots; root; root = root->next) {
    struct gcnode *node = root->node;
    if (node) {
      switch (node->type) {
        case GC_TYPE_ATOM: {
          struct atom *atom = (struct atom *)(node + 1);

          // mark atom and its reachable parts
          atom_mark(atom);
        } break;
        case GC_TYPE_ENVIRONMENT: {
          struct environment *env = (struct environment *)(node + 1);

          environment_gc_mark(env);
        } break;
        case GC_TYPE_BINDING_CELL: {
          // Should already be marked by environment marking.
          node->marked = 1;
        } break;
        case GC_TYPE_TOKEN: {
          node->marked = 1;
        } break;
        case GC_TYPE_LEXER: {
          struct lex *lexer = (struct lex *)(node + 1);
          lex_gc_mark(lexer);
        } break;
      }
    }
  }

  intern_gc_mark();

  size_t total_visited = 0;
  size_t total_skipped = 0;
  size_t total_swept = 0;

  size_t total_bytes = 0;
  size_t remaining_bytes = 0;

  // Sweep phase
  struct gcnode *node = gc_head;
  struct gcnode *prev = NULL;
  while (node) {
    int marked = node->marked;
    node->marked = 0;

    ++total_visited;
    total_bytes += node->size;

    if (marked) {
      ++total_skipped;
      remaining_bytes += node->size;

      // Safe.
      prev = node;
      node = node->next;
      continue;
    }

    ++total_swept;

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
    switch (node->type) {
      case GC_TYPE_ATOM: {
        struct atom *atom = (struct atom *)(node + 1);
        erase_atom(atom);
      } break;
      case GC_TYPE_ENVIRONMENT: {
        struct environment *env = (struct environment *)(node + 1);
        erase_environment(env);
      } break;
      case GC_TYPE_BINDING_CELL: {
        // Nothing within a binding cell needs to be erased.
      } break;
      case GC_TYPE_TOKEN: {
        lex_gc_erase_token((struct token *)(node + 1));
      } break;
      case GC_TYPE_LEXER: {
        lex_gc_erase((struct lex *)(node + 1));
      } break;
    }

    struct gcnode *next = node->next;

    clog_debug(CLOG(LOGGER_GC), "GC: collected %p (actual %p) of type %s", (void *)node,
               (void *)(node + 1), gc_type_to_str(node->type));
    free(node);

    node = next;
  }

  clog_debug(CLOG(LOGGER_GC), "GC: visited %zu nodes, skipped %zu, swept %zu", total_visited,
             total_skipped, total_swept);
  clog_info(CLOG(LOGGER_GC),
            "GC: started with %zu total bytes allocated, retained %zu bytes, freed %zu bytes",
            total_bytes, remaining_bytes, total_bytes - remaining_bytes);

  return total_bytes - remaining_bytes;
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
