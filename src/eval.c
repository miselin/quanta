#include "eval.h"

#include <clog.h>
#include <stdio.h>
#include <stdlib.h>

#include "atom.h"
#include "env.h"
#include "eval.h"
#include "log.h"
#include "print.h"

static const int ENABLE_TCO = 1;

// Evaluates the given list and its sublists, if necessary, in the provided environment.
static struct atom *eval_list(struct atom *list, struct environment *env);

// Binds the arguments in the environment based on the binding list and the provided arguments.
static struct atom *bind_arguments(struct environment *env, struct atom *binding_list,
                                   struct atom *args);

struct atom *eval(struct atom *atom, struct environment *env) {
  static char buf[1024];
  print_str(buf, 1024, atom, 0);
  clog_debug(CLOG(LOGGER_EVAL), "eval: %s", buf);

  while (1) {
    if (!atom || is_basic_type(atom)) {
      return atom;
    }

    if (atom->type == ATOM_TYPE_SYMBOL) {
      struct atom *value = env_lookup(env, atom);
      if (!value) {
        return new_atom_error(atom, "unbound symbol '%s'", atom->value.string.ptr);
      }

      return value;
    }

    if (atom->type == ATOM_TYPE_CONS) {
      struct atom *eval_car = car(atom);
      struct atom *eval_cdr = cdr(atom);

      struct atom *fn = eval(eval_car, env);
      if (is_error(fn)) {
        return fn;
      }

      struct atom *args = fn->type == ATOM_TYPE_SPECIAL ? eval_cdr : eval_list(eval_cdr, env);
      if (is_error(args)) {
        return args;
      }

      if (!ENABLE_TCO || !is_lambda(fn)) {
        return apply(fn, args, env);
      }

      // tail-call optimization - iteratively evaluate so we don't recurse
      atom = fn->value.lambda.body;
      env = create_environment(fn->value.lambda.env);
      bind_arguments(env, fn->value.lambda.args, args);
      continue;
    }

    return atom;
  }
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
  if (is_primitive(fn) || is_special(fn)) {
    // Call the internal function - no environment cloning needed
    return fn->value.primitive(args, env);
  } else if (!is_lambda(fn)) {
    return new_atom_error(fn, "expected a function, got a %s", atom_type_to_string(fn->type));
  }

  struct environment *parent_env = env;
  if (fn->type == ATOM_TYPE_LAMBDA) {
    parent_env = fn->value.lambda.env;
  }

  env = create_environment(parent_env);

  struct atom *error = bind_arguments(env, fn->value.lambda.args, args);
  if (error) {
    return error;
  }

  return eval(fn->value.lambda.body, env);
}

static struct atom *bind_arguments(struct environment *env, struct atom *binding_list,
                                   struct atom *args) {
  struct atom *current_arg = args;
  while (binding_list && binding_list->type == ATOM_TYPE_CONS) {
    struct atom *param = car(binding_list);
    struct atom *arg = car(current_arg);

    struct atom *evaled = eval(arg, env);
    env_bind(env, param, evaled);

    binding_list = cdr(binding_list);
    current_arg = cdr(current_arg);

    if (!is_cons(current_arg)) {
      break;
    }
  }

  if (!is_nil(binding_list)) {
    return new_atom_error(binding_list, "not enough arguments provided for function");
  }

  if (!is_nil(current_arg)) {
    return new_atom_error(current_arg, "too many arguments provided for function");
  }

  // rare usage of NULL return to indicate success and simplify error propagation
  // if (error_atom = bind_arguments(...)) { return error_atom; }}
  return NULL;
}
