#include "env.h"

#include <glib-2.0/glib.h>
#include <stdlib.h>

#include "atom.h"

struct environment {
  GHashTable *bindings;

  struct environment *parent;  // for nested environments
};

struct environment *create_default_environment(void) {
  struct environment *env = create_environment(NULL);
  // TODO: set default symbols and values
  return env;
}

struct environment *create_environment(struct environment *parent) {
  struct environment *env = malloc(sizeof(struct environment));

  env->bindings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  env->parent = parent;

  return env;
}

void free_environment(struct environment *env) {
  if (!env) {
    return;
  }

  g_hash_table_destroy(env->bindings);
  free(env);
}

struct atom *env_lookup(struct environment *env, struct atom *symbol) {
  if (!env || !symbol || symbol->type != ATOM_TYPE_SYMBOL) {
    return NULL;  // invalid environment or symbol
  }

  struct atom *bound = g_hash_table_lookup(env->bindings, symbol->value.string.ptr);
  if (bound) {
    return bound;  // found in current environment
  }

  // check parent environment if it exists
  if (env->parent) {
    return env_lookup(env->parent, symbol);
  }

  return NULL;  // not found in this environment or any parent
}

void env_bind(struct environment *env, struct atom *symbol, struct atom *value) {
  if (!env || !symbol || symbol->type != ATOM_TYPE_SYMBOL) {
    return;  // invalid environment or symbol
  }

  // key is an interned string, value is the real atom value
  // the binding should not outlive the atom
  g_hash_table_insert(env->bindings, symbol->value.string.ptr, value);
}
