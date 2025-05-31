#include "special.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atom.h"
#include "intern.h"

static struct atom *special_form(PrimitiveFunction func) {
  struct atom *atom = calloc(1, sizeof(struct atom));
  atom->type = ATOM_TYPE_SPECIAL;
  atom->value.primitive = func;
  return atom;
}

struct atom *quote(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.car) {
    fprintf(stderr, "Error: 'quote' requires one argument\n");
    return NULL;  // error handling
  }

  // Return the first element of the list, without evaluating it
  return args->value.cons.car;
}

struct atom *defun(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.car || !args->value.cons.cdr ||
      args->value.cons.cdr->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: 'defun' requires a function name and a body\n");
    return NULL;  // error handling
  }

  struct atom *name = car(args);
  if (name->type != ATOM_TYPE_SYMBOL) {
    fprintf(stderr, "Error: 'defun' first argument must be a symbol\n");
    return NULL;  // error handling
  }

  struct atom *next = cdr(args);

  struct atom *params = car(next);
  if (args->type != ATOM_TYPE_CONS) {
    fprintf(stderr,
            "Error: 'defun' second argument must be a list of parameters, or an empty list\n");
    return NULL;
  }

  next = cdr(next);

  struct atom *body = car(next);
  if (body->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: 'defun' body must be a list\n");
    return NULL;  // error handling
  }

  if (cdr(next) && cdr(next)->type != ATOM_TYPE_NIL) {
    fprintf(stderr, "Error: 'defun' expects only one body expression\n");
    return NULL;  // error handling
  }

  struct atom *func = calloc(1, sizeof(struct atom));
  func->type = ATOM_TYPE_LAMBDA;
  func->value.lambda.args = params;
  func->value.lambda.env = clone_environment(env);
  func->value.lambda.body = body;

  env_bind(env, name, func);
  return name;
}

void init_special_forms(struct environment *env) {
  env_bind(env, intern("quote", 0), special_form(quote));
  env_bind(env, intern("defun", 0), special_form(defun));
}
