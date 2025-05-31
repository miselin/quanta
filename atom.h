#ifndef _MATTLISP_ATOM_H
#define _MATTLISP_ATOM_H

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
};

typedef struct atom *(*PrimitiveFunction)(struct atom *args, struct environment *env);

struct cons {
  struct atom *car;
  struct atom *cdr;
};

struct atom {
  enum AtomType type;
  union {
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
    } lambda;
  } value;
};

void free_atom(struct atom *atom);

static inline int is_primitive_type(enum AtomType type) {
  return type == ATOM_TYPE_INT || type == ATOM_TYPE_FLOAT || type == ATOM_TYPE_STRING ||
         type == ATOM_TYPE_NIL || type == ATOM_TYPE_TRUE;
}

struct atom *car(struct atom *atom);
struct atom *cdr(struct atom *atom);

#endif  // _MATTLISP_ATOM_H
