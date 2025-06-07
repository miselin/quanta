#include "eval.h"

#include <clog.h>
#include <stdio.h>
#include <stdlib.h>

#include "atom.h"
#include "env.h"
#include "eval.h"
#include "gc.h"
#include "log.h"
#include "print.h"

static const int ENABLE_TCO = 1;

// Evaluates the given list and its sublists, if necessary, in the provided environment.
static struct atom *eval_list(struct atom *list, struct environment *env);

// Binds the arguments in the environment based on the binding list and the provided arguments.
static struct atom *bind_arguments(struct environment *env, struct atom *binding_list,
                                   struct atom *args, int should_eval);

static struct atom *apply_macro(struct atom *fn, struct atom *args, struct environment *env);

// Used to track shadow stack for roots in functions like eval_list
struct shadow_root {
  struct atom *atom;
  struct shadow_root *next;
};

struct atom *eval(struct atom *atom, struct environment *env) {
  static char buf[1024];

  size_t iter = 0;

  while (1) {
    print_str(buf, 1024, atom, 0);
    clog_debug(CLOG(LOGGER_EVAL), "eval [%zu]: %p %s", iter, (void *)atom, buf);

    ++iter;

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

      int eval_args = 1;
      if (fn->type == ATOM_TYPE_SPECIAL) {
        eval_args = 0;
      } else if (fn->value.lambda.flags & ATOM_LAMBDA_FLAG_MACRO) {
        eval_args = 0;
      }

      struct atom *args = eval_args ? eval_list(eval_cdr, env) : eval_cdr;
      if (is_error(args)) {
        return args;
      }

      if (is_lambda(fn) && (fn->value.lambda.flags & ATOM_LAMBDA_FLAG_MACRO)) {
        clog_debug(CLOG(LOGGER_EVAL), "expanding macro %s...", eval_car->value.string.ptr);
        struct atom *expanded = apply_macro(fn, args, env);
        print_str(buf, 1024, expanded, 0);
        clog_debug(CLOG(LOGGER_EVAL), "expanded macro %s to %s", eval_car->value.string.ptr, buf);
        return eval(expanded, env);
      }

      if (!ENABLE_TCO || !is_lambda(fn)) {
        return apply(fn, args, env);
      }

      // run GC before TCO loop to avoid unbounded memory growth
      // this also averages out to better performance as the stop-the-world is shorter
      gc_retain(args);
      gc_retain(fn);
      gc_run();

      // tail-call optimization - iteratively evaluate so we don't recurse
      atom = fn->value.lambda.body;
      env = create_environment(fn->value.lambda.env);
      struct atom *error = bind_arguments(env, fn->value.lambda.args, args, 1);

      gc_release(args);
      gc_release(fn);

      if (error) {
        return error;
      }
      continue;
    }

    return atom;
  }
}

static struct atom *eval_list(struct atom *atom, struct environment *env) {
  static char buf[1024];

  if (atom->type != ATOM_TYPE_CONS) {
    return atom;
  }

  print_str(buf, 1024, atom, 0);
  clog_debug(CLOG(LOGGER_EVAL), "eval_list: %p %s", (void *)atom, buf);

  struct atom *head = NULL;
  struct atom *tail = NULL;

  struct shadow_root *shadow = NULL;

  while (is_cons(atom)) {
    // Preserve the atom across GC runs (eval might trigger GC in TCO)
    gc_retain(atom);

    struct atom *evaled = eval(car(atom), env);

    gc_release(atom);

    if (is_error(evaled)) {
      return evaled;
    }

    struct atom *cons = new_cons(evaled, NULL);

    // retain the generated cons for the duration of eval_list
    gc_retain(cons);
    struct shadow_root *new_shadow = malloc(sizeof(struct shadow_root));
    new_shadow->atom = cons;
    new_shadow->next = shadow;
    shadow = new_shadow;

    if (!head) {
      head = cons;
      tail = head;
    } else {
      tail->value.cons.cdr = cons;
      tail = cons;
    }

    clog_debug(CLOG(LOGGER_EVAL), "eval_list iterating via cdr of atom %p", (void *)atom);
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

  // release shadow roots now that we have fully evaluated the list
  struct shadow_root *current = shadow;
  while (current) {
    struct shadow_root *next = current->next;
    gc_release(current->atom);
    free(current);
    current = next;
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

  struct atom *error = bind_arguments(env, fn->value.lambda.args, args, 1);
  if (error) {
    return error;
  }

  return eval(fn->value.lambda.body, env);
}

static struct atom *apply_macro(struct atom *fn, struct atom *args, struct environment *env) {
  if (!is_lambda(fn)) {
    return new_atom_error(fn, "expected a macro, got a %s", atom_type_to_string(fn->type));
  } else if ((fn->value.lambda.flags & ATOM_LAMBDA_FLAG_MACRO) == 0) {
    return new_atom_error(fn, "expected a macro, got a function");
  }

  struct environment *parent_env = env;
  if (fn->type == ATOM_TYPE_LAMBDA) {
    parent_env = fn->value.lambda.env;
  }

  env = create_environment(parent_env);

  // Bind arguments without evaluating them
  struct atom *error = bind_arguments(env, fn->value.lambda.args, args, 0);
  if (error) {
    return error;
  }

  // This will evaluate to expanded form that we're meant to evaluate
  return eval(fn->value.lambda.body, env);
}

static struct atom *bind_arguments(struct environment *env, struct atom *binding_list,
                                   struct atom *args, int should_eval) {
  clog_debug(CLOG(LOGGER_EVAL), "bind_arguments: binding_list %p args %p should_eval %d\n",
             (void *)binding_list, (void *)args, should_eval);
  struct atom *current_arg = args;
  while (binding_list && binding_list->type == ATOM_TYPE_CONS) {
    struct atom *param = car(binding_list);
    struct atom *arg = car(current_arg);

    struct atom *evaled = should_eval ? eval(arg, env) : arg;
    struct atom *bound = env_bind(env, param, evaled);
    if (is_error(bound)) {
      return bound;
    }

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
