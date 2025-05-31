#include "atom.h"

#include <stdio.h>
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

struct atom *car(struct atom *atom) {
  if (atom->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: 'car' requires a non-empty list\n");
    return NULL;  // error handling
  }

  struct atom *result = atom->value.cons.car;
  if (!result) {
    fprintf(stderr, "Error: 'car' called on an empty list\n");
    return NULL;  // error handling
  }

  return result;
}

struct atom *cdr(struct atom *atom) {
  if (atom->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: 'cdr' requires a non-empty list\n");
    return NULL;  // error handling
  }

  struct atom *result = atom->value.cons.cdr;
  if (!result) {
    fprintf(stderr, "Error: 'cdr' called on an empty list\n");
    return NULL;  // error handling
  }

  return result;
}
