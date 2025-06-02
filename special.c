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
    fprintf(stderr, "Error: 'quote' requires one argument\n");
    return NULL;
  }

  // Return the first element of the list, without evaluating it
  return car(args);
}

struct atom *lambda(struct atom *args, struct environment *env) {
  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.car || !args->value.cons.cdr ||
      args->value.cons.cdr->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: 'lambda' requires a list of parameters and a body\n");
    return NULL;
  }

  struct atom *params = car(args);
  if (params != atom_nil() && params->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: 'lambda' first argument must be a list of parameters\n");
    return NULL;
  }

  struct atom *body = car(cdr(args));
  if (body->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: 'lambda' body must be a list\n");
    return NULL;
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
    fprintf(stderr, "Error: 'defun' first argument must be a symbol\n");
    return NULL;
  }

  struct atom *defn = lambda(cdr(args), env);
  if (!defn) {
    fprintf(stderr, "Error: 'defun' failed to create lambda\n");
    return NULL;
  }

  env_bind(env, name, defn);

  return name;
}

struct atom *special_form_define(struct atom *args, struct environment *env) {
  (void)env;

  struct atom *name = car(args);
  if (name->type != ATOM_TYPE_SYMBOL) {
    fprintf(stderr, "Error: 'define' first argument must be a symbol\n");
    return NULL;
  }

  struct atom *value = car(cdr(args));
  if (!value) {
    fprintf(stderr, "Error: 'define' requires a value\n");
    return NULL;
  }

  struct atom *existing = env_lookup(env, name);
  if (existing) {
    // TODO: detect interactive REPL, and only warn/error if not there
    fprintf(stderr, "Warning: 'define' overwriting existing binding for %s\n",
            name->value.string.ptr);
  }

  // Create the initial binding so it's visible in any new environments created during definition
  // (e.g. in a lambda).
  env_bind(env, name, atom_nil());

  struct atom *value_evaled = eval(value, env);
  if (!value_evaled) {
    fprintf(stderr, "Error: 'define' failed to evaluate value\n");
    return NULL;
  }

  if (value_evaled->type == ATOM_TYPE_LAMBDA) {
    env_bind(value_evaled->value.lambda.env, name, value_evaled);
  }

  // Overwrite the nil binding with the final value now that we know it.
  env_set(env, name, value_evaled);

  return name;
}

struct atom *special_form_set(struct atom *args, struct environment *env) {
  (void)env;

  struct atom *name = car(args);
  if (name->type != ATOM_TYPE_SYMBOL) {
    fprintf(stderr, "Error: 'set' first argument must be a symbol\n");
    return NULL;
  }

  struct atom *existing = env_lookup(env, name);
  if (!existing) {
    fprintf(stderr, "Error: 'set' cannot set unbound symbol '%s'\n", name->value.string.ptr);
    return NULL;
  }

  struct atom *value = car(cdr(args));
  env_set(env, name, value);

  return name;
}

struct atom *special_form_begin(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: 'begin' requires at least one argument\n");
    return NULL;
  }

  struct atom *result = atom_nil();
  while (args && args->type == ATOM_TYPE_CONS) {
    struct atom *expr = car(args);

    result = eval(expr, env);
    if (!result) {
      fprintf(stderr, "Error evaluating 'begin' expression\n");
      return NULL;
    }
    args = cdr(args);
  }

  return result;
}

struct atom *special_form_let(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.car || !args->value.cons.cdr ||
      args->value.cons.cdr->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: 'let' requires a list of bindings and a body\n");
    return NULL;
  }

  struct atom *bindings = car(args);
  if (bindings->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: 'let' first argument must be a list of bindings\n");
    return NULL;
  }

  struct atom *body = cdr(args);
  if (body->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: 'let' body must be a list\n");
    return NULL;
  }

  struct environment *let_env = create_environment(env);

  while (bindings && bindings->type == ATOM_TYPE_CONS) {
    struct atom *binding = car(bindings);
    if (binding->type != ATOM_TYPE_CONS || !binding->value.cons.car || !binding->value.cons.cdr) {
      fprintf(stderr, "Error: 'let' binding must be a (name value) pair\n");
      return NULL;
    }

    struct atom *name = car(binding);
    struct atom *value = car(cdr(binding));

    if (name->type != ATOM_TYPE_SYMBOL) {
      fprintf(stderr, "Error: 'let' binding name must be a symbol\n");
      return NULL;
    }

    struct atom *evaled_value = eval(value, let_env);
    if (!evaled_value) {
      fprintf(stderr, "Error evaluating 'let' binding value\n");
      return NULL;
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
    fprintf(stderr, "Error: 'cond' requires at least one clause\n");
    return NULL;
  }

  while (args && args->type == ATOM_TYPE_CONS) {
    struct atom *clause = car(args);
    if (clause->type != ATOM_TYPE_CONS || !clause->value.cons.car) {
      fprintf(stderr, "Error: 'cond' clause must be a (test body) pair\n");
      return NULL;
    }

    struct atom *test = car(clause);
    struct atom *body = cdr(clause);

    struct atom *evaled_test = eval(test, env);
    if (!evaled_test) {
      fprintf(stderr, "Error evaluating 'cond' test\n");
      return NULL;
    }

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
