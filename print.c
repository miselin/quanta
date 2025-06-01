#include "print.h"

#include <stdio.h>

#include "atom.h"

static void print_list(FILE *fp, struct atom *atom) {
  if (atom->type != ATOM_TYPE_CONS) {
    print(fp, atom);
    return;
  }

  fprintf(fp, "(");
  while (atom && atom->type == ATOM_TYPE_CONS) {
    print(fp, atom->value.cons.car);
    atom = atom->value.cons.cdr;
    if (atom && atom->type == ATOM_TYPE_CONS) {
      fprintf(fp, " ");
    }
  }

  if (atom && atom->type != ATOM_TYPE_NIL) {
    fprintf(fp, " . ");
    print(fp, atom);
  }

  fprintf(fp, ")");
}

void print(FILE *fp, struct atom *atom) {
  if (!atom) {
    fprintf(fp, "nil");
    return;
  }

  switch (atom->type) {
    case ATOM_TYPE_INT:
      fprintf(fp, "%ld", atom->value.ivalue);
      break;
    case ATOM_TYPE_FLOAT:
      fprintf(fp, "%f", atom->value.fvalue);
      break;
    case ATOM_TYPE_STRING:
      fprintf(fp, "\"%s\"", atom->value.string.ptr);
      break;
    case ATOM_TYPE_CONS:
      print_list(fp, atom);
      break;
    case ATOM_TYPE_NIL:
      fprintf(fp, "nil");
      break;
    case ATOM_TYPE_SYMBOL:
      fprintf(fp, "%s", atom->value.string.ptr);
      break;
    case ATOM_TYPE_KEYWORD:
      fprintf(fp, "%s", atom->value.string.ptr);
      break;
    case ATOM_TYPE_TRUE:
      fprintf(fp, "t");
      break;
    default:
      fprintf(stderr, "Unknown atom type: %d\n", atom->type);
      break;
  }
}
