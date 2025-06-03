#include "atom.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

#include "env.h"
#include "gc.h"

static struct atom g_atom_nil = {
    .type = ATOM_TYPE_NIL,
    .value = {.ivalue = 0},
};

static struct atom g_atom_true = {
    .type = ATOM_TYPE_TRUE,
    .value = {.ivalue = 1},
};

struct atom *atom_nil(void) {
  return &g_atom_nil;
}

struct atom *atom_true(void) {
  return &g_atom_true;
}

struct atom *new_atom(enum AtomType type, union atom_value value) {
  struct atom *atom = gc_new(GC_TYPE_ATOM, sizeof(struct atom));
  atom->type = type;
  atom->value = value;
  return atom;
}

struct atom *new_cons(struct atom *car, struct atom *cdr) {
  if (!car && !cdr) {
    return new_atom_error(NULL, "'cons' requires at least one of car or cdr to be non-null");
  }

  if (!car) {
    car = atom_nil();
  }
  if (!cdr) {
    cdr = atom_nil();
  }

  union atom_value value = {.cons = {.car = car, .cdr = cdr}};

  return new_atom(ATOM_TYPE_CONS, value);
}

void erase_atom(struct atom *atom) {
  if (!atom) {
    return;
  }

  switch (atom->type) {
    case ATOM_TYPE_STRING:
    case ATOM_TYPE_SYMBOL:
    case ATOM_TYPE_KEYWORD:
      free(atom->value.string.ptr);
      break;
    case ATOM_TYPE_ERROR:
      free(atom->value.error.message);
      break;
    default:
      break;
  }
}

struct atom *car(struct atom *atom) {
  if (is_error(atom)) {
    return atom;
  }

  if (atom->type != ATOM_TYPE_CONS) {
    return new_atom_error(atom, "'car' requires a non-empty list");
  }

  struct atom *result = atom->value.cons.car;
  if (!result) {
    return new_atom_error(atom, "'car' called on an empty list");
  }

  return result;
}

struct atom *cdr(struct atom *atom) {
  if (is_error(atom)) {
    return atom;
  }

  if (atom->type != ATOM_TYPE_CONS) {
    return new_atom_error(atom, "'cdr' requires a non-empty list");
  }

  struct atom *result = atom->value.cons.cdr;
  if (!result) {
    return new_atom_error(atom, "'cdr' called on an empty list");
  }

  return result;
}

int is_cons(struct atom *atom) {
  return atom && atom->type == ATOM_TYPE_CONS;
}

int is_nil(struct atom *atom) {
  return atom == &g_atom_nil;
}

int is_symbol(struct atom *atom) {
  return atom && atom->type == ATOM_TYPE_SYMBOL;
}

int is_keyword(struct atom *atom) {
  return atom && atom->type == ATOM_TYPE_KEYWORD;
}

int is_string(struct atom *atom) {
  return atom && atom->type == ATOM_TYPE_STRING;
}

int is_int(struct atom *atom) {
  return atom && atom->type == ATOM_TYPE_INT;
}

int is_float(struct atom *atom) {
  return atom && atom->type == ATOM_TYPE_FLOAT;
}

int is_true(struct atom *atom) {
  return atom == &g_atom_true;
}

int is_basic_type(struct atom *atom) {
  return atom && (atom->type == ATOM_TYPE_INT || atom->type == ATOM_TYPE_FLOAT ||
                  atom->type == ATOM_TYPE_STRING || atom->type == ATOM_TYPE_NIL ||
                  atom->type == ATOM_TYPE_TRUE);
}

int is_error(struct atom *atom) {
  return atom && atom->type == ATOM_TYPE_ERROR;
}

const char *atom_type_to_string(enum AtomType type) {
  switch (type) {
    case ATOM_TYPE_NIL:
      return "NIL";
    case ATOM_TYPE_TRUE:
      return "TRUE";
    case ATOM_TYPE_INT:
      return "INT";
    case ATOM_TYPE_FLOAT:
      return "FLOAT";
    case ATOM_TYPE_STRING:
      return "STRING";
    case ATOM_TYPE_SYMBOL:
      return "SYMBOL";
    case ATOM_TYPE_KEYWORD:
      return "KEYWORD";
    case ATOM_TYPE_CONS:
      return "CONS";
    case ATOM_TYPE_LAMBDA:
      return "LAMBDA";
    case ATOM_TYPE_ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

void atom_mark(struct atom *atom) {
  if (!atom || atom == &g_atom_nil || atom == &g_atom_true) {
    return;
  }

  if (gc_mark(atom)) {
    // already marked, don't recurse
    return;
  }

  // When we mark a cons cell we need to mark its contents too.
  if (atom->type == ATOM_TYPE_CONS) {
    atom_mark(atom->value.cons.car);
    atom_mark(atom->value.cons.cdr);
  }

  if (atom->type == ATOM_TYPE_LAMBDA) {
    atom_mark(atom->value.lambda.args);
    atom_mark(atom->value.lambda.body);
    environment_gc_mark(atom->value.lambda.env);
  }

  if (atom->type == ATOM_TYPE_ERROR) {
    atom_mark(atom->value.error.cause);
  }
}

struct atom *new_atom_error(struct atom *cause, const char *message, ...) {
  struct atom *atom = gc_new(GC_TYPE_ATOM, sizeof(struct atom));
  atom->type = ATOM_TYPE_ERROR;
  atom->value.error.cause = cause;

  size_t bufsize = 256;
  atom->value.error.message = (char *)malloc(256);
  if (message) {
    va_list args;
    va_start(args, message);
    int written = vsnprintf(atom->value.error.message, bufsize, message, args);
    while ((size_t)written >= bufsize) {
      bufsize = written + 1;
      atom->value.error.message = (char *)realloc(atom->value.error.message, bufsize);
      va_start(args, message);
      written = vsnprintf(atom->value.error.message, bufsize, message, args);
    }
    va_end(args);
  } else {
    atom->value.error.message = NULL;
  }
  return atom;
}
