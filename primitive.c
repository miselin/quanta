#include "primitive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atom.h"
#include "eval.h"
#include "intern.h"
#include "print.h"
#include "read.h"

static struct atom *primitive_function(PrimitiveFunction func) {
  union atom_value value = {.primitive = func};
  return new_atom(ATOM_TYPE_PRIMITIVE, value);
}

typedef int64_t (*IArithmeticFunction)(int64_t, int64_t);
typedef double (*FArithmeticFunction)(double, double);

typedef int (*ComparisonFunction)(struct atom *, struct atom *);

static int64_t iadd(int64_t a, int64_t b) {
  return a + b;
}

static int64_t isub(int64_t a, int64_t b) {
  return a - b;
}

static int64_t imul(int64_t a, int64_t b) {
  return a * b;
}

static int64_t idiv(int64_t a, int64_t b) {
  if (b == 0) {
    fprintf(stderr, "Error: division by zero\n");
    return 0;
  }
  return a / b;
}

static double fadd(double a, double b) {
  return a + b;
}

static double fsub(double a, double b) {
  return a - b;
}

static double fmul(double a, double b) {
  return a * b;
}

static double fdiv(double a, double b) {
  if (b == 0.0) {
    fprintf(stderr, "Error: division by zero\n");
    return 0.0;
  }
  return a / b;
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

int check_arithmetic_args(struct atom *args) {
  struct atom *first_arg = car(args);
  enum AtomType type = first_arg->type;

  args = cdr(args);
  while (args && args->type == ATOM_TYPE_CONS) {
    struct atom *arg = car(args);
    if (arg->type != type) {
      fprintf(stderr,
              "Error: arithmetic operations require all arguments to be of the same type\n");
      return 0;
    } else if (arg->type != ATOM_TYPE_INT && arg->type != ATOM_TYPE_FLOAT) {
      fprintf(stderr, "Error: arithmetic operations only support integers and floats\n");
      return 0;
    }
    args = cdr(args);
  }

  return 1;
}

struct atom *iarithmetic(struct atom *args, IArithmeticFunction func) {
  struct atom *first_arg = car(args);
  int64_t value = first_arg->value.ivalue;

  args = cdr(args);
  while (args) {
    struct atom *arg = car(args);
    value = func(value, arg->value.ivalue);
    args = cdr(args);
  }

  union atom_value result_value = {.ivalue = value};
  return new_atom(ATOM_TYPE_INT, result_value);
}

struct atom *farithmetic(struct atom *args, FArithmeticFunction func) {
  struct atom *first_arg = car(args);
  double value = first_arg->value.ivalue;

  args = cdr(args);
  while (args) {
    struct atom *arg = car(args);
    value = func(value, arg->value.ivalue);
    args = cdr(args);
  }

  union atom_value result_value = {.ivalue = value};
  return new_atom(ATOM_TYPE_FLOAT, result_value);
}

struct atom *primitive_add(struct atom *args, struct environment *env) {
  (void)env;

  if (!check_arithmetic_args(args)) {
    return NULL;
  }

  if (car(args)->type == ATOM_TYPE_INT) {
    return iarithmetic(args, iadd);
  } else if (car(args)->type == ATOM_TYPE_FLOAT) {
    return farithmetic(args, fadd);
  } else {
    fprintf(stderr, "Error: '+' only supports integers and floats\n");
    return NULL;
  }
}

struct atom *primitive_subtract(struct atom *args, struct environment *env) {
  (void)env;

  if (!check_arithmetic_args(args)) {
    return NULL;
  }

  if (car(args)->type == ATOM_TYPE_INT) {
    return iarithmetic(args, isub);
  } else if (car(args)->type == ATOM_TYPE_FLOAT) {
    return farithmetic(args, fsub);
  } else {
    fprintf(stderr, "Error: '-' only supports integers and floats\n");
    return NULL;
  }
}

struct atom *primitive_multiply(struct atom *args, struct environment *env) {
  (void)env;

  if (!check_arithmetic_args(args)) {
    return NULL;
  }

  if (car(args)->type == ATOM_TYPE_INT) {
    return iarithmetic(args, imul);
  } else if (car(args)->type == ATOM_TYPE_FLOAT) {
    return farithmetic(args, fmul);
  } else {
    fprintf(stderr, "Error: '*' only supports integers and floats\n");
    return NULL;
  }
}

struct atom *primitive_divide(struct atom *args, struct environment *env) {
  (void)env;

  if (!check_arithmetic_args(args)) {
    return NULL;
  }

  if (car(args)->type == ATOM_TYPE_INT) {
    return iarithmetic(args, idiv);
  } else if (car(args)->type == ATOM_TYPE_FLOAT) {
    return farithmetic(args, fdiv);
  } else {
    fprintf(stderr, "Error: '/' only supports integers and floats\n");
    return NULL;
  }
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

struct atom *primitive_read(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.car) {
    fprintf(stderr, "Error: 'read' requires one argument\n");
    return NULL;
  }

  struct atom *input = car(args);
  if (input->type != ATOM_TYPE_STRING) {
    fprintf(stderr, "Error: 'read' argument must be a string\n");
    return NULL;
  }

  // TODO: need read_atom to support reading from strings
  // return read_atom(fp);
  return atom_nil();
}

struct atom *primitive_readf(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.car) {
    fprintf(stderr, "Error: 'readf' requires one argument\n");
    return NULL;
  }

  struct atom *input = car(args);
  if (input->type != ATOM_TYPE_STRING) {
    fprintf(stderr, "Error: 'readf' argument must be a string\n");
    return NULL;
  }

  FILE *fp = fopen(input->value.string.ptr, "r");
  if (!fp) {
    fprintf(stderr, "Error: could not open file '%s'\n", input->value.string.ptr);
    return NULL;
  }

  struct atom *result = read_atom(fp);
  fclose(fp);
  if (!result) {
    fprintf(stderr, "Error: could not read from file '%s'\n", input->value.string.ptr);
    return NULL;
  }

  return result;
}

struct atom *primitive_slurp(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.car) {
    fprintf(stderr, "Error: 'slurp' requires one argument\n");
    return NULL;
  }

  struct atom *input = car(args);
  if (input->type != ATOM_TYPE_STRING) {
    fprintf(stderr, "Error: 'slurp' argument must be a string\n");
    return NULL;
  }

  FILE *fp = fopen(input->value.string.ptr, "r");
  if (!fp) {
    fprintf(stderr, "Error: could not open file '%s'\n", input->value.string.ptr);
    return NULL;
  }

  off_t file_size = 0;
  fseek(fp, 0, SEEK_END);
  file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char *buffer = (char *)malloc(file_size + 1);
  if (!buffer) {
    fprintf(stderr, "Error: could not allocate memory for file contents\n");
    fclose(fp);
    return NULL;
  }

  size_t bytes_read = fread(buffer, 1, file_size, fp);
  fclose(fp);
  if (bytes_read != (size_t)file_size) {
    fprintf(stderr, "Error: could not read entire file '%s'\n", input->value.string.ptr);
    free(buffer);
    return NULL;
  }

  buffer[file_size] = '\0';

  union atom_value value = {.string = {.ptr = buffer, .len = file_size}};
  return new_atom(ATOM_TYPE_STRING, value);
}

void init_primitives(struct environment *env) {
  // Add primitive functions to the environment
  env_bind_noref(env, intern_noref("+", 0), primitive_function(primitive_add));
  env_bind_noref(env, intern_noref("-", 0), primitive_function(primitive_subtract));
  env_bind_noref(env, intern_noref("*", 0), primitive_function(primitive_multiply));
  env_bind_noref(env, intern_noref("/", 0), primitive_function(primitive_divide));
  env_bind_noref(env, intern_noref("eq?", 0), primitive_function(primitive_equal));
  env_bind_noref(env, intern_noref("cons", 0), primitive_function(primitive_cons));
  env_bind_noref(env, intern_noref("car", 0), primitive_function(primitive_car));
  env_bind_noref(env, intern_noref("cdr", 0), primitive_function(primitive_cdr));
  env_bind_noref(env, intern_noref("atom?", 0), primitive_function(primitive_atomp));
  env_bind_noref(env, intern_noref("nil?", 0), primitive_function(primitive_nilp));
  env_bind_noref(env, intern_noref("apply", 0), primitive_function(primitive_apply));
  env_bind_noref(env, intern_noref("eval", 0), primitive_function(primitive_eval));
  env_bind_noref(env, intern_noref("print", 0), primitive_function(primitive_print));
  env_bind_noref(env, intern_noref("read", 0), primitive_function(primitive_read));
  env_bind_noref(env, intern_noref("readf", 0), primitive_function(primitive_readf));
  env_bind_noref(env, intern_noref("slurp", 0), primitive_function(primitive_slurp));
}
