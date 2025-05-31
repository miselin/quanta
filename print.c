#include "print.h"

#include <stdio.h>

#include "atom.h"

void print(struct atom *atom);

static void print_list(struct atom *atom) {
  if (atom->type != ATOM_TYPE_CONS) {
    print(atom);
    return;
  }

  printf("(");
  while (atom && atom->type == ATOM_TYPE_CONS) {
    print(atom->value.cons.car);
    atom = atom->value.cons.cdr;
    if (atom && atom->type == ATOM_TYPE_CONS) {
      printf(" ");
    }
  }

  if (atom && atom->type != ATOM_TYPE_NIL) {
    printf(" . ");
    print(atom);
  }

  printf(")");
}

void print(struct atom *atom) {
  if (!atom) {
    printf("nil");
    return;
  }

  switch (atom->type) {
    case ATOM_TYPE_INT:
      printf("%ld", atom->value.ivalue);
      break;
    case ATOM_TYPE_FLOAT:
      printf("%f", atom->value.fvalue);
      break;
    case ATOM_TYPE_STRING:
      printf("\"%s\"", atom->value.string.ptr);
      break;
    case ATOM_TYPE_CONS:
      print_list(atom);
      break;
    case ATOM_TYPE_NIL:
      printf("nil");
      break;
    case ATOM_TYPE_SYMBOL:
      printf("%s", atom->value.string.ptr);
      break;
    case ATOM_TYPE_KEYWORD:
      printf("%s", atom->value.string.ptr);
      break;
    case ATOM_TYPE_TRUE:
      printf("t");
      break;
    default:
      fprintf(stderr, "Unknown atom type: %d\n", atom->type);
      break;
  }
}
