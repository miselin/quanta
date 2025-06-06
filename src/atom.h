#ifndef _QUANTA_ATOM_H
#define _QUANTA_ATOM_H

#include <glib-2.0/glib.h>
#include <stddef.h>
#include <stdint.h>

#include "env.h"

enum AtomType {
  ATOM_TYPE_INT = 0,        // 42
  ATOM_TYPE_FLOAT = 1,      // 3.14
  ATOM_TYPE_STRING = 2,     // "..."
  ATOM_TYPE_CONS = 3,       // lists
  ATOM_TYPE_NIL = 4,        // "nil"
  ATOM_TYPE_SYMBOL = 5,     // e.g. symbol (without quotes)
  ATOM_TYPE_KEYWORD = 6,    // e.g. :keyword
  ATOM_TYPE_TRUE = 7,       // t (nil is false)
  ATOM_TYPE_PRIMITIVE = 8,  // primitive functions
  ATOM_TYPE_SPECIAL = 9,    // special forms (e.g. if, define)
  ATOM_TYPE_LAMBDA = 10,    // user-defined functions
  ATOM_TYPE_ERROR = 11,     // error, to propagate errors in evaluation
  ATOM_TYPE_EOF = 12,       // end of file marker
};

#define ATOM_LAMBDA_FLAG_MACRO (1 << 0)

typedef struct atom *(*PrimitiveFunction)(struct atom *args, struct environment *env);

struct cons {
  struct atom *car;
  struct atom *cdr;
};

union atom_value {
  int64_t ivalue;
  double fvalue;
  struct {
    char *ptr;
    size_t len;
  } string;
  struct cons cons;
  PrimitiveFunction primitive;
  struct {
    struct atom *args;
    struct environment *env;
    struct atom *body;
    int flags;
  } lambda;
  struct {
    char *message;
    struct atom *cause;
  } error;
};

struct atom {
  enum AtomType type;
  union atom_value value;
};

#ifdef __cplusplus
extern "C" {
#endif

struct atom *atom_nil(void);
struct atom *atom_true(void);
struct atom *atom_eof(void);

struct atom *new_atom(enum AtomType type, union atom_value value);

// Erase allocated memory inside an atom (e.g. strings)
// Does not free other atoms (e.g. cons cells).
void erase_atom(struct atom *atom);

struct atom *new_cons(struct atom *car, struct atom *cdr);

struct atom *car(struct atom *atom);
struct atom *cdr(struct atom *atom);

int is_cons(struct atom *atom);
int is_nil(struct atom *atom);
int is_symbol(struct atom *atom);
int is_keyword(struct atom *atom);
int is_string(struct atom *atom);
int is_int(struct atom *atom);
int is_float(struct atom *atom);
int is_true(struct atom *atom);
int is_basic_type(struct atom *atom);
int is_error(struct atom *atom);
int is_lambda(struct atom *atom);
int is_primitive(struct atom *atom);
int is_special(struct atom *atom);
int is_eof(struct atom *atom);

const char *atom_type_to_string(enum AtomType type);

// Exposed for GC
void atom_mark(struct atom *atom);

struct atom *new_atom_error(struct atom *cause, const char *message, ...);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _QUANTA_ATOM_H
