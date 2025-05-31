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
    struct atom *args = fn->type == ATOM_TYPE_SPECIAL ? cdr : eval_list(cdr, env);
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
  struct environment *parent = env;
  if (fn->type == ATOM_TYPE_LAMBDA) {
    // Use closure's environment from time of definition as parent in this case
    parent = fn->value.lambda.env;
  }

  if (fn->type != ATOM_TYPE_PRIMITIVE && fn->type != ATOM_TYPE_SPECIAL) {
    env = create_environment(parent);  // create a new environment for function application
  }

  struct atom *result = NULL;
  if (fn->type == ATOM_TYPE_PRIMITIVE || fn->type == ATOM_TYPE_SPECIAL) {
    // Call the internal function
    result = fn->value.primitive(args, env);
  } else if (fn->type == ATOM_TYPE_LAMBDA) {
    // bind arguments to the function's parameters
    struct atom *params = fn->value.lambda.args;
    struct atom *current_args = args;
    while (params && params->type == ATOM_TYPE_CONS) {
      if (!current_args || current_args->type != ATOM_TYPE_CONS) {
        fprintf(stderr, "Error: not enough arguments for lambda function\n");
        free_environment(env);
        return NULL;
      }

      struct atom *param = params->value.cons.car;
      struct atom *arg = current_args->value.cons.car;

      if (param->type != ATOM_TYPE_SYMBOL) {
        fprintf(stderr, "Error: lambda parameter must be a symbol\n");
        free_environment(env);
        return NULL;
      }

      env_bind(env, param, eval(arg, parent));  // bind the argument to the parameter
      params = params->value.cons.cdr;
      current_args = current_args->value.cons.cdr;
    }
    if (params && params->type != ATOM_TYPE_NIL) {
      fprintf(stderr, "Error: too many arguments for lambda function\n");
      free_environment(env);
      return NULL;
    }
    if (current_args && current_args->type != ATOM_TYPE_NIL) {
      fprintf(stderr, "Error: not enough parameters for lambda function\n");
      free_environment(env);
      return NULL;
    }

    // Now evaluate the body of the lambda function
    result = eval(fn->value.lambda.body, env);
    if (!result) {
      fprintf(stderr, "Error evaluating lambda body\n");
      free_environment(env);
      return NULL;
    }
  } else {
    fprintf(stderr, "Error: unexpected type in apply\n");
    return NULL;
  }

  if (fn->type != ATOM_TYPE_PRIMITIVE && fn->type != ATOM_TYPE_SPECIAL) {
    free_environment(env);
  }

  return result;
}
