#include "primitive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atom.h"
#include "intern.h"

static struct atom *primitive_function(PrimitiveFunction func) {
  struct atom *atom = calloc(1, sizeof(struct atom));
  atom->type = ATOM_TYPE_PRIMITIVE;
  atom->value.primitive = func;
  return atom;
}

struct atom *primitive_car(struct atom *args, struct environment *env) {
  (void)env;

  if (!args) {
    fprintf(stderr, "Error: 'car' requires a non-empty list\n");
    return NULL;  // error handling
  }

  return car(args);
}

struct atom *primitive_cdr(struct atom *args, struct environment *env) {
  (void)env;

  if (!args) {
    fprintf(stderr, "Error: 'cdr' requires a non-empty list\n");
    return NULL;  // error handling
  }

  return cdr(args);
}

struct atom *primitive_add(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: '+' requires at least one argument\n");
    return NULL;  // error handling
  }

  struct atom *result = calloc(1, sizeof(struct atom));
  result->type = ATOM_TYPE_INT;
  result->value.ivalue = 0;  // Initialize result to zero

  struct atom *current = args;
  while (current && current->type == ATOM_TYPE_CONS) {
    struct atom *arg = car(current);
    if (arg->type == ATOM_TYPE_INT) {
      result->value.ivalue += arg->value.ivalue;  // Add integer values
    } else if (arg->type == ATOM_TYPE_FLOAT) {
      result->type = ATOM_TYPE_FLOAT;             // Change result type to float
      result->value.fvalue += arg->value.fvalue;  // Add float values
    } else {
      fprintf(stderr, "Error: '+' only supports integers and floats\n");
      free_atom(result);
      return NULL;  // error handling
    }
    current = cdr(current);
  }

  if (current && current->type != ATOM_TYPE_NIL) {
    fprintf(stderr, "Error: '+' expects a list of arguments\n");
    free_atom(result);
    return NULL;  // error handling
  }

  return result;
}

struct atom *primitive_subtract(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: '-' requires at least one argument\n");
    return NULL;  // error handling
  }

  struct atom *result = calloc(1, sizeof(struct atom));
  result->type = ATOM_TYPE_INT;
  result->value.ivalue = 0;  // Initialize result to zero

  struct atom *current = args;
  while (current && current->type == ATOM_TYPE_CONS) {
    struct atom *arg = current->value.cons.car;
    if (arg->type == ATOM_TYPE_INT) {
      result->value.ivalue -= arg->value.ivalue;  // Add integer values
    } else if (arg->type == ATOM_TYPE_FLOAT) {
      result->type = ATOM_TYPE_FLOAT;             // Change result type to float
      result->value.fvalue -= arg->value.fvalue;  // Add float values
    } else {
      fprintf(stderr, "Error: '-' only supports integers and floats\n");
      free_atom(result);
      return NULL;  // error handling
    }
    current = current->value.cons.cdr;
  }

  if (current && current->type != ATOM_TYPE_NIL) {
    fprintf(stderr, "Error: '-' expects a list of arguments\n");
    free_atom(result);
    return NULL;  // error handling
  }

  return result;
}

struct atom *primitive_multiply(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: '*' requires at least one argument\n");
    return NULL;  // error handling
  }

  struct atom *result = calloc(1, sizeof(struct atom));
  result->type = ATOM_TYPE_INT;
  result->value.ivalue = 1;

  struct atom *current = args;
  while (current && current->type == ATOM_TYPE_CONS) {
    struct atom *arg = current->value.cons.car;
    if (arg->type == ATOM_TYPE_INT) {
      result->value.ivalue *= arg->value.ivalue;  // Add integer values
    } else if (arg->type == ATOM_TYPE_FLOAT) {
      result->type = ATOM_TYPE_FLOAT;             // Change result type to float
      result->value.fvalue *= arg->value.fvalue;  // Add float values
    } else {
      fprintf(stderr, "Error: '*' only supports integers and floats\n");
      free_atom(result);
      return NULL;  // error handling
    }
    current = current->value.cons.cdr;
  }

  if (current && current->type != ATOM_TYPE_NIL) {
    fprintf(stderr, "Error: '*' expects a list of arguments\n");
    free_atom(result);
    return NULL;  // error handling
  }

  return result;
}

struct atom *primitive_divide(struct atom *args, struct environment *env) {
  (void)args;
  (void)env;

  fprintf(stderr, "divide not implemented yet\n");
  return NULL;
}

struct atom *primitive_equal(struct atom *args, struct environment *env) {
  (void)env;

  // Must be two arguments
  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.cdr ||
      args->value.cons.cdr->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: '=' requires exactly two arguments\n");
    return NULL;  // error handling
  }

  struct atom *first = args->value.cons.car;
  struct atom *second = args->value.cons.cdr->value.cons.car;
  struct atom *result = calloc(1, sizeof(struct atom));
  result->type = ATOM_TYPE_TRUE;  // Assume true unless proven otherwise

  if (first->type != second->type) {
    result->type = ATOM_TYPE_NIL;
    return result;
  }

  switch (first->type) {
    case ATOM_TYPE_INT:
      result->type = (first->value.ivalue == second->value.ivalue) ? ATOM_TYPE_TRUE : ATOM_TYPE_NIL;
      break;
    case ATOM_TYPE_FLOAT:
      result->type = (first->value.fvalue == second->value.fvalue) ? ATOM_TYPE_TRUE : ATOM_TYPE_NIL;
      break;
    case ATOM_TYPE_STRING:
      result->type = (strcmp(first->value.string.ptr, second->value.string.ptr) == 0)
                         ? ATOM_TYPE_TRUE
                         : ATOM_TYPE_NIL;
      break;
    case ATOM_TYPE_NIL:
      result->type = (first->type == ATOM_TYPE_NIL) ? ATOM_TYPE_TRUE : ATOM_TYPE_NIL;
      break;
    case ATOM_TYPE_TRUE:
      result->type = (second->type == ATOM_TYPE_TRUE) ? ATOM_TYPE_TRUE : ATOM_TYPE_NIL;
      break;
    case ATOM_TYPE_SYMBOL:
    case ATOM_TYPE_KEYWORD:
      // interned strings, we can just compare the pointers
      result->type =
          first->value.string.ptr == second->value.string.ptr ? ATOM_TYPE_TRUE : ATOM_TYPE_NIL;
      break;
    case ATOM_TYPE_PRIMITIVE:
      // Primitive functions are equal if they point to the same function
      result->type =
          (first->value.primitive == second->value.primitive ? ATOM_TYPE_TRUE : ATOM_TYPE_NIL);
      break;
    case ATOM_TYPE_SPECIAL:
      // Special forms are not compared here, as they are not typically equal
      result->type = ATOM_TYPE_NIL;
      break;
    case ATOM_TYPE_CONS:
      // For cons cells, we can only check if they are the same object
      result->type = (first == second ? ATOM_TYPE_TRUE : ATOM_TYPE_NIL);
      break;
    case ATOM_TYPE_LAMBDA:
      // For user-defined functions, we can only check if they are the same object
      result->type = (first == second ? ATOM_TYPE_TRUE : ATOM_TYPE_NIL);
      break;
  }

  return result;
}

struct atom *primitive_not_equal(struct atom *args, struct environment *env);
struct atom *primitive_less_than(struct atom *args, struct environment *env);
struct atom *primitive_greater_than(struct atom *args, struct environment *env);
struct atom *primitive_less_than_equal(struct atom *args, struct environment *env);
struct atom *primitive_greater_than_equal(struct atom *args, struct environment *env);

struct atom *primitive_cons(struct atom *args, struct environment *env);
struct atom *primitive_car(struct atom *args, struct environment *env);
struct atom *primitive_cdr(struct atom *args, struct environment *env);

void init_primitives(struct environment *env) {
  // Add primitive functions to the environment
  env_bind(env, intern("+", 0), primitive_function(primitive_add));
  env_bind(env, intern("-", 0), primitive_function(primitive_subtract));
  env_bind(env, intern("*", 0), primitive_function(primitive_multiply));
  env_bind(env, intern("/", 0), primitive_function(primitive_divide));

  env_bind(env, intern("=", 0), primitive_function(primitive_equal));
  /*
  env_bind(env, intern("!=", 0), primitive_function(primitive_not_equal));
  env_bind(env, intern("<", 0), primitive_function(primitive_less_than));
  env_bind(env, intern(">", 0), primitive_function(primitive_greater_than));
  env_bind(env, intern("<=", 0), primitive_function(primitive_less_than_equal));
  env_bind(env, intern(">=", 0), primitive_function(primitive_greater_than_equal));

  env_bind(env, intern("cons", 0), primitive_function(primitive_cons));
  */
  env_bind(env, intern("car", 0), primitive_function(primitive_car));
  env_bind(env, intern("cdr", 0), primitive_function(primitive_cdr));
}
