#include "env.h"

#include <clog.h>
#include <glib-2.0/glib.h>
#include <stdio.h>
#include <stdlib.h>

#include "atom.h"
#include "gc.h"
#include "log.h"
#include "primitive.h"
#include "special.h"

struct binding_cell {
  struct atom *atom;
};

struct environment {
  GHashTable *bindings;        // char* -> struct binding_cell* bindings
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
    g_hash_table_insert(new_env->bindings, g_strdup(key), value);
  }

  return new_env;
}

void erase_environment(struct environment *env) {
  if (!env) {
    return;
  }

  g_hash_table_destroy(env->bindings);
}

static struct binding_cell *env_lookup_cell(struct environment *env, struct atom *symbol) {
  while (env) {
    struct binding_cell *cell = g_hash_table_lookup(env->bindings, symbol->value.string.ptr);
    if (cell) {
      return cell;
    }

    env = env->parent;
  }
  return NULL;
}

struct atom *env_lookup(struct environment *env, struct atom *symbol) {
  struct binding_cell *cell = env_lookup_cell(env, symbol);
  if (cell) {
    clog_debug(CLOG(LOGGER_ENV), "Found binding for symbol '%s' in env cell %p",
               symbol->value.string.ptr, (void *)cell);
    return cell->atom;
  }

  return NULL;
}

void env_bind(struct environment *env, struct atom *symbol, struct atom *value) {
  // TODO: if already set, we need to error (create a bind_set and use it instead)

  struct binding_cell *cell = gc_new(GC_TYPE_BINDING_CELL, sizeof(struct binding_cell));
  cell->atom = value;

  clog_debug(CLOG(LOGGER_ENV), "Binding value for symbol '%s' in env cell %p",
             symbol->value.string.ptr, (void *)cell);

  // key is an interned string, value is the real atom value
  // the binding should not outlive the atom
  g_hash_table_insert(env->bindings, g_strdup(symbol->value.string.ptr), cell);
}

void env_set(struct environment *env, struct atom *symbol, struct atom *value) {
  struct binding_cell *cell = env_lookup_cell(env, symbol);
  if (cell) {
    clog_debug(CLOG(LOGGER_ENV), "Setting value for symbol '%s' in env cell %p",
               symbol->value.string.ptr, (void *)cell);
    cell->atom = value;
    return;
  }

  // TODO: error if not found, set requires an existing binding
  clog_debug(CLOG(LOGGER_ENV), "Warning: env_set called on unbound symbol '%s'",
             symbol->value.string.ptr);
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
    struct binding_cell *cell = (struct binding_cell *)value;
    if (gc_mark(cell)) {
      continue;
    }

    if (cell && cell->atom) {
      atom_mark(cell->atom);
    }
  }

  if (env->parent) {
    environment_gc_mark(env->parent);
  }
}
