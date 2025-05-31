#include "intern.h"

#include <glib-2.0/glib.h>

#include "atom.h"

static GHashTable *symbol_table = NULL;
static GHashTable *keyword_table = NULL;

void init_intern_tables(void) {
  symbol_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  keyword_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

static void free_atoms_in_table(GHashTable *table) {
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init(&iter, table);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    struct atom *atom = (struct atom *)value;
    if (atom->type == ATOM_TYPE_STRING) {
      g_free(atom->value.string.ptr);  // free the string pointer
    }
  }
}

void cleanup_intern_tables(void) {
  if (symbol_table) {
    free_atoms_in_table(symbol_table);
    g_hash_table_destroy(symbol_table);
    symbol_table = NULL;
  }

  if (keyword_table) {
    free_atoms_in_table(keyword_table);
    g_hash_table_destroy(keyword_table);
    keyword_table = NULL;
  }
}

struct atom *intern(const char *name, int is_keyword) {
  if (!symbol_table) {
    init_intern_tables();
  }

  GHashTable *table = is_keyword ? keyword_table : symbol_table;

  struct atom *existing = g_hash_table_lookup(table, name);
  if (existing) {
    return existing;
  }

  struct atom *a = g_new0(struct atom, 1);
  a->type = is_keyword ? ATOM_TYPE_KEYWORD : ATOM_TYPE_SYMBOL;
  a->value.string.ptr = g_strdup(name);
  a->value.string.len = strlen(name);

  g_hash_table_insert(table, a->value.string.ptr, a);
  return a;
}
