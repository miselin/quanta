#include "print.h"

#include <stdio.h>

#include "atom.h"

static int print_list(char *buffer, size_t buffer_size, struct atom *atom, int readably) {
  if (!is_cons(atom)) {
    return print_str(buffer, buffer_size, atom, readably);
  }

  size_t offset = 0;
  offset += snprintf(buffer + offset, buffer_size - offset, "(");

  while (atom && atom->type == ATOM_TYPE_CONS) {
    offset += print_str(buffer + offset, buffer_size - offset, car(atom), readably);

    atom = cdr(atom);
    if (atom && atom->type == ATOM_TYPE_CONS) {
      offset += snprintf(buffer + offset, buffer_size - offset, " ");
    }
  }

  if (atom && atom->type != ATOM_TYPE_NIL) {
    offset += snprintf(buffer + offset, buffer_size - offset, " . ");
    offset += print_str(buffer + offset, buffer_size - offset, atom, readably);
  }

  offset += snprintf(buffer + offset, buffer_size - offset, ")");
  return (int)offset;
}

void print(FILE *fp, struct atom *atom, int readably) {
  char *buffer = malloc(1024);
  print_str(buffer, 1024, atom, readably);

  fputs(buffer, fp);
  free(buffer);

  fflush(fp);
}

const char *escape_string(const char *str, size_t len) {
  if (!str || len == 0) {
    return "\"\"";
  }

  char *escaped = malloc(len * 2 + 3);  // worst case: every char is escaped
  size_t j = 0;

  escaped[j++] = '"';
  for (size_t i = 0; i < len; i++) {
    switch (str[i]) {
      case '"':
        escaped[j++] = '\\';
        escaped[j++] = '"';
        break;
      case '\\':
        escaped[j++] = '\\';
        escaped[j++] = '\\';
        break;
      case '\n':
        escaped[j++] = '\\';
        escaped[j++] = 'n';
        break;
      case '\t':
        escaped[j++] = '\\';
        escaped[j++] = 't';
        break;
      default:
        escaped[j++] = str[i];
    }
  }
  escaped[j++] = '"';
  escaped[j] = '\0';

  return escaped;
}

int print_str(char *buffer, size_t buffer_size, struct atom *atom, int readably) {
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
      return print_list(buffer, buffer_size, atom, readably);
      break;
    case ATOM_TYPE_NIL:
      return snprintf(buffer, buffer_size, "nil");
      break;
    case ATOM_TYPE_SYMBOL:
      if (!readably) {
        const char *escaped = escape_string(atom->value.string.ptr, atom->value.string.len);
        int len = snprintf(buffer, buffer_size, "%s", escaped);
        free((void *)escaped);
        return len;
      }

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
