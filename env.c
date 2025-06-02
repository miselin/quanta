#include "env.h"

#include <glib-2.0/glib.h>
#include <stdio.h>
#include <stdlib.h>

#include "atom.h"
#include "gc.h"
#include "primitive.h"
#include "special.h"

struct environment {
  GHashTable *bindings;        // char* -> struct atom* bindings
  struct environment *parent;  // for nested environments
};

struct environment *create_default_environment(void) {
  struct environment *env = create_environment(NULL);
  init_primitives(env);
  init_special_forms(env);
  return env;
}

struct environment *create_environment(struct environment *parent) {
  struct environment *env = gc_new(GC_TYPE_ENVIRONMENT, sizeof(struct environment));

  env->bindings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
  env->parent = parent;

  return env;
}

struct environment *clone_environment(struct environment *env) {
  if (!env) {
    return NULL;  // nothing to clone
  }

  struct environment *new_env = create_environment(env->parent);

  // Copy bindings from the old environment
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init(&iter, env->bindings);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    g_hash_table_insert(new_env->bindings, g_strdup(key), value);  // copy key and value
  }

  return new_env;
}

void erase_environment(struct environment *env) {
  if (!env) {
    return;
  }

  g_hash_table_destroy(env->bindings);
}

struct atom *env_lookup(struct environment *env, struct atom *symbol) {
  if (!env || !symbol || symbol->type != ATOM_TYPE_SYMBOL) {
    return NULL;  // invalid environment or symbol
  }

  struct atom *bound = g_hash_table_lookup(env->bindings, symbol->value.string.ptr);
  if (bound) {
    return bound;
  }

  // check parent environment if it exists
  if (env->parent) {
    return env_lookup(env->parent, symbol);
  }

  return NULL;  // not found in this environment or any parent
}

void env_bind(struct environment *env, struct atom *symbol, struct atom *value) {
  if (!env || !symbol || !value || symbol->type != ATOM_TYPE_SYMBOL) {
    return;  // invalid environment or symbol
  }

  // TODO: if already set, we need to error (create a bind_set and use it instead)

  // key is an interned string, value is the real atom value
  // the binding should not outlive the atom
  g_hash_table_insert(env->bindings, g_strdup(symbol->value.string.ptr), value);
}

void environment_gc_mark(struct environment *env) {
  GHashTableIter iter;
  gpointer key, value;
  if (!env) {
    return;  // nothing to mark
  }

  if (gc_mark(env)) {
    // already marked, no need to recurse
    return;
  }

  g_hash_table_iter_init(&iter, env->bindings);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    struct atom *atom = (struct atom *)value;
    if (atom) {
      atom_mark(atom);
    }
  }

  if (env->parent) {
    environment_gc_mark(env->parent);
  }
}
