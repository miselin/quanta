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

  if (!is_cons(args) || car(args) == atom_nil() || cdr(args) != atom_nil()) {
    return new_atom_error(args, "Error: 'quote' requires exactly one argument");
  }

  return car(args);
}

static struct atom *quasiquote_atom(struct atom *atom, struct environment *env, int depth) {
  if (is_cons(atom)) {
    struct atom *head = car(atom);
    if (head == intern("quasiquote", 0)) {
      // don't evaluate the quasiquote itself yet
      return new_cons(intern("quasiquote", 0),
                      new_cons(quasiquote_atom(car(cdr(atom)), env, depth + 1), atom_nil()));
    } else if (head == intern("unquote", 0)) {
      if (depth == 0) {
        // unquote here
        return eval(car(cdr(atom)), env);
      } else {
        // don't evaluate this unquote yet, stash it for later but continue quasiquoting
        return new_cons(intern("unquote", 0),
                        new_cons(quasiquote_atom(car(cdr(atom)), env, depth - 1), atom_nil()));
      }
    }

    struct atom *quasi_car = quasiquote_atom(car(atom), env, depth);
    struct atom *quasi_cdr = quasiquote_atom(cdr(atom), env, depth);

    return new_cons(quasi_car, quasi_cdr);
  }

  return atom;
}

struct atom *quasiquote(struct atom *args, struct environment *env) {
  (void)env;

  if (!is_cons(args) || car(args) == atom_nil() || cdr(args) != atom_nil()) {
    return new_atom_error(args, "Error: 'quasiquote' requires exactly one argument");
  }

  return quasiquote_atom(car(args), env, 0);
}

struct atom *unquote(struct atom *args, struct environment *env) {
  (void)env;
  (void)args;

  // this special form exists only for this helpful error message
  // quasiquote handles unquoting itself during evaluation
  return new_atom_error(args, "Error: 'unquote' is only valid inside a 'quasiquote'");
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

struct atom *defmacro(struct atom *args, struct environment *env) {
  (void)env;

  struct atom *name = car(args);
  if (!is_symbol(name)) {
    return new_atom_error(name, "Error: 'defun' first argument must be a symbol");
  }

  struct atom *defn = lambda(cdr(args), env);
  if (is_error(defn)) {
    return defn;
  }

  defn->value.lambda.flags |= ATOM_LAMBDA_FLAG_MACRO;

  env_bind(env, name, defn);

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

  struct atom *value = eval(car(cdr(args)), env);
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
  env_bind(env, intern("quasiquote", 0), special_form(quasiquote));
  env_bind(env, intern("unquote", 0), special_form(unquote));

  env_bind(env, intern("lambda", 0), special_form(lambda));
  env_bind(env, intern("begin", 0), special_form(special_form_begin));
  env_bind(env, intern("define", 0), special_form(special_form_define));
  env_bind(env, intern("defun", 0), special_form(defun));
  env_bind(env, intern("defmacro", 0), special_form(defmacro));
  env_bind(env, intern("set!", 0), special_form(special_form_set));
  env_bind(env, intern("let", 0), special_form(special_form_let));
  env_bind(env, intern("cond", 0), special_form(special_form_cond));
}
