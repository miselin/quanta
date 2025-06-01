#include "atom.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

#include "env.h"

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
  struct atom *atom = calloc(1, sizeof(struct atom));
  atom->type = type;
  atom->value = value;
  g_atomic_ref_count_init(&atom->ref_count);
  return atom;
}

struct atom *atom_ref(struct atom *atom) {
  if (!atom) {
    return atom;
  } else if (atom == &g_atom_nil || atom == &g_atom_true) {
    // g_atom_nil and g_atom_true are static atoms, do not increment their ref count
    return atom;
  }

  // fprintf(stderr, "ref atom %p\n", (void *)atom);

  g_atomic_ref_count_inc(&atom->ref_count);

  // When we ref a cons cell we need to ref its contents too.
  if (atom->type == ATOM_TYPE_CONS) {
    atom_ref(atom->value.cons.car);
    atom_ref(atom->value.cons.cdr);
  }

  if (atom->type == ATOM_TYPE_LAMBDA) {
    atom_ref(atom->value.lambda.args);
    atom_ref(atom->value.lambda.body);
    ref_environment(atom->value.lambda.env);
  }

  return atom;
}

void atom_deref(struct atom *atom) {
  if (!atom) {
    return;
  } else if (atom == &g_atom_nil || atom == &g_atom_true) {
    // g_atom_nil and g_atom_true are static atoms, do not free them
    return;
  }

  // fprintf(stderr, "deref atom %p\n", (void *)atom);

  // handle values that always get ref counts incremented in atom_ref
  switch (atom->type) {
    case ATOM_TYPE_CONS:
      atom_deref(atom->value.cons.car);
      atom_deref(atom->value.cons.cdr);
      break;
    case ATOM_TYPE_LAMBDA:
      atom_deref(atom->value.lambda.args);
      atom_deref(atom->value.lambda.body);
      deref_environment(atom->value.lambda.env);
      break;
    default:
      break;
  }

  if (!g_atomic_ref_count_dec(&atom->ref_count)) {
    return;
  }

  // handle cleanup for specific atom values that aren't refcounted
  switch (atom->type) {
    case ATOM_TYPE_SYMBOL:
    case ATOM_TYPE_KEYWORD:
    case ATOM_TYPE_STRING:
      free(atom->value.string.ptr);
      break;
    default:
      break;
  }

  free(atom);
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
