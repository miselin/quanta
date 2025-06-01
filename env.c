#include "env.h"

#include <glib-2.0/glib.h>
#include <stdio.h>
#include <stdlib.h>

#include "atom.h"
#include "primitive.h"
#include "special.h"

struct environment {
  GHashTable *bindings;  // char* -> struct atom* bindings

  struct environment *parent;  // for nested environments

  gatomicrefcount ref_count;  // reference count for memory management
};

static void free_atom_value(void *value) {
  struct atom *atom = (struct atom *)value;
  if (atom) {
    atom_deref(atom);
  }
}

struct environment *create_default_environment(void) {
  struct environment *env = create_environment(NULL);
  g_atomic_ref_count_init(&env->ref_count);
  init_primitives(env);
  init_special_forms(env);
  return env;
}

struct environment *create_environment(struct environment *parent) {
  struct environment *env = malloc(sizeof(struct environment));

  env->bindings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, free_atom_value);
  env->parent = parent;

  g_atomic_ref_count_init(&env->ref_count);
  if (parent) {
    ref_environment(parent);
  }

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
    g_hash_table_insert(new_env->bindings, g_strdup(key), atom_ref(value));  // copy key and value
  }

  return new_env;
}

struct environment *ref_environment(struct environment *env) {
  if (!env) {
    return NULL;
  }

  g_atomic_ref_count_inc(&env->ref_count);
  return env;
}

void deref_environment(struct environment *env) {
  if (!env) {
    return;
  }

  if (!g_atomic_ref_count_dec(&env->ref_count)) {
    // fprintf(stderr, "deref env %p: still alive\n", (void *)env);
    return;
  }

  // fprintf(stderr, "deref env %p: freeing\n", (void *)env);

  if (env->parent) {
    deref_environment(env->parent);
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
    return atom_ref(bound);  // found in current environment
  }

  // check parent environment if it exists
  if (env->parent) {
    return env_lookup(env->parent, symbol);
  }

  return NULL;  // not found in this environment or any parent
}

void env_bind(struct environment *env, struct atom *symbol, struct atom *value) {
  env_bind_noref(env, symbol, atom_ref(value));
}

void env_bind_noref(struct environment *env, struct atom *symbol, struct atom *value) {
  if (!env || !symbol || !value || symbol->type != ATOM_TYPE_SYMBOL) {
    return;  // invalid environment or symbol
  }

  // key is an interned string, value is the real atom value
  // the binding should not outlive the atom
  g_hash_table_insert(env->bindings, g_strdup(symbol->value.string.ptr), value);
}
