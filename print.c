#include "print.h"

#include <stdio.h>

#include "atom.h"

static int print_list(char *buffer, size_t buffer_size, struct atom *atom) {
  if (!is_cons(atom)) {
    return print_str(buffer, buffer_size, atom);
  }

  size_t offset = 0;
  offset += snprintf(buffer + offset, buffer_size - offset, "(");

  while (atom && atom->type == ATOM_TYPE_CONS) {
    offset += print_str(buffer + offset, buffer_size - offset, car(atom));

    atom = cdr(atom);
    if (atom && atom->type == ATOM_TYPE_CONS) {
      offset += snprintf(buffer + offset, buffer_size - offset, " ");
    }
  }

  if (atom && atom->type != ATOM_TYPE_NIL) {
    offset += snprintf(buffer + offset, buffer_size - offset, " . ");
    offset += print_str(buffer + offset, buffer_size - offset, atom);
  }

  offset += snprintf(buffer + offset, buffer_size - offset, ")");
  return (int)offset;
}

void print(FILE *fp, struct atom *atom) {
  char *buffer = malloc(1024);
  print_str(buffer, 1024, atom);

  fputs(buffer, fp);
  free(buffer);
}

int print_str(char *buffer, size_t buffer_size, struct atom *atom) {
  if (!atom) {
    return snprintf(buffer, buffer_size, "nil");
  }

  switch (atom->type) {
    case ATOM_TYPE_INT:
      return snprintf(buffer, buffer_size, "%ld", atom->value.ivalue);
      break;
    case ATOM_TYPE_FLOAT:
      return snprintf(buffer, buffer_size, "%f", atom->value.fvalue);
      break;
    case ATOM_TYPE_STRING:
      return snprintf(buffer, buffer_size, "\"%s\"", atom->value.string.ptr);
      break;
    case ATOM_TYPE_CONS:
      return print_list(buffer, buffer_size, atom);
      break;
    case ATOM_TYPE_NIL:
      return snprintf(buffer, buffer_size, "nil");
      break;
    case ATOM_TYPE_SYMBOL:
      return snprintf(buffer, buffer_size, "%s", atom->value.string.ptr);
      break;
    case ATOM_TYPE_KEYWORD:
      return snprintf(buffer, buffer_size, "%s", atom->value.string.ptr);
      break;
    case ATOM_TYPE_TRUE:
      return snprintf(buffer, buffer_size, "t");
      break;
    default:
      fprintf(stderr, "Unknown atom type: %d\n", atom->type);
      break;
  }

  return 0;
}
