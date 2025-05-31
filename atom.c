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
      free_atom(atom->value.cons.car);
      free_atom(atom->value.cons.cdr);
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
