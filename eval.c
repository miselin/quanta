#include "eval.h"

#include <stdio.h>
#include <stdlib.h>

#include "atom.h"
#include "env.h"

static struct atom *eval_list(struct atom *list, struct environment *env);

struct atom *eval(struct atom *atom, struct environment *env) {
  if (!atom || atom->type == ATOM_TYPE_INT || atom->type == ATOM_TYPE_FLOAT ||
      atom->type == ATOM_TYPE_STRING || atom->type == ATOM_TYPE_NIL ||
      atom->type == ATOM_TYPE_TRUE) {
    return atom;
  }

  if (atom->type == ATOM_TYPE_SYMBOL) {
    struct atom *value = env_lookup(env, atom);
    if (!value) {
      fprintf(stderr, "Error: unbound symbol '%s'\n", atom->value.string.ptr);
      return NULL;
    }
    return value;
  }

  if (atom->type == ATOM_TYPE_CONS) {
    // Eval all the elements into a new cons cell, apply first element to the rest?
    struct atom *car = atom->value.cons.car;
    struct atom *cdr = atom->value.cons.cdr;

    struct atom *fn = eval(car, env);
    struct atom *args = car->type == ATOM_TYPE_SPECIAL ? cdr : eval_list(cdr, env);
    if (!fn || !args) {
      fprintf(stderr, "Error: invalid function or arguments\n");
      return NULL;
    }

    // TODO: tail-recursion optimization (iterative evaluation instead of recursive)
    return apply(fn, args, env);
  }

  return atom;
}

static struct atom *eval_list(struct atom *atom, struct environment *env) {
  if (atom->type != ATOM_TYPE_CONS) {
    return atom;
  }

  struct atom *head = NULL;
  struct atom *tail = NULL;

  while (atom && atom->type == ATOM_TYPE_CONS) {
    struct atom *evaled = eval(atom->value.cons.car, env);
    if (!evaled) {
      fprintf(stderr, "Error evaluating list element\n");
      return NULL;
    }

    struct atom *new_cons = calloc(1, sizeof(struct atom));
    new_cons->type = ATOM_TYPE_CONS;
    new_cons->value.cons.car = evaled;
    new_cons->value.cons.cdr = NULL;

    if (!head) {
      head = new_cons;
      tail = head;
    } else {
      tail->value.cons.cdr = new_cons;
      tail = new_cons;
    }

    atom = atom->value.cons.cdr;  // move to the next element
  }
  if (atom && atom->type != ATOM_TYPE_NIL) {
    fprintf(stderr, "Error: expected a list, got something else\n");
    return NULL;  // error handling
  }

  // if we reached here, we have a valid evaluated list
  struct atom *nil_atom = calloc(1, sizeof(struct atom));
  nil_atom->type = ATOM_TYPE_NIL;
  if (tail) {
    tail->value.cons.cdr = nil_atom;
  } else {
    head = nil_atom;
  }

  return head;
}

struct atom *apply(struct atom *fn, struct atom *args, struct environment *env) {
  if (fn->type != ATOM_TYPE_PRIMITIVE) {
    env = create_environment(env);  // create a new environment for function application
  }

  struct atom *result = NULL;
  if (fn->type == ATOM_TYPE_PRIMITIVE) {
    // Call the primitive function
    result = fn->value.primitive(args, env);
  } else {
    // TODO
    fprintf(stderr, "Error: non-primitive function application not implemented\n");
  }

  if (fn->type != ATOM_TYPE_PRIMITIVE) {
    free_environment(env);
  }

  return result;
}
