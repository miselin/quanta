#include "intern.h"

#include <clog.h>
#include <glib-2.0/glib.h>
#include <stdio.h>

#include "atom.h"
#include "gc.h"
#include "log.h"

static GHashTable *symbol_table = NULL;
static GHashTable *keyword_table = NULL;

void init_intern_tables(void) {
  symbol_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
  keyword_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
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

  clog_debug(CLOG(LOGGER_INTERN), "interned %s as %p", name, (void *)atom);

  g_hash_table_insert(table, g_strdup(atom->value.string.ptr), atom);
  return atom;
}

void intern_gc_mark(void) {
  GHashTableIter iter;
  gpointer key, value;
  if (!symbol_table || !keyword_table) {
    return;  // no intern tables initialized
  }

  g_hash_table_iter_init(&iter, symbol_table);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    struct atom *atom = (struct atom *)value;
    gc_mark(atom);
  }
}
