#include "read.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atom.h"
#include "intern.h"

static struct atom *read_list(FILE *fp);

static void consume_whitespace(FILE *fp) {
  char c = fgetc(fp);
  while (c != EOF && isspace(c)) {
    c = fgetc(fp);
  }

  if (c != EOF) {
    ungetc(c, fp);
  }
}

struct atom *read_atom(FILE *fp) {
  // consume leading whitespace
  consume_whitespace(fp);

  char c = fgetc(fp);
  if (c == EOF) {
    return NULL;
  }

  if (c == '(') {
    // if we read a '(', we need to handle it as a list instead of an atom
    ungetc(c, fp);  // put back the '('
    return read_list(fp);
  }

  // read the atom string
  char buffer[256];
  size_t index = 0;
  while (c != EOF && !isspace(c) && c != '(' && c != ')') {
    if (index < sizeof(buffer) - 1) {
      buffer[index++] = c;
    }
    c = fgetc(fp);
  }
  buffer[index] = '\0';  // null-terminate the string

  if (!index) {
    return NULL;  // empty atom
  }

  fprintf(stderr, "atom buffer: %s (index=%zd)\n", buffer, index);

  if (c != EOF) {
    ungetc(c, fp);  // put back the last character
  }

  struct atom *atom = calloc(1, sizeof(struct atom));

  // what have we read?
  if (buffer[0] == '"') {
    // string literal
    // TODO: check for correct string termination, handle escaping, etc
    atom->type = ATOM_TYPE_STRING;
    atom->value.string.ptr = strdup(buffer + 1);  // skip the opening quote
    atom->value.string.len = strlen(atom->value.string.ptr);
    fprintf(stderr, "read string: %s\n", atom->value.string.ptr);
    return atom;
  }

  if (isdigit(buffer[0]) || (buffer[0] == '-' && isdigit(buffer[1]))) {
    // integer or float
    char *endptr;
    if (strchr(buffer, '.')) {
      // float
      atom->type = ATOM_TYPE_FLOAT;
      atom->value.fvalue = strtod(buffer, &endptr);
      fprintf(stderr, "read float: %f\n", atom->value.fvalue);
    } else {
      // integer
      atom->type = ATOM_TYPE_INT;
      atom->value.ivalue = strtoll(buffer, &endptr, 10);
      fprintf(stderr, "read integer: %ld\n", atom->value.ivalue);
    }
    return atom;
  }

  if (!strcmp(buffer, "nil") || !strcmp(buffer, "f")) {
    // nil
    atom->type = ATOM_TYPE_NIL;
    fprintf(stderr, "read nil\n");
    return atom;
  }

  if (strcmp(buffer, "t") == 0) {
    // true
    atom->type = ATOM_TYPE_TRUE;
    fprintf(stderr, "read true\n");
    return atom;
  }

  // keyword or symbol
  free(atom);

  atom = intern(buffer, buffer[0] == ':');
  fprintf(stderr, "read & intern %s: %s [%p]\n", buffer[0] == ':' ? "keyword" : "symbol",
          atom->value.string.ptr, (void *)atom);
  return atom;
}

static struct atom *read_list(FILE *fp) {
  char c = fgetc(fp);
  if (c != '(') {
    fprintf(stderr, "Error: expected '(', got '%c'\n", c);
    return NULL;  // error reading list
  }

  fprintf(stderr, "reading list...\n");

  consume_whitespace(fp);
  c = fgetc(fp);
  if (c == ')') {
    // empty list
    struct atom *nil_atom = calloc(1, sizeof(struct atom));
    nil_atom->type = ATOM_TYPE_NIL;
    fprintf(stderr, "read empty list\n");
    return nil_atom;
  }

  if (c == EOF) {
    fprintf(stderr, "Error: unexpected end of file while reading list\n");
    return NULL;  // error reading list
  }

  ungetc(c, fp);  // put back the last character

  fprintf(stderr, "reading car...\n");

  struct atom *head = NULL;
  struct atom *prev = NULL;

  while (1) {
    consume_whitespace(fp);
    c = fgetc(fp);
    if (c == ')') {
      break;
    }
    if (c == EOF) {
      fprintf(stderr, "Error: unexpected end of file while reading list\n");
      return NULL;  // error reading list
    }

    ungetc(c, fp);  // put back the last character

    struct atom *atom = read_atom(fp);
    if (!atom) {
      fprintf(stderr, "Error reading atom in list\n");
      return NULL;  // error reading atom
    }

    struct atom *cons = calloc(1, sizeof(struct atom));
    cons->type = ATOM_TYPE_CONS;
    cons->value.cons.car = atom;  // the atom is the car of the cons cell
    cons->value.cons.cdr = NULL;  // initially cdr is NULL

    if (!head) {
      head = cons;  // first cons cell becomes the head of the list
    } else {
      prev->value.cons.cdr = cons;  // link the previous cons cell to the new one
    }

    prev = cons;  // update the previous pointer to the current cons cell
  }

  if (!head) {
    head = calloc(1, sizeof(struct atom));
    head->type = ATOM_TYPE_NIL;  // if we didn't read any atoms, return nil
  }

  return head;
}
