#include "eval.h"

#include <stdio.h>
#include <stdlib.h>

#include "atom.h"
#include "env.h"
#include "eval.h"

static struct atom *eval_list(struct atom *list, struct environment *env);

struct atom *eval(struct atom *atom, struct environment *env) {
  if (!atom || is_basic_type(atom)) {
    return atom;
  }

  if (atom->type == ATOM_TYPE_SYMBOL) {
    struct atom *value = env_lookup(env, atom);
    if (!value) {
      return new_atom_error(atom, "unbound symbol");
    }

    return value;
  }

  if (atom->type == ATOM_TYPE_CONS) {
    // Eval all the elements into a new cons cell, apply first element to the rest?
    struct atom *car = atom->value.cons.car;
    struct atom *cdr = atom->value.cons.cdr;

    struct atom *fn = eval(car, env);
    if (is_error(fn)) {
      return fn;
    }

    struct atom *args = fn->type == ATOM_TYPE_SPECIAL ? cdr : eval_list(cdr, env);
    if (is_error(args)) {
      return args;
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
    if (is_error(evaled)) {
      return evaled;
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
    return new_atom_error(atom, "expected a list, got something else");
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
    return new_atom_error(fn, "expected a function, got a %s", atom_type_to_string(fn->type));
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
      break;
    }

    struct atom *param = car(binding_list);
    struct atom *arg = car(current_args);

    struct atom *evaled = eval(arg, env);
    env_bind(env, param, evaled);

    binding_list = cdr(binding_list);
    current_args = cdr(current_args);
  }

  if (binding_list && binding_list->type != ATOM_TYPE_NIL) {
    return new_atom_error(fn, "not enough arguments provided for function '%s'",
                          fn->value.lambda.args->value.string.ptr);
  }

  if (current_args && current_args->type != ATOM_TYPE_NIL) {
    return new_atom_error(fn, "too many arguments provided for function '%s'",
                          fn->value.lambda.args->value.string.ptr);
  }

  return eval(fn->value.lambda.body, env);
}
