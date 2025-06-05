#include "special.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atom.h"
#include "eval.h"
#include "intern.h"

static struct atom *special_form(PrimitiveFunction func) {
  union atom_value value = {.primitive = func};
  return new_atom(ATOM_TYPE_SPECIAL, value);
}

struct atom *quote(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.car) {
    return new_atom_error(args, "Error: 'quote' requires one argument");
  }

  // Return the first element of the list, without evaluating it
  return car(args);
}

struct atom *lambda(struct atom *args, struct environment *env) {
  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.car || !args->value.cons.cdr ||
      args->value.cons.cdr->type != ATOM_TYPE_CONS) {
    return new_atom_error(args, "Error: 'lambda' requires a list of parameters and a body");
  }

  struct atom *params = car(args);
  if (params != atom_nil() && params->type != ATOM_TYPE_CONS) {
    return new_atom_error(params, "Error: 'lambda' first argument must be a list of parameters");
  }

  struct atom *body = car(cdr(args));
  if (body->type != ATOM_TYPE_CONS) {
    return new_atom_error(body, "Error: 'lambda' body must be a list");
  }

  union atom_value value = {
      .lambda = {.args = params, .env = clone_environment(env), .body = body}};

  return new_atom(ATOM_TYPE_LAMBDA, value);
}

// Syntax sugar for (define name (lambda args body))
struct atom *defun(struct atom *args, struct environment *env) {
  (void)env;

  struct atom *name = car(args);
  if (name->type != ATOM_TYPE_SYMBOL) {
    return new_atom_error(name, "Error: 'defun' first argument must be a symbol");
  }

  // placeholder binding for self-references
  env_bind(env, name, atom_nil());

  struct atom *defn = lambda(cdr(args), env);
  if (is_error(defn)) {
    return defn;
  }

  env_set(env, name, defn);

  return name;
}

struct atom *special_form_define(struct atom *args, struct environment *env) {
  (void)env;

  struct atom *name = car(args);
  if (name->type != ATOM_TYPE_SYMBOL) {
    return new_atom_error(name, "Error: 'define' first argument must be a symbol");
  }

  struct atom *value = car(cdr(args));
  if (!value) {
    return new_atom_error(name, "Error: 'define' requires a value");
  }

  struct atom *existing = env_lookup(env, name);
  if (existing) {
    return new_atom_error(name,
                          "Error: 'define' cannot overwrite existing binding for '%s' (use set!)",
                          name->value.string.ptr);
  }

  // placeholder binding for self-references
  env_bind(env, name, atom_nil());

  struct atom *value_evaled = eval(value, env);
  if (is_error(value_evaled)) {
    return value_evaled;
  }

  if (value_evaled->type == ATOM_TYPE_LAMBDA) {
    env_bind(value_evaled->value.lambda.env, name, value_evaled);
  }

  env_set(env, name, value_evaled);

  return name;
}

struct atom *special_form_set(struct atom *args, struct environment *env) {
  (void)env;

  struct atom *name = car(args);
  if (name->type != ATOM_TYPE_SYMBOL) {
    return new_atom_error(name, "Error: 'set!' first argument must be a symbol");
  }

  struct atom *existing = env_lookup(env, name);
  if (!existing) {
    return new_atom_error(name, "Error: 'set!' cannot set unbound symbol '%s' (use define first)",
                          name->value.string.ptr);
  }

  struct atom *value = car(cdr(args));
  if (is_error(value)) {
    return value;
  }
  env_set(env, name, value);

  return name;
}

struct atom *special_form_begin(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS) {
    return new_atom_error(args, "Error: 'begin' requires at least one expression");
  }

  struct atom *result = atom_nil();
  while (args && args->type == ATOM_TYPE_CONS) {
    struct atom *expr = car(args);

    result = eval(expr, env);
    if (is_error(result)) {
      return result;
    }
    args = cdr(args);
  }

  return result;
}

struct atom *special_form_let(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.car || !args->value.cons.cdr ||
      args->value.cons.cdr->type != ATOM_TYPE_CONS) {
    return new_atom_error(args, "Error: 'let' requires a list of bindings and a body");
  }

  struct atom *bindings = car(args);
  if (bindings->type != ATOM_TYPE_CONS) {
    return new_atom_error(bindings, "Error: 'let' first argument must be a list of bindings");
  }

  struct atom *body = cdr(args);
  if (body->type != ATOM_TYPE_CONS) {
    return new_atom_error(body, "Error: 'let' body must be a list");
  }

  struct environment *let_env = create_environment(env);

  while (bindings && bindings->type == ATOM_TYPE_CONS) {
    struct atom *binding = car(bindings);
    if (binding->type != ATOM_TYPE_CONS || !binding->value.cons.car || !binding->value.cons.cdr) {
      return new_atom_error(binding, "Error: 'let' binding must be a (name value) pair");
    }

    struct atom *name = car(binding);
    struct atom *value = car(cdr(binding));

    if (name->type != ATOM_TYPE_SYMBOL) {
      return new_atom_error(name, "Error: 'let' binding name must be a symbol, got %s",
                            atom_type_to_string(name->type));
    }

    struct atom *evaled_value = eval(value, let_env);
    if (is_error(evaled_value)) {
      return evaled_value;
    }

    env_bind(let_env, name, evaled_value);

    bindings = cdr(bindings);
  }

  struct atom *result = special_form_begin(body, let_env);

  return result;
}

struct atom *special_form_cond(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS) {
    return new_atom_error(args, "Error: 'cond' requires at least one clause");
  }

  while (args && args->type == ATOM_TYPE_CONS) {
    struct atom *clause = car(args);
    if (clause->type != ATOM_TYPE_CONS || !clause->value.cons.car) {
      return new_atom_error(clause, "Error: 'cond' clause must be a (test body) pair, got %s",
                            atom_type_to_string(clause->type));
    }

    struct atom *test = car(clause);
    struct atom *body = cdr(clause);

    struct atom *evaled_test = eval(test, env);
    if (is_error(evaled_test)) {
      return evaled_test;
    }

    // short-circuit evaluation - don't evaluate tests after the first true
    if (is_true(evaled_test)) {
      return special_form_begin(body, env);
    }

    args = cdr(args);
  }

  return atom_nil();
}

void init_special_forms(struct environment *env) {
  env_bind(env, intern("quote", 0), special_form(quote));
  env_bind(env, intern("lambda", 0), special_form(lambda));
  env_bind(env, intern("begin", 0), special_form(special_form_begin));
  env_bind(env, intern("define", 0), special_form(special_form_define));
  env_bind(env, intern("defun", 0), special_form(defun));
  env_bind(env, intern("set!", 0), special_form(special_form_set));
  env_bind(env, intern("let", 0), special_form(special_form_let));
  env_bind(env, intern("cond", 0), special_form(special_form_cond));
}
