#include "eval.h"

#include <stdio.h>
#include <stdlib.h>

#include "atom.h"
#include "env.h"
#include "eval.h"
#include "print.h"

static struct atom *eval_list(struct atom *list, struct environment *env);

struct atom *eval(struct atom *atom, struct environment *env) {
  fprintf(stderr, "eval: ");
  print(stderr, atom);
  fprintf(stderr, "\n");

  if (!atom || is_basic_type(atom)) {
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
    if (!fn) {
      fprintf(stderr, "Error evaluating function\n");
      return NULL;
    }

    struct atom *args = fn->type == ATOM_TYPE_SPECIAL ? cdr : eval_list(cdr, env);
    if (!args) {
      fprintf(stderr, "Error evaluating arguments\n");
      return NULL;
    }

    // TODO: tail-recursion optimization (iterative evaluation instead of recursive)
    struct atom *result = apply(fn, args, env);
    return result;
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
    struct atom *evaled = eval(car(atom), env);
    if (!evaled) {
      fprintf(stderr, "Error evaluating list element\n");
      return NULL;
    }

    struct atom *cons = new_cons(evaled, NULL);

    if (!head) {
      head = cons;
      tail = head;
    } else {
      tail->value.cons.cdr = cons;
      tail = cons;
    }

    atom = cdr(atom);
  }

  if (atom && atom->type != ATOM_TYPE_NIL) {
    fprintf(stderr, "Error: expected a list, got something else\n");
    return NULL;  // error handling
  }

  if (tail) {
    tail->value.cons.cdr = atom_nil();
  } else {
    head = atom_nil();
  }

  return head;
}

struct atom *apply(struct atom *fn, struct atom *args, struct environment *env) {
  if (fn->type == ATOM_TYPE_PRIMITIVE || fn->type == ATOM_TYPE_SPECIAL) {
    // Call the internal function - no environment cloning needed
    return fn->value.primitive(args, env);
  } else if (fn->type != ATOM_TYPE_LAMBDA) {
    fprintf(stderr, "Error: expected a function\n");
    return NULL;
  }

  struct environment *parent_env = env;
  if (fn->type == ATOM_TYPE_LAMBDA) {
    parent_env = fn->value.lambda.env;
  }

  env = create_environment(parent_env);

  struct atom *binding_list = fn->value.lambda.args;
  struct atom *current_args = args;
  while (binding_list && binding_list->type == ATOM_TYPE_CONS) {
    if (!current_args || current_args->type != ATOM_TYPE_CONS) {
      fprintf(stderr, "Error: not enough arguments for lambda function\n");
      return NULL;
    }

    struct atom *param = car(binding_list);
    struct atom *arg = car(current_args);

    struct atom *evaled = eval(arg, env);
    env_bind(env, param, evaled);

    binding_list = cdr(binding_list);
    current_args = cdr(current_args);
  }

  if (binding_list && binding_list->type != ATOM_TYPE_NIL) {
    fprintf(stderr, "Error: not enough parameters for function\n");
    return NULL;
  }

  if (current_args && current_args->type != ATOM_TYPE_NIL) {
    fprintf(stderr, "Error: too many parameters for function\n");
    return NULL;
  }

  struct atom *result = eval(fn->value.lambda.body, env);
  if (!result) {
    fprintf(stderr, "Error evaluating lambda body\n");
    return NULL;
  }

  return result;
}
