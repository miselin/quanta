#include "primitive.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atom.h"
#include "eval.h"
#include "intern.h"
#include "print.h"
#include "read.h"
#include "source.h"

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
  return new_cons(cell_car, cell_cdr);
}

struct atom *primitive_car(struct atom *args, struct environment *env) {
  (void)env;

  return car(args);
}

struct atom *primitive_cdr(struct atom *args, struct environment *env) {
  (void)env;

  return cdr(args);
}

static int check_arithmetic_args(struct atom *args, struct atom **error) {
  struct atom *first_arg = car(args);
  enum AtomType type = first_arg->type;

  *error = NULL;

  args = cdr(args);
  while (args && args->type == ATOM_TYPE_CONS) {
    struct atom *arg = car(args);
    if (arg->type != type) {
      *error = new_atom_error(first_arg,
                              "arithmetic operations require all arguments to be of the same type, "
                              "expected %s, got %s",
                              atom_type_to_string(type), atom_type_to_string(arg->type));
      return 0;
    } else if (arg->type != ATOM_TYPE_INT && arg->type != ATOM_TYPE_FLOAT) {
      *error = new_atom_error(arg, "arithmetic operations only support integers and floats, got %s",
                              atom_type_to_string(arg->type));
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
  while (!is_nil(args)) {
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
  while (!is_nil(args)) {
    struct atom *arg = car(args);
    value = func(value, arg->value.ivalue);
    args = cdr(args);
  }

  union atom_value result_value = {.ivalue = value};
  return new_atom(ATOM_TYPE_FLOAT, result_value);
}

struct atom *primitive_add(struct atom *args, struct environment *env) {
  (void)env;

  struct atom *error = NULL;
  if (!check_arithmetic_args(args, &error)) {
    return error;
  }

  if (car(args)->type == ATOM_TYPE_INT) {
    return iarithmetic(args, iadd);
  } else if (car(args)->type == ATOM_TYPE_FLOAT) {
    return farithmetic(args, fadd);
  } else {
    return new_atom_error(car(args), "Error: '+' only supports integers and floats, got %s",
                          atom_type_to_string(car(args)->type));
  }
}

struct atom *primitive_subtract(struct atom *args, struct environment *env) {
  (void)env;

  struct atom *error = NULL;
  if (!check_arithmetic_args(args, &error)) {
    return error;
  }

  if (car(args)->type == ATOM_TYPE_INT) {
    return iarithmetic(args, isub);
  } else if (car(args)->type == ATOM_TYPE_FLOAT) {
    return farithmetic(args, fsub);
  } else {
    return new_atom_error(car(args), "Error: '-' only supports integers and floats, got %s",
                          atom_type_to_string(car(args)->type));
  }
}

struct atom *primitive_multiply(struct atom *args, struct environment *env) {
  (void)env;

  struct atom *error = NULL;
  if (!check_arithmetic_args(args, &error)) {
    return error;
  }

  if (car(args)->type == ATOM_TYPE_INT) {
    return iarithmetic(args, imul);
  } else if (car(args)->type == ATOM_TYPE_FLOAT) {
    return farithmetic(args, fmul);
  } else {
    return new_atom_error(car(args), "Error: '*' only supports integers and floats, got %s",
                          atom_type_to_string(car(args)->type));
  }
}

struct atom *primitive_divide(struct atom *args, struct environment *env) {
  (void)env;

  struct atom *error = NULL;
  if (!check_arithmetic_args(args, &error)) {
    return error;
  }

  if (car(args)->type == ATOM_TYPE_INT) {
    return iarithmetic(args, idiv);
  } else if (car(args)->type == ATOM_TYPE_FLOAT) {
    return farithmetic(args, fdiv);
  } else {
    return new_atom_error(car(args), "Error: '/' only supports integers and floats, got %s",
                          atom_type_to_string(car(args)->type));
  }
}

struct atom *primitive_equal(struct atom *args, struct environment *env) {
  (void)env;

  // Must be two arguments
  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.cdr ||
      args->value.cons.cdr->type != ATOM_TYPE_CONS) {
    return new_atom_error(args, "Error: '=' requires exactly two arguments");
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
    case ATOM_TYPE_ERROR:
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
  struct atom *arguments = car(cdr(args));

  return apply(fn, arguments, env);
}

struct atom *primitive_eval(struct atom *args, struct environment *env) {
  (void)env;

  // TODO: car() here is probably not completely right
  return eval(car(args), env);
}

struct atom *primitive_not_equal(struct atom *args, struct environment *env);
struct atom *primitive_less_than(struct atom *args, struct environment *env);
struct atom *primitive_greater_than(struct atom *args, struct environment *env);
struct atom *primitive_less_than_equal(struct atom *args, struct environment *env);
struct atom *primitive_greater_than_equal(struct atom *args, struct environment *env);

struct atom *primitive_print(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS) {
    return new_atom_error(args, "Error: 'print' requires at least one argument");
  }

  while (args && args->type == ATOM_TYPE_CONS) {
    print(stdout, car(args), 1);
    printf("\n");
    args = cdr(args);
  }

  return atom_nil();
}

struct atom *primitive_write(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS) {
    return new_atom_error(args, "Error: 'write' requires at least one argument");
  }

  while (args && args->type == ATOM_TYPE_CONS) {
    print(stdout, car(args), 0);
    printf("\n");
    args = cdr(args);
  }

  return atom_nil();
}

struct atom *primitive_to_string(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS) {
    return new_atom_error(args, "Error: 'to-string' requires at least one argument");
  }

  struct atom *arg = car(args);
  if (cdr(args) != atom_nil()) {
    return new_atom_error(args, "Error: 'to-string' requires exactly one argument");
  }

  char *buf = (char *)malloc(1024);
  if (print_str(buf, 1024, arg, 0) <= 0) {
    free(buf);
    return new_atom_error(arg, "Error: could not convert atom to string");
  }

  union atom_value value = {.string = {.ptr = buf, .len = strlen(buf)}};
  return new_atom(ATOM_TYPE_STRING, value);
}

struct atom *primitive_read(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.car) {
    return new_atom_error(args, "Error: 'read' requires one argument");
  }

  struct atom *input = car(args);
  if (input->type != ATOM_TYPE_STRING) {
    return new_atom_error(input, "Error: 'read' argument must be a string");
  }

  struct source_file *source = source_file_str(input->value.string.ptr, input->value.string.len);
  if (!source) {
    return new_atom_error(input, "Error: could not create source from string '%s'",
                          input->value.string.ptr);
  }

  struct atom *result = read_atom(source);

  source_file_free(source);

  if (!result) {
    return new_atom_error(input, "Error: could not read from string '%s'", input->value.string.ptr);
  }

  return result;
}

struct atom *primitive_readf(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.car) {
    return new_atom_error(args, "Error: 'readf' requires one argument");
  }

  struct atom *input = car(args);
  if (input->type != ATOM_TYPE_STRING) {
    return new_atom_error(input, "Error: 'readf' argument must be a string");
  }

  struct source_file *source = source_file_new(input->value.string.ptr);
  if (!source) {
    return new_atom_error(input, "Error: could not open file '%s'", input->value.string.ptr);
  }

  struct atom *result = read_atom(source);

  source_file_free(source);

  if (!result) {
    return new_atom_error(input, "Error: could not read from file '%s'", input->value.string.ptr);
  }

  return result;
}

struct atom *primitive_read_all(struct atom *args, struct environment *env) {
  (void)env;

  if (!args || args->type != ATOM_TYPE_CONS || !args->value.cons.car) {
    return new_atom_error(args, "Error: 'read-all' requires one argument");
  }

  struct atom *input = car(args);
  if (input->type != ATOM_TYPE_STRING) {
    return new_atom_error(input, "Error: 'read-all' argument must be a string");
  }

  struct source_file *source = source_file_new(input->value.string.ptr);
  if (!source) {
    return new_atom_error(input, "Error: could not open file '%s'", input->value.string.ptr);
  }

  struct atom *head = NULL;
  struct atom *tail = NULL;

  while (!source_file_eof(source)) {
    struct atom *result = read_atom(source);
    if (!result) {
      struct atom *error =
          new_atom_error(input, "Error: could not read from file '%s'", input->value.string.ptr);
      source_file_free(source);
      return error;
    }

    struct atom *cons = new_cons(result, NULL);
    if (!head) {
      head = cons;
      tail = head;
    } else {
      tail->value.cons.cdr = cons;
      tail = cons;
    }
  }

  source_file_free(source);

  if (!head) {
    return atom_nil();
  }

  return head;
}

struct atom *primitive_read_line(struct atom *args, struct environment *env) {
  (void)args;
  (void)env;

  // read a single line from stdin
  char *buffer = (char *)malloc(1024);
  if (!buffer) {
    return new_atom_error(NULL, "Error: could not allocate memory for reading line");
  }

  size_t at = 0;
  size_t sz = 1024;
  while (1) {
    char c = fgetc(stdin);
    if (c == EOF || c == '\n' || c == '\r') {
      buffer[at] = '\0';
      break;
    }

    buffer[at++] = c;
    if (at >= (sz - 1)) {
      sz *= 2;
      buffer = (char *)realloc(buffer, sz);
    }
  }

  union atom_value value = {.string = {.ptr = buffer, .len = at}};
  return new_atom(ATOM_TYPE_STRING, value);
}

void init_primitives(struct environment *env) {
  env_bind(env, intern("+", 0), primitive_function(primitive_add));
  env_bind(env, intern("-", 0), primitive_function(primitive_subtract));
  env_bind(env, intern("*", 0), primitive_function(primitive_multiply));
  env_bind(env, intern("/", 0), primitive_function(primitive_divide));
  env_bind(env, intern("eq?", 0), primitive_function(primitive_equal));
  env_bind(env, intern("cons", 0), primitive_function(primitive_cons));
  env_bind(env, intern("car", 0), primitive_function(primitive_car));
  env_bind(env, intern("cdr", 0), primitive_function(primitive_cdr));
  env_bind(env, intern("atom?", 0), primitive_function(primitive_atomp));
  env_bind(env, intern("nil?", 0), primitive_function(primitive_nilp));
  env_bind(env, intern("apply", 0), primitive_function(primitive_apply));
  env_bind(env, intern("eval", 0), primitive_function(primitive_eval));
  env_bind(env, intern("print", 0), primitive_function(primitive_print));
  env_bind(env, intern("write", 0), primitive_function(primitive_write));
  env_bind(env, intern("to-string", 0), primitive_function(primitive_to_string));
  env_bind(env, intern("read", 0), primitive_function(primitive_read));
  env_bind(env, intern("readf", 0), primitive_function(primitive_readf));
  env_bind(env, intern("read-all", 0), primitive_function(primitive_read_all));
  env_bind(env, intern("read-line", 0), primitive_function(primitive_read_line));
}
