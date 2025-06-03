#include "read.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atom.h"
#include "clog.h"
#include "intern.h"
#include "log.h"

static struct atom *read_list(struct source_file *source);

static void consume_whitespace(struct source_file *source) {
  char c = source_file_getc(source);
  if (c == ';') {
    // skip comments
    while (c != EOF && c != '\n') {
      c = source_file_getc(source);
    }
  }

  while (c != EOF && isspace(c)) {
    c = source_file_getc(source);
  }

  if (c != EOF) {
    source_file_ungetc(source, c);
  }
}

struct atom *read_atom(struct source_file *source) {
  // consume leading whitespace
  consume_whitespace(source);

  char c = source_file_getc(source);
  if (c == EOF) {
    return NULL;
  }

  if (c == '(') {
    // if we read a '(', we need to handle it as a list instead of an atom
    source_file_ungetc(source, c);  // put back the '('
    return read_list(source);
  }

  // handle certain syntactic sugars
  if (c == '\'') {
    // it's actually (quote atom)
    struct atom *atom = read_atom(source);
    if (!atom) {
      fprintf(stderr, "Error: expected atom after '\n");
      return NULL;  // error reading atom
    }

    struct atom *quote_atom = intern("quote", 0);
    return new_cons(quote_atom, new_cons(atom, atom_nil()));
  }

  int is_str = 0;
  int is_escaped = 0;
  if (c == '"') {
    is_str = 2;
  }

  // read the atom string
  char buffer[256];
  size_t buffer_length = 0;
  while (c != EOF) {
    if (is_str > 0) {
      if (c == '"' && !is_escaped) {
        // end of string
        --is_str;
      }

      if (c == '\\') {
        is_escaped = 1;
      }

      // all characters are valid inside a string for now
    } else if (isspace(c) || c == '(' || c == ')' || c == ';') {
      break;
    }

    if (buffer_length < sizeof(buffer) - 1) {
      buffer[buffer_length++] = c;
    }
    c = source_file_getc(source);
  }
  buffer[buffer_length] = '\0';  // null-terminate the string

  if (!buffer_length) {
    fprintf(stderr, "Error: empty atom read\n");
    return NULL;  // empty atom
  }

  clog_debug(CLOG(LOGGER_READ), "processing atom from '%s'", buffer);

  if (is_str != 0) {
    fprintf(stderr, "Error: string not terminated properly\n");
    return NULL;
  }

  if (c != EOF) {
    source_file_ungetc(source, c);  // put back the last character
  }

  if (!strcmp(buffer, "nil") || !strcmp(buffer, "f")) {
    // nil
    return atom_nil();
  }

  if (strcmp(buffer, "t") == 0) {
    // true
    return atom_true();
  }

  // what have we read?
  if (buffer[0] == '"') {
    // search for closing quote
    size_t last = buffer_length - 1;
    if (buffer[last] != '"') {
      fprintf(stderr, "Error: string not terminated properly\n");
      return NULL;  // error reading string
    }

    char *value_ptr = malloc(last);

    // search for characters that require escaping
    int is_escape = 0;
    size_t at = 0;
    for (size_t i = 1; i < last; i++) {
      if (is_escape) {
        is_escape = 0;

        switch (buffer[i]) {
          case 'n':
            value_ptr[at++] = '\n';
            break;
          case 't':
            value_ptr[at++] = '\t';
            break;
          case 'r':
            value_ptr[at++] = '\r';
            break;
          case '"':
            value_ptr[at++] = '"';
            break;
          case '\\':
            value_ptr[at++] = '\\';
            break;
          default:
            fprintf(stderr, "Error: unknown escape sequence '\\%c'\n", buffer[i]);
            free(value_ptr);
            return NULL;  // error reading string
        }
      } else if (buffer[i] == '\\') {
        is_escape = 1;
      } else if (buffer[i] == '"') {
        fprintf(stderr, "Error: unexpected closing quote in string\n");
        free(value_ptr);
        return NULL;  // error reading string
      } else {
        value_ptr[at++] = buffer[i];
      }
    }

    value_ptr[at] = '\0';  // null-terminate the string

    union atom_value value = {.string = {.ptr = value_ptr, .len = at}};
    return new_atom(ATOM_TYPE_STRING, value);
  }

  if (isdigit(buffer[0]) || (buffer[0] == '-' && isdigit(buffer[1]))) {
    union atom_value value;

    // integer or float
    // TODO: need better parsing here
    char *endptr;
    if (strchr(buffer, '.')) {
      // float
      value.fvalue = strtod(buffer, &endptr);
      return new_atom(ATOM_TYPE_FLOAT, value);
    } else {
      // integer
      value.ivalue = strtoll(buffer, &endptr, 10);
      return new_atom(ATOM_TYPE_INT, value);
    }
  }

  return intern(buffer, buffer[0] == ':');
}

static struct atom *read_list(struct source_file *source) {
  char c = source_file_getc(source);
  if (c != '(') {
    fprintf(stderr, "Error: expected '(', got '%c'\n", c);
    return NULL;  // error reading list
  }

  consume_whitespace(source);
  c = source_file_getc(source);
  if (c == ')') {
    // empty list
    return atom_nil();
  }

  if (c == EOF) {
    fprintf(stderr, "Error: unexpected end of file while reading list\n");
    return NULL;  // error reading list
  }

  source_file_ungetc(source, c);  // put back the last character

  struct atom *head = NULL;
  struct atom *prev = NULL;

  int dotted = 0;

  while (1) {
    consume_whitespace(source);
    c = source_file_getc(source);
    if (c == ')') {
      break;
    }
    if (c == EOF) {
      fprintf(stderr, "Error: unexpected end of file while reading list\n");
      return NULL;
    }

    if (c == '.') {
      // TODO: handle invalid syntax like '(1 . 2 . 3)'
      dotted = 1;
    } else {
      source_file_ungetc(source, c);  // put back the character we read to check
    }

    struct atom *atom = read_atom(source);
    if (!atom) {
      fprintf(stderr, "Error reading atom in list\n");
      return NULL;
    }

    if (dotted) {
      if (!prev) {
        fprintf(stderr, "Error: cannot have dotted pair without a previous cons cell\n");
        return NULL;
      }

      if (!is_nil(cdr(prev))) {
        fprintf(stderr, "Error: cannot have more than one dotted pair in a list\n");
        return NULL;
      }

      prev->value.cons.cdr = atom;

      c = source_file_getc(source);
      if (c != ')') {
        fprintf(stderr, "Error: expected ')', got '%c'\n", c);
        return NULL;  // error reading list
      }

      break;
    }

    struct atom *cons = new_cons(atom, NULL);

    if (!head) {
      head = cons;  // first cons cell becomes the head of the list
    } else {
      prev->value.cons.cdr = cons;  // link the previous cons cell to the new one
    }

    prev = cons;  // update the previous pointer to the current cons cell
  }

  if (!head) {
    head = atom_nil();
  }

  return head;
}
