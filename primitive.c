#include "primitive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atom.h"
#include "eval.h"
#include "intern.h"
#include "print.h"

static struct atom *primitive_function(PrimitiveFunction func) {
  union atom_value value = {.primitive = func};
  return new_atom(ATOM_TYPE_PRIMITIVE, value);
}

struct atom *primitive_cons(struct atom *args, struct environment *env) {
  (void)env;

  // TODO: check types, error handling

  struct atom *cell_car = car(args);
  struct atom *cell_cdr = car(cdr(args));
  return new_cons(atom_ref(cell_car), atom_ref(cell_cdr));
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
    return NULL;
  }

  union atom_value value = {.ivalue = 0};

  // get first argument to determine the result type of the expression
  struct atom *first_arg = car(args);
  enum AtomType type = first_arg->type;

  if (type == ATOM_TYPE_INT) {
    value.ivalue = first_arg->value.ivalue;
  } else if (type == ATOM_TYPE_FLOAT) {
    value.fvalue = first_arg->value.fvalue;
    type = ATOM_TYPE_FLOAT;
  } else {
    fprintf(stderr, "Error: '+' only supports integers and floats\n");
    return NULL;
  }

  // perform the arithmetic
  while (args && args->type == ATOM_TYPE_CONS) {
    struct atom *arg = car(args);
    if (arg->type != type) {
      fprintf(stderr, "Error: '+' requires all arguments to be of the same type\n");
      return NULL;
    } else if (arg->type == ATOM_TYPE_INT) {
      value.ivalue += arg->value.ivalue;
    } else if (arg->type == ATOM_TYPE_FLOAT) {
      value.fvalue += arg->value.fvalue;
    } else {
      fprintf(stderr, "Error: '+' only supports integers and floats\n");
      return NULL;
    }

    args = cdr(args);
  }

  return new_atom(type, value);
}

struct atom *primitive_subtract(struct atom *args, struct environment *env) {
  (void)args;
  (void)env;

  return atom_nil();
}

struct atom *primitive_multiply(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: '*' requires at least one argument\n");
    return NULL;
  }

  union atom_value value = {.ivalue = 0};

  // get first argument to determine the result type of the expression
  struct atom *first_arg = car(args);
  enum AtomType type = first_arg->type;

  if (type == ATOM_TYPE_INT) {
    value.ivalue = first_arg->value.ivalue;
  } else if (type == ATOM_TYPE_FLOAT) {
    value.fvalue = first_arg->value.fvalue;
    type = ATOM_TYPE_FLOAT;
  } else {
    fprintf(stderr, "Error: '*' only supports integers and floats\n");
    return NULL;
  }

  args = cdr(args);
  if (!args || args->type != ATOM_TYPE_CONS) {
    // If there's only one argument, return it as the result
    return atom_ref(first_arg);
  }

  // perform the arithmetic
  while (args && args->type == ATOM_TYPE_CONS) {
    struct atom *arg = car(args);
    if (arg->type != type) {
      fprintf(stderr, "Error: '*' requires all arguments to be of the same type\n");
      return NULL;
    } else if (arg->type == ATOM_TYPE_INT) {
      value.ivalue *= arg->value.ivalue;
    } else if (arg->type == ATOM_TYPE_FLOAT) {
      value.fvalue *= arg->value.fvalue;
    } else {
      fprintf(stderr, "Error: '*' only supports integers and floats\n");
      return NULL;
    }

    args = cdr(args);
  }

  return new_atom(type, value);
}

struct atom *primitive_divide(struct atom *args, struct environment *env) {
  (void)args;
  (void)env;

  fprintf(stderr, "divide not implemented yet\n");
  return atom_nil();
}

struct atom *primitive_equal(struct atom *args, struct environment *env) {
  (void)env;

  // Must be two arguments
  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.cdr ||
      args->value.cons.cdr->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: '=' requires exactly two arguments\n");
    return NULL;  // error handling
  }

  struct atom *first = car(args);
  struct atom *second = car(cdr(args));

  if (first->type != second->type) {
    return atom_nil();
  }

  int equal = 0;
  switch (first->type) {
    case ATOM_TYPE_INT:
      equal = (first->value.ivalue == second->value.ivalue);
      break;
    case ATOM_TYPE_FLOAT:
      equal = (first->value.fvalue == second->value.fvalue);
      break;
    case ATOM_TYPE_STRING:
      equal = (strcmp(first->value.string.ptr, second->value.string.ptr) == 0);
      break;
    case ATOM_TYPE_TRUE:
      equal = (second->type == ATOM_TYPE_TRUE);
      break;
    case ATOM_TYPE_SYMBOL:
    case ATOM_TYPE_KEYWORD:
      // interned strings, we can just compare the pointers
      equal = first->value.string.ptr == second->value.string.ptr;
      break;
    case ATOM_TYPE_PRIMITIVE:
      // Primitive functions are equal if they point to the same function
      equal = first->value.primitive == second->value.primitive;
      break;
    case ATOM_TYPE_SPECIAL:
      return atom_nil();
      break;
    case ATOM_TYPE_CONS:
    case ATOM_TYPE_NIL:
    case ATOM_TYPE_LAMBDA:
      // Identity equality (not structural equality)
      equal = first == second;
      break;
  }

  return equal ? atom_true() : atom_nil();
}

struct atom *primitive_atomp(struct atom *args, struct environment *env) {
  (void)env;

  struct atom *arg = car(args);
  if (arg->type == ATOM_TYPE_INT || arg->type == ATOM_TYPE_FLOAT || arg->type == ATOM_TYPE_STRING ||
      arg->type == ATOM_TYPE_SYMBOL || arg->type == ATOM_TYPE_KEYWORD ||
      arg->type == ATOM_TYPE_TRUE || arg->type == ATOM_TYPE_NIL) {
    return atom_true();
  }

  return atom_nil();
}

struct atom *primitive_nilp(struct atom *args, struct environment *env) {
  (void)env;

  struct atom *arg = car(args);
  return is_nil(arg) ? atom_true() : atom_nil();
}

struct atom *primitive_apply(struct atom *args, struct environment *env) {
  (void)env;

  // TODO: checks etc

  struct atom *fn = car(args);
  struct atom *arguments = cdr(args);

  return apply(fn, arguments, env);
}

struct atom *primitive_eval(struct atom *args, struct environment *env) {
  (void)env;

  return eval(args, env);
}

struct atom *primitive_not_equal(struct atom *args, struct environment *env);
struct atom *primitive_less_than(struct atom *args, struct environment *env);
struct atom *primitive_greater_than(struct atom *args, struct environment *env);
struct atom *primitive_less_than_equal(struct atom *args, struct environment *env);
struct atom *primitive_greater_than_equal(struct atom *args, struct environment *env);

struct atom *primitive_print(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS) {
    fprintf(stderr, "Error: 'print' requires at least one argument\n");
    return NULL;
  }

  while (args && args->type == ATOM_TYPE_CONS) {
    print(car(args));
    printf("\n");
    args = cdr(args);
  }

  return atom_nil();
}

void init_primitives(struct environment *env) {
  // Add primitive functions to the environment
  env_bind_noref(env, intern_noref("+", 0), primitive_function(primitive_add));
  env_bind_noref(env, intern_noref("-", 0), primitive_function(primitive_subtract));
  env_bind_noref(env, intern_noref("*", 0), primitive_function(primitive_multiply));
  env_bind_noref(env, intern_noref("/", 0), primitive_function(primitive_divide));

  env_bind_noref(env, intern_noref("eq?", 0), primitive_function(primitive_equal));
  /*
  env_bind_noref(env, intern_noref("!=", 0), primitive_function(primitive_not_equal));
  env_bind_noref(env, intern_noref("<", 0), primitive_function(primitive_less_than));
  env_bind_noref(env, intern_noref(">", 0), primitive_function(primitive_greater_than));
  env_bind_noref(env, intern_noref("<=", 0), primitive_function(primitive_less_than_equal));
  env_bind_noref(env, intern_noref(">=", 0), primitive_function(primitive_greater_than_equal));
  */

  env_bind_noref(env, intern_noref("cons", 0), primitive_function(primitive_cons));
  env_bind_noref(env, intern_noref("car", 0), primitive_function(primitive_car));
  env_bind_noref(env, intern_noref("cdr", 0), primitive_function(primitive_cdr));

  env_bind_noref(env, intern_noref("atom?", 0), primitive_function(primitive_atomp));
  env_bind_noref(env, intern_noref("nil?", 0), primitive_function(primitive_nilp));

  env_bind_noref(env, intern_noref("apply", 0), primitive_function(primitive_apply));
  env_bind_noref(env, intern_noref("eval", 0), primitive_function(primitive_eval));

  env_bind_noref(env, intern_noref("print", 0), primitive_function(primitive_print));
}
