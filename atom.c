#include "atom.h"

#include <stdlib.h>

void free_atom(struct atom *atom) {
  if (!atom) {
    return;
  }

  switch (atom->type) {
    case ATOM_TYPE_STRING:
      free(atom->value.string.ptr);
      break;
    case ATOM_TYPE_CONS: {
      struct cons *cons = atom->value.cons;
      free_atom(cons->car);
      free_atom(cons->cdr);
      free(cons);
      break;
    }
    case ATOM_TYPE_SYMBOL:
    case ATOM_TYPE_KEYWORD:
      // for symbols and keywords, we don't free the string here
      // because they are interned and managed by the hash table
      return;
    default:
      break;  // no additional cleanup needed for other types
  }

  free(atom);
}
