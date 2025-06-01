#include "intern.h"

#include <glib-2.0/glib.h>
#include <stdio.h>

#include "atom.h"

static GHashTable *symbol_table = NULL;
static GHashTable *keyword_table = NULL;

static void free_atom_value(void *value) {
  struct atom *atom = (struct atom *)value;
  if (atom) {
    atom_deref(atom);
  }
}

void init_intern_tables(void) {
  symbol_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, free_atom_value);
  keyword_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, free_atom_value);
}

void cleanup_intern_tables(void) {
  if (symbol_table) {
    g_hash_table_destroy(symbol_table);
    symbol_table = NULL;
  }

  if (keyword_table) {
    g_hash_table_destroy(keyword_table);
    keyword_table = NULL;
  }
}

struct atom *intern(const char *name, int is_keyword) {
  return atom_ref(intern_noref(name, is_keyword));
}

struct atom *intern_noref(const char *name, int is_keyword) {
  if (!symbol_table) {
    init_intern_tables();
  }

  GHashTable *table = is_keyword ? keyword_table : symbol_table;

  struct atom *existing = g_hash_table_lookup(table, name);
  if (existing) {
    return existing;
  }

  union atom_value value = {.string = {.ptr = g_strdup(name), .len = strlen(name)}};
  enum AtomType atom_type = is_keyword ? ATOM_TYPE_KEYWORD : ATOM_TYPE_SYMBOL;

  struct atom *atom = new_atom(atom_type, value);

  // fprintf(stderr, "interned %s as %p\n", name, (void *)atom);

  g_hash_table_insert(table, g_strdup(atom->value.string.ptr), atom);
  return atom;
}
