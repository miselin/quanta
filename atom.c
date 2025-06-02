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
    fprintf(stderr, "Error: 'cons' requires at least one of car or cdr to be non-null\n");
    return NULL;
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
    default:
      break;
  }
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
}
